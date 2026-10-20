#!/usr/bin/env python3

"""Run one isolated MIDAS experiment containing all detector simulators."""

from __future__ import annotations

import argparse
import getpass
import os
from pathlib import Path
import shutil
import signal
import subprocess
import time


EXPERIMENT = "ZMQ_PUBLISHER_DEMO"
DETECTORS = {
    "sampic": "PIONEER SAMPIC Simulator 00",
    "musip": "PIONEER MUSIP Simulator 00",
    "hdsoc": "PIONEER HDSoC Simulator 00",
    "wfd5": "PIONEER WFD5 Simulator 00",
}


class Frontend:
    def __init__(self, name: str, binary: Path, environment: dict[str, str], log_dir: Path):
        self.name = name
        self.log_path = log_dir / f"{name}.log"
        self.log = self.log_path.open("a", encoding="utf-8")
        self.log.write(f"\n--- starting {name} frontend ---\n")
        self.log.flush()
        self.process = subprocess.Popen(
            [str(binary), "-i", "0"],
            env=environment,
            stdout=self.log,
            stderr=subprocess.STDOUT,
            start_new_session=True,
        )

    def require_running(self) -> None:
        status = self.process.poll()
        if status is not None:
            raise RuntimeError(
                f"{self.name} frontend exited with status {status}; see {self.log_path}")

    def stop(self) -> None:
        if self.process.poll() is None:
            os.killpg(self.process.pid, signal.SIGTERM)
            try:
                self.process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                os.killpg(self.process.pid, signal.SIGKILL)
                self.process.wait(timeout=3)
        self.log.close()


class Experiment:
    def __init__(self, midas_root: Path, artifact_dir: Path):
        self.midas_root = midas_root.resolve()
        self.directory = (artifact_dir / "experiment").resolve()
        self.directory.mkdir(parents=True)
        self.exptab = self.directory / "exptab"
        self.exptab.write_text(
            f"{EXPERIMENT} {self.directory} {getpass.getuser()}\n", encoding="utf-8")
        self.environment = os.environ.copy()
        self.environment.update({
            "MIDASSYS": str(self.midas_root),
            "MIDAS_EXPTAB": str(self.exptab),
            "MIDAS_EXPT_NAME": EXPERIMENT,
            "PATH": f"{self.midas_root / 'bin'}:{self.environment.get('PATH', '')}",
            "ZMQ_RATE_TEST_WRITE_CACHE_BYTES": "10000000",
            "ZMQ_RATE_TEST_FLUSH_INTERVAL_US": "1000",
            "ZMQ_RATE_TEST_SAMPIC_DQM_MODE": "1",
        })

    def run(self, program: str, *arguments: str, check: bool = True) -> subprocess.CompletedProcess:
        return subprocess.run(
            [str(self.midas_root / "bin" / program), *arguments],
            env=self.environment,
            cwd=self.directory,
            check=check,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        )

    def odb(self, command: str, check: bool = True) -> subprocess.CompletedProcess:
        return self.run("odbedit", "-e", EXPERIMENT, "-c", command, check=check)

    def initialize(self) -> None:
        # No process from this isolated experiment may survive into a fresh
        # launch. Event buffers are separate POSIX shared-memory objects and
        # odbinit --cleanup only knows how to remove the ODB-related objects.
        prefix = f"{os.getuid()}_{EXPERIMENT}_"
        shared_memory = Path("/dev/shm")
        if shared_memory.is_dir():
            for path in shared_memory.iterdir():
                if path.name.startswith(prefix):
                    path.unlink()

        for attempt in range(3):
            self.run("odbinit", "-e", EXPERIMENT, "--cleanup", check=False)
            for stale in self.directory.glob(".*.SHM"):
                stale.unlink(missing_ok=True)
            for name in (".ODB_SIZE.TXT", ".SHM_HOST.TXT", ".SHM_TYPE.TXT"):
                (self.directory / name).unlink(missing_ok=True)
            self.run("odbinit", "-e", EXPERIMENT, "-s", "64MB")
            snapshot = self.directory / ".ODB.SHM"
            magic = b""
            if snapshot.is_file():
                with snapshot.open("rb") as stream:
                    magic = stream.read(4)
            if magic == b"ODB\0":
                # Under POSIX SHM (notably WSL), letting a frontend race the
                # segment just created by odbinit can expose a zeroed root key.
                # Preserve the initialized snapshot and close that segment;
                # the first frontend then opens it through the normal path.
                self.run("odbinit", "-e", EXPERIMENT, "--cleanup", check=False)
                time.sleep(0.25)
                return
            time.sleep(0.2 * (attempt + 1))
        raise RuntimeError("Could not initialize a valid MIDAS ODB after three attempts")

    def wait_for(self, equipment: str, frontend: Frontend) -> None:
        deadline = time.monotonic() + 15.0
        while time.monotonic() < deadline:
            frontend.require_running()
            result = self.odb(f'ls "/Equipment/{equipment}/Settings"', check=False)
            if result.returncode == 0 and "Payload Bytes" in result.stdout:
                return
            time.sleep(0.2)
        raise RuntimeError(f"MIDAS equipment did not appear: {equipment}")

    def configure(self, payload_bytes: int, rate_hz: float) -> None:
        interval_us = round(1_000_000 / rate_hz)
        for equipment in DETECTORS.values():
            settings = f"/Equipment/{equipment}/Settings"
            self.odb(f'set "{settings}/Payload Bytes" {payload_bytes}')
            self.odb(f'set "{settings}/Polling Interval (us)" {interval_us}')
            self.odb(f'set "{settings}/Maximum Events" 0')

    def stop_run(self) -> None:
        self.run("mtransition", "-e", EXPERIMENT, "STOP", check=False)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, default=Path("build"))
    parser.add_argument(
        "--midas-root", type=Path,
        default=Path(os.environ.get("MIDASSYS", "/home/jack/software/midas")))
    parser.add_argument("--payload-bytes", type=int, default=16 * 1024)
    parser.add_argument("--rate-hz", type=float, default=100.0,
                        help="event rate for each of the four frontends")
    return parser.parse_args()


def demo_processes() -> list[tuple[int, str]]:
    marker = f"MIDAS_EXPT_NAME={EXPERIMENT}".encode()
    tracked_commands = (
        "zmq_publisher_", "/zmq_publisher", "gaudirun.py", "run_sidecar.sh",
        "/mhttpd", "/mserver",
    )
    result = []
    for process in Path("/proc").iterdir():
        if not process.name.isdigit() or int(process.name) == os.getpid():
            continue
        try:
            environment = (process / "environ").read_bytes().split(b"\0")
        except (FileNotFoundError, PermissionError, ProcessLookupError):
            continue
        if marker in environment:
            try:
                command = (process / "cmdline").read_bytes().replace(b"\0", b" ").decode().strip()
            except (FileNotFoundError, PermissionError, ProcessLookupError, UnicodeDecodeError):
                command = "unknown command"
            if not any(marker in command for marker in tracked_commands):
                continue
            result.append((int(process.name), command))
    return result


def start_frontend(
    detector: str,
    equipment: str,
    binary: Path,
    experiment: Experiment,
    log_dir: Path,
) -> Frontend:
    last_error: RuntimeError | None = None
    for attempt in range(3):
        frontend = Frontend(detector, binary, experiment.environment, log_dir)
        try:
            experiment.wait_for(equipment, frontend)
            frontend.require_running()
            return frontend
        except RuntimeError as error:
            last_error = error
            frontend.stop()
            if attempt != 2:
                time.sleep(0.5 * (attempt + 1))
    raise RuntimeError(
        f"Could not start {detector} frontend after three attempts: {last_error}")


def main() -> int:
    args = parse_args()
    if args.payload_bytes <= 0 or args.rate_hz <= 0:
        raise ValueError("payload bytes and rate must be positive")

    project = Path(__file__).resolve().parents[2]
    build = (project / args.build_dir).resolve()
    artifact = project / ".artifacts/midas-demo"
    active_processes = demo_processes()
    if active_processes:
        raise RuntimeError(
            "The ZMQ_PUBLISHER_DEMO experiment already has active processes. "
            "Stop the existing publisher/demo before replacing its MIDAS state:\n  " +
            "\n  ".join(f"{pid}: {command}" for pid, command in active_processes))
    if artifact.exists():
        shutil.rmtree(artifact)
    (artifact / "logs").mkdir(parents=True)

    binaries = {
        detector: build / f"bin/zmq_publisher_{detector}_simulator_frontend"
        for detector in DETECTORS
    }
    for binary in binaries.values():
        if not binary.is_file():
            raise RuntimeError(f"Missing simulator binary: {binary}")
    extension = build / "lib/libzmq_publisher_gaudi_extension.so"
    if not extension.is_file():
        raise RuntimeError(f"Missing Gaudi extension: {extension}")

    experiment = Experiment(args.midas_root, artifact)
    experiment.initialize()
    frontends: list[Frontend] = []
    try:
        for detector, equipment in DETECTORS.items():
            frontend = start_frontend(
                detector,
                equipment,
                binaries[detector],
                experiment,
                artifact / "logs",
            )
            frontends.append(frontend)

        experiment.configure(args.payload_bytes, args.rate_hz)
        experiment.run("mtransition", "-e", EXPERIMENT, "START")
        print(f"MIDAS experiment {EXPERIMENT} is running", flush=True)
        print(f"  four frontend rates: {args.rate_hz:g} Hz each", flush=True)
        print(f"  requested payload: {args.payload_bytes} bytes each", flush=True)
        print("Start the publisher in another terminal:", flush=True)
        print(
            "  ./scripts/run.sh --config config/online_demo.json "
            f"--experiment {EXPERIMENT} --exptab {experiment.exptab}",
            flush=True,
        )
        print("Press Ctrl-C here to stop acquisition and all frontends.", flush=True)
        while True:
            time.sleep(1.0)
            for frontend in frontends:
                frontend.require_running()
    except KeyboardInterrupt:
        return 0
    finally:
        experiment.stop_run()
        for frontend in reversed(frontends):
            frontend.stop()


if __name__ == "__main__":
    raise SystemExit(main())
