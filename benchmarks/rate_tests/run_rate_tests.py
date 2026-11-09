#!/usr/bin/env python3

from __future__ import annotations

import argparse
import csv
import getpass
import json
import os
from pathlib import Path
import signal
import shutil
import struct
import subprocess
import sys
import time
from dataclasses import asdict, dataclass
from datetime import datetime

import psutil
import zmq


EXPERIMENT_NAME = "ZMQ_RATE_TEST"
TIMING_MAGIC = 0x5254494D


@dataclass(frozen=True)
class Scenario:
    name: str
    payload_bytes: int
    interval_us: int


DEFAULT_SCENARIOS = [
    Scenario("1KiB_1kHz", 1024, 1000),
    Scenario("16KiB_5kHz", 16 * 1024, 200),
    Scenario("64KiB_10kHz", 64 * 1024, 100),
    Scenario("1MiB_1kHz", 1024 * 1024, 1000),
    Scenario("64KiB_saturation", 64 * 1024, 0),
    Scenario("1MiB_saturation", 1024 * 1024, 0),
]

MATRIX_PAYLOADS = [1024, 4 * 1024, 16 * 1024, 64 * 1024, 256 * 1024, 1024 * 1024]
MATRIX_RATES_HZ = [100, 300, 1000, 3000]


def matrix_scenarios() -> list[Scenario]:
    scenarios = []
    for payload in MATRIX_PAYLOADS:
        size_label = f"{payload // 1024}KiB"
        for rate in MATRIX_RATES_HZ:
            scenarios.append(Scenario(
                f"matrix_{size_label}_{rate}Hz",
                payload,
                round(1_000_000 / rate),
            ))
        scenarios.append(Scenario(f"matrix_{size_label}_saturation", payload, 0))
    return scenarios


class ManagedProcess:
    def __init__(self, name: str, command: list[str], env: dict[str, str], log_dir: Path):
        self.name = name
        self.log_path = log_dir / f"{name}.log"
        self.log_file = self.log_path.open("a", encoding="utf-8")
        self.process = subprocess.Popen(
            command,
            env=env,
            stdout=self.log_file,
            stderr=subprocess.STDOUT,
            text=True,
            start_new_session=True,
        )

    @property
    def pid(self) -> int:
        return self.process.pid

    def require_running(self) -> None:
        status = self.process.poll()
        if status is not None:
            raise RuntimeError(
                f"{self.name} exited with status {status}; see {self.log_path}")

    def stop(self, requested_signal: int = signal.SIGINT) -> None:
        if self.process.poll() is None:
            os.killpg(self.process.pid, requested_signal)
            try:
                self.process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                os.killpg(self.process.pid, signal.SIGTERM)
                try:
                    self.process.wait(timeout=3)
                except subprocess.TimeoutExpired:
                    os.killpg(self.process.pid, signal.SIGKILL)
                    self.process.wait(timeout=3)
        self.log_file.close()


class MidasExperiment:
    def __init__(self, midas_root: Path, output_dir: Path, equipment_names: list[str]):
        self.midas_root = midas_root.resolve()
        self.experiment_dir = (output_dir / "experiment").resolve()
        self.experiment_dir.mkdir(parents=True, exist_ok=True)
        self.exptab = self.experiment_dir / "exptab"
        self.equipment_names = equipment_names
        self.exptab.write_text(
            f"{EXPERIMENT_NAME} {self.experiment_dir} {getpass.getuser()}\n",
            encoding="utf-8",
        )
        self.env = os.environ.copy()
        self.env.update({
            "MIDASSYS": str(self.midas_root),
            "MIDAS_EXPTAB": str(self.exptab),
            "MIDAS_EXPT_NAME": EXPERIMENT_NAME,
            "PATH": f"{self.midas_root / 'bin'}:{self.env.get('PATH', '')}",
        })

    def initialize(self) -> None:
        # POSIX MIDAS shared-memory names survive removal of the artifact
        # directory after an interrupted run. Clean that exact experiment
        # mapping before creating the fresh benchmark ODB.
        prefix = f"{os.getuid()}_{EXPERIMENT_NAME}_"
        shared_memory = Path("/dev/shm")
        if shared_memory.is_dir():
            for path in shared_memory.iterdir():
                if path.name.startswith(prefix):
                    path.unlink()
        self.run("odbinit", "-e", EXPERIMENT_NAME, "--cleanup", check=False)
        for stale in self.experiment_dir.glob(".*.SHM"):
            stale.unlink(missing_ok=True)
        for name in (".ODB_SIZE.TXT", ".SHM_HOST.TXT", ".SHM_TYPE.TXT"):
            (self.experiment_dir / name).unlink(missing_ok=True)
        self.run("odbinit", "-e", EXPERIMENT_NAME, "-s", "64MB")
        # Close the segment created by odbinit after preserving its snapshot.
        # This avoids a POSIX-SHM startup race where the first frontend can see
        # a zeroed ODB root key even though .ODB.SHM is valid.
        self.run("odbinit", "-e", EXPERIMENT_NAME, "--cleanup", check=False)
        time.sleep(0.25)

    def run(self, program: str, *arguments: str, check: bool = True) -> subprocess.CompletedProcess:
        return subprocess.run(
            [str(self.midas_root / "bin" / program), *arguments],
            env=self.env,
            cwd=self.experiment_dir,
            check=check,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        )

    def odb(self, command: str, check: bool = True) -> subprocess.CompletedProcess:
        return self.run("odbedit", "-e", EXPERIMENT_NAME, "-c", command, check=check)

    def wait_for_equipment(
        self,
        equipment_names: list[str] | None = None,
        timeout: float = 15.0,
    ) -> None:
        equipment_names = equipment_names or self.equipment_names
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            ready = True
            for equipment_name in equipment_names:
                result = self.odb(
                    f'ls "/Equipment/{equipment_name}/Settings"', check=False)
                ready = ready and result.returncode == 0 and "Payload Bytes" in result.stdout
            if ready:
                return
            time.sleep(0.2)
        raise RuntimeError("MIDAS rate frontends did not create all ODB settings")

    def configure(self, scenario: Scenario, maximum_events: int = 0) -> None:
        for equipment_name in self.equipment_names:
            base = f"/Equipment/{equipment_name}/Settings"
            self.odb(f'set "{base}/Payload Bytes" {scenario.payload_bytes}')
            self.odb(f'set "{base}/Polling Interval (us)" {scenario.interval_us}')
            self.odb(f'set "{base}/Maximum Events" {maximum_events}')

    def start_run(self) -> None:
        self.run("mtransition", "-e", EXPERIMENT_NAME, "START")

    def stop_run(self) -> None:
        self.run("mtransition", "-e", EXPERIMENT_NAME, "STOP", check=False)


class ZmqCapture:
    def __init__(self, data_endpoint: str, metrics_endpoint: str, processes: dict[str, int]):
        self.context = zmq.Context()
        self.metrics = self.context.socket(zmq.SUB)
        self.metrics.setsockopt(zmq.SUBSCRIBE, b"METRICS")
        self.metrics.connect(metrics_endpoint)
        self.poller = zmq.Poller()
        self.poller.register(self.metrics, zmq.POLLIN)
        self.latest_metrics: dict | None = None
        self.data_messages = 0
        self.parsed_messages = 0
        self.processes = {name: psutil.Process(pid) for name, pid in processes.items()}
        for process in self.processes.values():
            process.cpu_percent(None)

    def replace_process(self, name: str, pid: int) -> None:
        process = psutil.Process(pid)
        process.cpu_percent(None)
        self.processes[name] = process

    @staticmethod
    def timing_record(event: bytes) -> tuple[int, int, int] | None:
        if len(event) < 24:
            return None
        _, _, _, _, data_size = struct.unpack_from("<hhIII", event, 0)
        bank_size, flags = struct.unpack_from("<II", event, 16)
        end = min(len(event), 24 + bank_size, 16 + data_size)
        position = 24
        while position < end:
            if flags & (1 << 5):
                name, _, size, _ = struct.unpack_from("<4sIII", event, position)
                data_offset = position + 16
            elif flags & (1 << 4):
                name, _, size = struct.unpack_from("<4sII", event, position)
                data_offset = position + 12
            else:
                name, _, size = struct.unpack_from("<4sHH", event, position)
                data_offset = position + 8
            if name == b"RTIM" and size >= 40:
                magic, _, _, sequence, monotonic_ns, _, payload_bytes, _ = struct.unpack_from(
                    "<IHHQQQII", event, data_offset)
                if magic == TIMING_MAGIC:
                    return sequence, monotonic_ns, payload_bytes
            position = data_offset + ((size + 7) & ~7)
        return None

    def drain(self, duration: float) -> None:
        self._collect(duration, None, None, None, None)

    def collect(self, duration: float, scenario_dir: Path) -> None:
        scenario_dir.mkdir(parents=True, exist_ok=True)
        with (scenario_dir / "events.csv").open("w", newline="", encoding="utf-8") as events_file, \
             (scenario_dir / "telemetry.jsonl").open("w", encoding="utf-8") as telemetry_file, \
             (scenario_dir / "resources.csv").open("w", newline="", encoding="utf-8") as resources_file:
            events = csv.writer(events_file)
            events.writerow([
                "received_monotonic_ns",
                "producer_monotonic_ns",
                "receiver_monotonic_ns",
                "processor_dequeued_ns",
                "product_created_ns",
                "published_ns",
                "source_sequence",
                "publish_sequence",
                "payload_bytes",
                "product_bytes",
                "receiver_events_received",
                "receiver_events_consumed",
                "receiver_events_dropped",
                "receiver_queue_depth",
                "receiver_batch_queue_depth",
                "receiver_queue_high_watermark",
                "receiver_total_queue_latency_ns",
            ])
            resources = csv.writer(resources_file)
            resources.writerow(["monotonic_ns", "process", "cpu_percent", "rss_bytes"])
            if self.latest_metrics is not None:
                telemetry_file.write(json.dumps(self.latest_metrics) + "\n")
            self._collect(duration, events, telemetry_file, resources, scenario_dir)

    def _collect(self, duration: float, events, telemetry_file, resources, scenario_dir) -> None:
        deadline = time.monotonic() + duration
        next_resource_sample = time.monotonic()
        while time.monotonic() < deadline:
            ready = dict(self.poller.poll(timeout=25))
            if self.metrics in ready:
                frames = self.metrics.recv_multipart()
                if len(frames) == 2:
                    document = json.loads(frames[1])
                    document["captured-monotonic-ns"] = time.monotonic_ns()
                    self.latest_metrics = document
                    if telemetry_file is not None:
                        telemetry_file.write(json.dumps(document) + "\n")
            now = time.monotonic()
            if resources is not None and now >= next_resource_sample:
                sample_ns = time.monotonic_ns()
                for name, process in self.processes.items():
                    try:
                        resources.writerow([
                            sample_ns,
                            name,
                            process.cpu_percent(None),
                            process.memory_info().rss,
                        ])
                    except psutil.Error:
                        pass
                next_resource_sample = now + 0.25

    def close(self) -> None:
        self.metrics.close(0)
        self.context.term()


def materialize_native_events(output: Path, scenarios: list[Scenario]) -> None:
    with (output / "native_events.csv").open(newline="", encoding="utf-8") as source:
        reader = csv.DictReader(source)
        fieldnames = reader.fieldnames
        if fieldnames is None:
            raise RuntimeError("Native subscriber produced no CSV header")
        rows = list(reader)
    for scenario in scenarios:
        scenario_dir = output / scenario.name
        metadata = json.loads((scenario_dir / "scenario.json").read_text(encoding="utf-8"))
        start = metadata["measurement_start_monotonic_ns"]
        end = metadata["measurement_end_monotonic_ns"]
        selected = [
            row for row in rows
            if start <= int(row["received_monotonic_ns"]) < end
        ]
        with (scenario_dir / "events.csv").open("w", newline="", encoding="utf-8") as target:
            writer = csv.DictWriter(target, fieldnames=fieldnames)
            writer.writeheader()
            writer.writerows(selected)
        print(f"  {scenario.name}: native subscriber received={len(selected)}", flush=True)


def publisher_config(
    build_dir: Path,
    output: Path,
    data_endpoint: str,
    metrics_endpoint: str,
    delivery_batch_size: int,
    midas_yield_timeout_ms: int,
    events_per_retrieval: int,
    receiver_buffer_events: int,
    midas_read_cache_bytes: int,
    gaudi_socket: str | None,
    gaudi_maximum_batch_bytes: int,
    gaudi_publish_every_events: int,
    gaudi_publish_every_ms: int,
    gaudi_launcher: Path | None,
    gaudi_job_options: Path | None,
) -> Path:
    extension_name = (
        "libzmq_publisher_gaudi_extension.so"
        if gaudi_socket else "libzmq_publisher_midas_extension.so"
    )
    midas_config = {
        "host": "",
        "experiment": EXPERIMENT_NAME,
        "buffer": "SYSTEM",
        "client-name": "ZmqRatePublisher",
        "event-id": 42,
        "get-all": True,
        "buffer-size": receiver_buffer_events,
        "read-cache-size-bytes": midas_read_cache_bytes,
        "yield-timeout-ms": midas_yield_timeout_ms,
    }
    if gaudi_socket:
        sidecar_config = {
            "socket-path": gaudi_socket,
            "connect-timeout-ms": 5000,
            "request-timeout-ms": 30000,
        }
        if gaudi_launcher is not None and gaudi_job_options is not None:
            sidecar_config.update({
                "launcher": str(gaudi_launcher.resolve()),
                "job-options": str(gaudi_job_options.resolve()),
                "startup-timeout-ms": 60000,
            })
        processor_config = {
            "midas": midas_config,
            "processing": {
                "events-per-request": events_per_retrieval,
                "maximum-batch-bytes": gaudi_maximum_batch_bytes,
            },
            "publication": {
                "every-events": gaudi_publish_every_events,
                "every-ms": gaudi_publish_every_ms,
            },
            "sidecar": sidecar_config,
        }
    else:
        midas_config["num-events-per-retrieval"] = events_per_retrieval
        processor_config = midas_config

    config = {
        "general-settings": {
            "verbose": 0,
            "worker-threads": 2,
            "transport": {
                "worker-threads": 1,
                "zmq-io-threads": 1,
                "queue-capacity": 65536,
                "queue-policy": "block",
            },
            "telemetry": {
                "enabled": True,
                "interval-ms": 250,
                "endpoint": metrics_endpoint,
                "topic": "METRICS",
            },
            "processor-extensions": [
                str((build_dir / f"lib/{extension_name}").resolve())
            ],
        },
        "data-channels": {
            "midas-events": {
                "enabled": True,
                "endpoint": data_endpoint,
                "topic": "DATA",
                "delivery": (
                    {"mode": "each"}
                    if delivery_batch_size == 1
                    else {"mode": "batch", "size": delivery_batch_size}
                ),
                "processors": [{
                    "type": "midas-gaudi" if gaudi_socket else "midas",
                    "name": "midas-source",
                    "schedule": {"mode": "event-driven"},
                    "config": processor_config,
                }],
            },
        },
    }
    path = output / "publisher_config.json"
    path.write_text(json.dumps(config, indent=2) + "\n", encoding="utf-8")
    return path


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Run the MIDAS-to-ZMQ rate benchmark")
    parser.add_argument("--build-dir", type=Path, default=Path("build"))
    parser.add_argument(
        "--detector",
        choices=("raw", "sampic", "musip", "hdsoc", "wfd5", "all"),
        default="raw",
        help="select the benchmark frontend bank format",
    )
    parser.add_argument("--midas-root", type=Path, default=Path("/home/jack/software/midas"))
    parser.add_argument("--output", type=Path)
    parser.add_argument("--replace-output", action="store_true")
    parser.add_argument(
        "--receiver-source",
        type=Path,
        default=Path("/home/jack/cpp_projects/midas_stuff/midas_receiver"),
    )
    parser.add_argument("--duration", type=float, default=4.0)
    parser.add_argument("--warmup", type=float, default=1.0)
    parser.add_argument("--midas-yield-timeout-ms", type=int, default=1)
    parser.add_argument("--events-per-retrieval", type=int, default=128)
    parser.add_argument("--delivery-batch-size", type=int, default=1)
    parser.add_argument("--receiver-buffer-events", type=int, default=4096)
    parser.add_argument("--midas-read-cache-bytes", type=int, default=100000)
    parser.add_argument("--frontend-write-cache-bytes", type=int, default=10_000_000)
    parser.add_argument("--frontend-flush-interval-us", type=int, default=1000)
    parser.add_argument("--data-endpoint", default="tcp://127.0.0.1:5655")
    parser.add_argument("--metrics-endpoint", default="tcp://127.0.0.1:5656")
    parser.add_argument(
        "--gaudi-socket",
        help="Benchmark the midas-gaudi plugin using an already-running sidecar",
    )
    parser.add_argument("--gaudi-maximum-batch-bytes", type=int, default=16 * 1024 * 1024)
    parser.add_argument("--gaudi-publish-every-events", type=int, default=1)
    parser.add_argument("--gaudi-publish-every-ms", type=int, default=0)
    parser.add_argument("--gaudi-launcher", type=Path)
    parser.add_argument("--gaudi-job-options", type=Path)
    parser.add_argument("--scenario", action="append", metavar="NAME:BYTES:INTERVAL_US")
    parser.add_argument(
        "--matrix",
        action="store_true",
        help="run the standard 1 KiB-1 MiB by 100 Hz-saturation benchmark matrix",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    if args.midas_yield_timeout_ms < 0:
        raise ValueError("--midas-yield-timeout-ms cannot be negative")
    if args.events_per_retrieval <= 0:
        raise ValueError("--events-per-retrieval must be positive")
    if args.delivery_batch_size <= 0:
        raise ValueError("--delivery-batch-size must be positive")
    if args.receiver_buffer_events <= 0:
        raise ValueError("--receiver-buffer-events must be positive")
    if args.midas_read_cache_bytes < 0:
        raise ValueError("--midas-read-cache-bytes cannot be negative")
    if args.frontend_write_cache_bytes < 0:
        raise ValueError("--frontend-write-cache-bytes cannot be negative")
    if args.frontend_flush_interval_us < -1:
        raise ValueError("--frontend-flush-interval-us must be -1 or greater")
    if args.gaudi_maximum_batch_bytes <= 0:
        raise ValueError("--gaudi-maximum-batch-bytes must be positive")
    if args.gaudi_publish_every_events < 0 or args.gaudi_publish_every_ms < 0:
        raise ValueError("Gaudi publication intervals cannot be negative")
    if (args.gaudi_launcher is None) != (args.gaudi_job_options is None):
        raise ValueError("--gaudi-launcher and --gaudi-job-options must be used together")
    if args.gaudi_launcher is not None and not args.gaudi_socket:
        raise ValueError("--gaudi-launcher requires --gaudi-socket")
    project_root = Path(__file__).resolve().parents[2]
    build_dir = (project_root / args.build_dir).resolve()
    output = args.output or (project_root / ".artifacts/rate-tests/latest")
    output = output.resolve()
    if output.exists():
        if args.output is not None and not args.replace_output:
            raise FileExistsError(
                f"Output directory already exists: {output}; use --replace-output to replace it")
        shutil.rmtree(output)
    output.mkdir(parents=True, exist_ok=False)
    (output / "logs").mkdir()

    if args.matrix and args.scenario:
        raise ValueError("--matrix and --scenario cannot be used together")
    scenarios = matrix_scenarios() if args.matrix else DEFAULT_SCENARIOS
    if args.scenario:
        scenarios = []
        for encoded in args.scenario:
            name, payload, interval = encoded.split(":")
            scenarios.append(Scenario(name, int(payload), int(interval)))

    frontend_names = {
        "raw": "zmq_publisher_rate_frontend",
        "sampic": "zmq_publisher_sampic_simulator_frontend",
        "musip": "zmq_publisher_musip_simulator_frontend",
        "hdsoc": "zmq_publisher_hdsoc_simulator_frontend",
        "wfd5": "zmq_publisher_wfd5_simulator_frontend",
    }
    equipment_names = {
        "raw": "ZMQ Rate Source 00",
        "sampic": "PIONEER SAMPIC Simulator 00",
        "musip": "PIONEER MUSIP Simulator 00",
        "hdsoc": "PIONEER HDSoC Simulator 00",
        "wfd5": "PIONEER WFD5 Simulator 00",
    }
    selected_detectors = (
        ["sampic", "musip", "hdsoc", "wfd5"]
        if args.detector == "all" else [args.detector]
    )
    frontend_binaries = {
        detector: build_dir / f"bin/{frontend_names[detector]}"
        for detector in selected_detectors
    }
    publisher_binary = build_dir / "bin/zmq_publisher"
    subscriber_binary = build_dir / "bin/zmq_publisher_rate_subscriber"
    for binary in (*frontend_binaries.values(), publisher_binary, subscriber_binary):
        if not binary.is_file():
            raise RuntimeError(f"Missing benchmark binary: {binary}")

    experiment = MidasExperiment(
        args.midas_root,
        output,
        [equipment_names[detector] for detector in selected_detectors],
    )
    experiment.initialize()
    experiment.env["ZMQ_RATE_TEST_WRITE_CACHE_BYTES"] = str(
        args.frontend_write_cache_bytes)
    experiment.env["ZMQ_RATE_TEST_FLUSH_INTERVAL_US"] = str(
        args.frontend_flush_interval_us)
    # The shared online job configures every detector tool so mixed streams can
    # arrive without restarting Gaudi. PITMidasMusip validates geometry during
    # initialization even when this particular benchmark emits another bank.
    if args.gaudi_socket:
        experiment.env["PIONEER_GAUDI_MUSIP_DEMO_GEOMETRY"] = "1"
    config_path = publisher_config(
        build_dir,
        output,
        args.data_endpoint,
        args.metrics_endpoint,
        args.delivery_batch_size,
        args.midas_yield_timeout_ms,
        args.events_per_retrieval,
        args.receiver_buffer_events,
        args.midas_read_cache_bytes,
        args.gaudi_socket,
        args.gaudi_maximum_batch_bytes,
        args.gaudi_publish_every_events,
        args.gaudi_publish_every_ms,
        args.gaudi_launcher,
        args.gaudi_job_options,
    )
    receiver_source = args.receiver_source.resolve()
    receiver_revision = None
    receiver_branch = None
    if (receiver_source / ".git").exists():
        receiver_revision = subprocess.check_output(
            ["git", "rev-parse", "HEAD"], cwd=receiver_source, text=True).strip()
        receiver_branch = subprocess.check_output(
            ["git", "branch", "--show-current"], cwd=receiver_source, text=True).strip()

    (output / "run.json").write_text(json.dumps({
        "created": datetime.now().isoformat(),
        "detector": args.detector,
        "frontend_count": len(selected_detectors),
        "duration_seconds": args.duration,
        "warmup_seconds": args.warmup,
        "midas_yield_timeout_ms": args.midas_yield_timeout_ms,
        "events_per_retrieval": args.events_per_retrieval,
        "delivery_batch_size": args.delivery_batch_size,
        "receiver_buffer_events": args.receiver_buffer_events,
        "midas_read_cache_bytes": args.midas_read_cache_bytes,
        "frontend_write_cache_bytes": args.frontend_write_cache_bytes,
        "frontend_flush_interval_us": args.frontend_flush_interval_us,
        "gaudi_socket": args.gaudi_socket,
        "gaudi_maximum_batch_bytes": args.gaudi_maximum_batch_bytes,
        "gaudi_publish_every_events": args.gaudi_publish_every_events,
        "gaudi_publish_every_ms": args.gaudi_publish_every_ms,
        "gaudi_launcher": str(args.gaudi_launcher.resolve()) if args.gaudi_launcher else None,
        "gaudi_job_options": (
            str(args.gaudi_job_options.resolve()) if args.gaudi_job_options else None
        ),
        "scenarios": [asdict(scenario) for scenario in scenarios],
        "build_dir": str(build_dir),
        "midas_root": str(args.midas_root.resolve()),
        "git_commit": subprocess.check_output(
            ["git", "rev-parse", "HEAD"], cwd=project_root, text=True).strip(),
        "git_dirty": bool(subprocess.check_output(
            ["git", "status", "--porcelain"], cwd=project_root, text=True).strip()),
        "receiver_source": str(receiver_source),
        "receiver_git_commit": receiver_revision,
        "receiver_git_branch": receiver_branch,
        "receiver_git_dirty": bool(subprocess.check_output(
            ["git", "status", "--porcelain"], cwd=receiver_source, text=True).strip())
            if receiver_revision else None,
    }, indent=2) + "\n", encoding="utf-8")

    frontends = []
    publisher = None
    native_subscriber = None
    capture = None
    try:
        for detector, frontend_binary in frontend_binaries.items():
            frontends.append(ManagedProcess(
                f"{detector}_frontend",
                [str(frontend_binary), "-i", "0"],
                experiment.env,
                output / "logs",
            ))
            # MIDAS ODB creation is not safe when several new frontends make
            # their first connection at exactly the same time. Serialize only
            # startup; acquisition remains fully concurrent.
            experiment.wait_for_equipment([equipment_names[detector]])
        publisher = ManagedProcess(
            "zmq_publisher",
            [str(publisher_binary), "--config", str(config_path)],
            experiment.env,
            output / "logs",
        )
        time.sleep(1.0)
        for frontend in frontends:
            frontend.require_running()
        publisher.require_running()

        native_subscriber = ManagedProcess(
            "rate_subscriber",
            [str(subscriber_binary), args.data_endpoint, str(output / "native_events.csv")],
            experiment.env,
            output / "logs",
        )

        capture = ZmqCapture(
            args.data_endpoint,
            args.metrics_endpoint,
            {
                **{frontend.name: frontend.pid for frontend in frontends},
                "publisher": publisher.pid,
                "subscriber": native_subscriber.pid,
            },
        )
        capture.drain(1.0)

        # The transport binds a data endpoint lazily on its first publication.
        # Prime it before measuring so ZeroMQ subscription propagation and the
        # first MIDAS read do not appear as scenario loss.
        # Keep the priming source deliberately slow. Gaudi and detector tools
        # perform lazy initialization; a high-rate prime can create a queue that
        # leaks into the first measured run and invalidates it.
        prime = Scenario("transport-prime", 1024, 50_000)
        experiment.configure(prime, maximum_events=150)
        experiment.start_run()
        capture.drain(8.0 if args.gaudi_socket else 1.5)
        experiment.stop_run()
        capture.drain(0.5)
        native_subscriber.require_running()

        for scenario in scenarios:
            print(
                f"Running {scenario.name}: payload={scenario.payload_bytes} "
                f"interval={scenario.interval_us} us",
                flush=True,
            )
            experiment.configure(scenario)
            experiment.start_run()
            capture.drain(args.warmup)
            scenario_dir = output / scenario.name
            scenario_dir.mkdir(parents=True, exist_ok=True)
            measurement_start = time.monotonic_ns()
            capture.collect(args.duration, scenario_dir)
            measurement_end = time.monotonic_ns()
            (scenario_dir / "scenario.json").write_text(
                json.dumps({
                    **asdict(scenario),
                    "measurement_duration_seconds": args.duration,
                    "measurement_start_monotonic_ns": measurement_start,
                    "measurement_end_monotonic_ns": measurement_end,
                }, indent=2) + "\n",
                encoding="utf-8",
            )
            experiment.stop_run()
            capture.drain(0.5)
            for frontend in frontends:
                frontend.require_running()
            publisher.require_running()

            # Saturation deliberately creates backlog. Reconnecting the MIDAS
            # receiver is the only reliable way to keep that backlog from
            # contaminating the next payload-size scenario.
            if scenario.interval_us == 0 and scenario is not scenarios[-1]:
                publisher.stop(signal.SIGHUP)
                publisher = ManagedProcess(
                    "zmq_publisher",
                    [str(publisher_binary), "--config", str(config_path)],
                    experiment.env,
                    output / "logs",
                )
                capture.replace_process("publisher", publisher.pid)
                time.sleep(1.0)
                publisher.require_running()

                experiment.configure(prime, maximum_events=150)
                experiment.start_run()
                capture.drain(8.0 if args.gaudi_socket else 1.5)
                experiment.stop_run()
                capture.drain(0.5)

        capture.close()
        capture = None
        native_subscriber.stop(signal.SIGINT)
        native_subscriber = None
        materialize_native_events(output, scenarios)
        # MIDAS replaces SIGINT/SIGTERM handlers when it connects. SIGHUP remains
        # owned by zmq_publisher and therefore gives the application a clean shutdown.
        publisher.stop(signal.SIGHUP)
        publisher = None
        for frontend in frontends:
            frontend.stop(signal.SIGTERM)
        frontends.clear()

        report = subprocess.run(
            [sys.executable, str(Path(__file__).with_name("generate_report.py")), str(output)],
            check=True,
            text=True,
        )
        return report.returncode
    finally:
        experiment.stop_run()
        if capture is not None:
            capture.close()
        if publisher is not None:
            publisher.stop(signal.SIGHUP)
        if native_subscriber is not None:
            native_subscriber.stop(signal.SIGINT)
        for frontend in frontends:
            frontend.stop(signal.SIGTERM)
        print(f"Benchmark artifacts: {output}", flush=True)


if __name__ == "__main__":
    raise SystemExit(main())
