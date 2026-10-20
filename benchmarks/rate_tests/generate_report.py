#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.backends.backend_pdf import PdfPages
import numpy as np
import pandas as pd


PROCESSOR = "processor/midas-events/midas-source"
TRANSPORT = "transport/worker-0"


def component_delta(documents: list[dict], name: str) -> tuple[dict, float]:
    samples = []
    for document in documents:
        component = next(
            (entry for entry in document.get("components", []) if entry["component"] == name),
            None,
        )
        if component is not None:
            samples.append((document["captured-monotonic-ns"], component))
    if len(samples) < 2:
        return {}, 0.0
    elapsed = (samples[-1][0] - samples[0][0]) / 1e9
    delta = {
        key: samples[-1][1].get(key, 0) - samples[0][1].get(key, 0)
        for key in samples[-1][1]
        if key != "component" and isinstance(samples[-1][1].get(key), (int, float))
    }
    return delta, elapsed


def percentile(series: pd.Series, value: float) -> float:
    return float(np.percentile(series.to_numpy(), value)) if not series.empty else float("nan")


def observed_counter_delta(events: pd.DataFrame, column: str) -> float:
    values = events[column].dropna()
    return float(values.iloc[-1] - values.iloc[0]) if len(values) >= 2 else 0.0


def summarize(run_dir: Path) -> pd.DataFrame:
    run_metadata = json.loads((run_dir / "run.json").read_text(encoding="utf-8"))
    scenario_order = {
        scenario["name"]: index
        for index, scenario in enumerate(run_metadata["scenarios"])
    }
    rows = []
    publication_every = int(run_metadata.get("gaudi_publish_every_events", 1))
    frontend_count = int(run_metadata.get("frontend_count", 1))
    for scenario_file in sorted(run_dir.glob("*/scenario.json")):
        scenario_dir = scenario_file.parent
        scenario = json.loads(scenario_file.read_text(encoding="utf-8"))
        events = pd.read_csv(scenario_dir / "events.csv")
        documents = [
            json.loads(line)
            for line in (scenario_dir / "telemetry.jsonl").read_text(encoding="utf-8").splitlines()
            if line
        ]
        resources = pd.read_csv(scenario_dir / "resources.csv")
        processor, processor_seconds = component_delta(documents, PROCESSOR)
        transport, transport_seconds = component_delta(documents, TRANSPORT)
        measurement_seconds = float(scenario.get(
            "measurement_duration_seconds",
            run_metadata["duration_seconds"],
        ))

        if events.empty:
            producer_rate = subscriber_rate = expected = received = 0.0
            source_sequence_span = missing_sequences = 0.0
            latency = pd.DataFrame()
        else:
            source_sequence_span = float(
                events["source_sequence"].max() - events["source_sequence"].min())
            received = float(len(events))
            missing_sequences = max(source_sequence_span + 1.0 - received, 0.0)
            producer_times = events["producer_monotonic_ns"].dropna()
            if len(producer_times) >= 2:
                producer_span = (producer_times.iloc[-1] - producer_times.iloc[0]) / 1e9
                producer_rate = (
                    source_sequence_span / producer_span if producer_span > 0 else 0.0)
                expected = producer_rate * measurement_seconds
                end_to_end_start = events["producer_monotonic_ns"]
            else:
                # Serialized analysis products no longer contain the source's
                # benchmark-only RTIM bank. Sequence numbers still provide an
                # exact loss/rate measurement, and callback time bounds latency.
                expected = source_sequence_span + 1.0
                producer_rate = expected / measurement_seconds
                end_to_end_start = events["receiver_monotonic_ns"]
            subscriber_rate = received / measurement_seconds
            latency = pd.DataFrame({
                "producer_to_receiver_us": (
                    events["receiver_monotonic_ns"] - events["producer_monotonic_ns"]
                ) / 1000.0,
                "receiver_queue_us": (
                    events["processor_dequeued_ns"] - events["receiver_monotonic_ns"]
                ) / 1000.0,
                "processor_handoff_us": (
                    events["product_created_ns"] - events["processor_dequeued_ns"]
                ) / 1000.0,
                "processor_to_transport_us": (
                    events["published_ns"] - events["product_created_ns"]
                ) / 1000.0,
                "transport_to_subscriber_us": (
                    events["received_monotonic_ns"] - events["published_ns"]
                ) / 1000.0,
                "end_to_end_us": (
                    events["received_monotonic_ns"] - end_to_end_start
                ) / 1000.0,
            }).clip(lower=0)

        publisher_resources = resources[resources["process"] == "publisher"]
        frontend_resources = resources[
            resources["process"].str.endswith("frontend")
        ]
        receiver_received = observed_counter_delta(events, "receiver_events_received")
        receiver_consumed = observed_counter_delta(events, "receiver_events_consumed")
        receiver_dropped = observed_counter_delta(events, "receiver_events_dropped")
        receiver_queue_latency = observed_counter_delta(
            events, "receiver_total_queue_latency_ns")
        processor_rate = processor.get("items", 0) / processor_seconds if processor_seconds else 0
        transport_rate = transport.get("items", 0) / transport_seconds if transport_seconds else 0
        if (run_metadata.get("gaudi_socket") and publication_every > 1 and
                scenario.get("interval_us", 0) > 0):
            # Sparse snapshots provide too few sequence endpoints for a stable
            # source-rate estimate. The benchmark frontend is configured with
            # this exact fixed interval, so use its requested rate.
            producer_rate = frontend_count * 1_000_000.0 / scenario["interval_us"]
            expected = producer_rate * measurement_seconds
        row = {
            **scenario,
            "expected_source_events": expected,
            "received_events": received,
            "missing_source_sequences": missing_sequences,
            "sequence_gap_percent": (
                100.0 * missing_sequences / (source_sequence_span + 1.0)
                if source_sequence_span else 0.0
            ),
            "receiver_events_received": receiver_received,
            "receiver_events_consumed": receiver_consumed,
            "receiver_events_dropped": receiver_dropped,
            "receiver_drop_percent": (
                100.0 * receiver_dropped / receiver_received
                if receiver_received else 0.0
            ),
            "receiver_queue_high_watermark": events[
                "receiver_queue_high_watermark"].max(),
            "receiver_batch_queue_depth_max": events[
                "receiver_batch_queue_depth"].max(),
            "receiver_queue_depth_max": events["receiver_queue_depth"].max(),
            "receiver_queue_mean_us": (
                receiver_queue_latency / receiver_consumed / 1000.0
                if receiver_consumed else 0.0
            ),
            "pipeline_gap_percent": (
                100.0 * max(expected - received, 0) / expected if expected else 100.0
            ),
            "producer_events_per_s": producer_rate,
            "processor_events_per_s": processor_rate,
            "transport_events_per_s": transport_rate,
            "subscriber_events_per_s": subscriber_rate,
            "producer_payload_MB_per_s": producer_rate * scenario["payload_bytes"] / 1e6,
            "processor_payload_MB_per_s": (
                processor_rate * scenario["payload_bytes"] / 1e6
            ),
            "subscriber_payload_MB_per_s": (
                events["product_bytes"].sum() / measurement_seconds / 1e6
                if not events.empty else 0.0
            ),
            "processor_mean_us": (
                processor.get("processing-ns", 0) /
                max(processor.get("items", 0), 1) / 1000.0
            ),
            "transport_queue_mean_us": (
                transport.get("latency-ns", 0) /
                max(transport.get("invocations", 0), 1) / 1000.0
            ),
            "transport_dropped": transport.get("dropped", 0),
            "transport_blocked_ms": transport.get("blocked-ns", 0) / 1e6,
            "publisher_cpu_mean_percent": publisher_resources["cpu_percent"].mean(),
            "publisher_cpu_max_percent": publisher_resources["cpu_percent"].max(),
            "publisher_rss_MiB": publisher_resources["rss_bytes"].max() / 2**20,
            "frontend_cpu_mean_percent": frontend_resources["cpu_percent"].mean(),
        }
        if run_metadata.get("gaudi_socket") and publication_every > 1:
            row["pipeline_gap_percent"] = (
                100.0 * max(producer_rate - processor_rate, 0) / producer_rate
                if producer_rate else 0.0
            )
            row["sequence_gap_percent"] = float("nan")
        if frontend_count > 1:
            # Each independent frontend has its own valid serial/sequence
            # namespace. Interleaved source_sequence values cannot be treated
            # as one monotonic loss counter.
            row["missing_source_sequences"] = float("nan")
            row["sequence_gap_percent"] = float("nan")
        gaudi_timing_columns = {
            "gaudi_client_request_send_ns": "gaudi_request_send",
            "gaudi_client_response_wait_ns": "gaudi_response_wait",
            "gaudi_client_response_receive_ns": "gaudi_response_receive",
            "gaudi_client_round_trip_ns": "gaudi_round_trip",
            "gaudi_sidecar_socket_receive_ns": "gaudi_socket_receive",
            "gaudi_sidecar_midas_parse_ns": "gaudi_midas_parse",
            "gaudi_sidecar_pipeline_ns": "gaudi_pipeline",
            "gaudi_sidecar_serialization_ns": "gaudi_serialization",
        }
        for source_column, output_column in gaudi_timing_columns.items():
            values = events.get(source_column, pd.Series(dtype=float)).dropna() / 1000.0
            for rank in (50, 95, 99):
                row[f"{output_column}_p{rank}_us"] = percentile(values, rank)
        algorithm_samples: dict[str, list[float]] = {}
        for encoded in events.get(
                "gaudi_algorithm_timing_json", pd.Series(dtype=str)).dropna():
            for algorithm, nanoseconds in json.loads(encoded).items():
                algorithm_samples.setdefault(algorithm, []).append(nanoseconds / 1000.0)
        for algorithm, values in algorithm_samples.items():
            samples = pd.Series(values, dtype=float)
            for rank in (50, 95, 99):
                row[f"gaudi_algorithm::{algorithm}::p{rank}_us"] = percentile(samples, rank)
        attributed = (
            events.get("gaudi_sidecar_midas_parse_ns", 0)
            + events.get("gaudi_sidecar_pipeline_ns", 0)
            + events.get("gaudi_sidecar_serialization_ns", 0)
        )
        residual = (
            events.get("gaudi_client_response_wait_ns", pd.Series(dtype=float))
            - attributed
        ).clip(lower=0) / 1000.0
        for rank in (50, 95, 99):
            row[f"gaudi_unattributed_wait_p{rank}_us"] = percentile(residual.dropna(), rank)
        for source_column, output_column in [
            ("producer_to_receiver", "producer_to_receiver"),
            ("receiver_queue", "receiver_queue"),
            ("processor_handoff", "processor_handoff"),
            ("processor_to_transport", "processor_to_transport"),
            ("transport_to_subscriber", "transport_to_subscriber"),
            ("end_to_end", "e2e"),
        ]:
            values = latency.get(f"{source_column}_us", pd.Series(dtype=float))
            for rank in (50, 95, 99):
                row[f"{output_column}_p{rank}_us"] = percentile(values, rank)
        rows.append(row)
    result = pd.DataFrame(rows)
    result["_scenario_order"] = result["name"].map(scenario_order)
    return result.sort_values("_scenario_order").drop(columns="_scenario_order").reset_index(drop=True)


def build_pdf(summary: pd.DataFrame, run_metadata: dict, output: Path) -> None:
    labels = summary["name"].tolist()
    x = np.arange(len(labels))
    with PdfPages(output) as pdf:
        if len(summary) > 12 and summary["payload_bytes"].nunique() > 2:
            payloads = list(dict.fromkeys(summary["payload_bytes"].tolist()))
            intervals = list(dict.fromkeys(summary["interval_us"].tolist()))
            payload_labels = [f"{value / 1024:g} KiB" for value in payloads]
            rate_labels = [
                "saturation" if interval == 0 else f"{1_000_000 / interval:g} Hz"
                for interval in intervals
            ]
            panels = [
                ("processor_events_per_s", "Processed event rate", "events/s", ".0f"),
                ("processor_payload_MB_per_s", "Processed input throughput", "MB/s", ".1f"),
                ("pipeline_gap_percent", "Unprocessed source fraction", "%", ".1f"),
                ("processor_mean_us", "Processor time per event", "µs", ".0f"),
            ]
            fig, axes = plt.subplots(2, 2, figsize=(14, 10), constrained_layout=True)
            for axis, (column, title, unit, value_format) in zip(axes.flat, panels):
                values = summary.pivot(
                    index="payload_bytes", columns="interval_us", values=column
                ).reindex(index=payloads, columns=intervals)
                image = axis.imshow(values.to_numpy(), aspect="auto", cmap="viridis")
                axis.set_xticks(range(len(intervals)), rate_labels, rotation=25, ha="right")
                axis.set_yticks(range(len(payloads)), payload_labels)
                axis.set_title(title)
                for row in range(len(payloads)):
                    for column_index in range(len(intervals)):
                        value = values.iloc[row, column_index]
                        if np.isfinite(value):
                            axis.text(
                                column_index, row, format(value, value_format),
                                ha="center", va="center", fontsize=7, color="white",
                            )
                fig.colorbar(image, ax=axis, label=unit, shrink=0.8)
            detector = run_metadata.get("detector", "raw").upper()
            fig.suptitle(f"{detector} MIDAS → Gaudi benchmark matrix")
            pdf.savefig(fig)
            plt.close(fig)
        else:
            fig, ax = plt.subplots(figsize=(11, 6))
            width = 0.2
            stages = [
                ("producer_events_per_s", "MIDAS generated"),
                ("processor_events_per_s", "processor output"),
                ("transport_events_per_s", "ZMQ sent"),
                ("subscriber_events_per_s", "subscriber received"),
            ]
            for index, (column, label) in enumerate(stages):
                ax.bar(x + (index - 1.5) * width, summary[column], width, label=label)
            ax.set_xticks(x, labels, rotation=20, ha="right")
            ax.set_ylabel("events / second")
            ax.set_title("Event rate at each pipeline stage")
            ax.legend()
            ax.grid(axis="y", alpha=0.25)
            fig.tight_layout()
            pdf.savefig(fig)
            plt.close(fig)

        if "gaudi_round_trip_p50_us" in summary and summary[
                "gaudi_round_trip_p50_us"].notna().any():
            fig, axes = plt.subplots(1, 2, figsize=(13, 6), sharey=True)
            gaudi_stages = [
                ("gaudi_request_send", "client request send"),
                ("gaudi_midas_parse", "MIDAS parse/bank scan"),
                ("gaudi_pipeline", "Gaudi pipeline before publication"),
                ("gaudi_serialization", "compact-product serialization"),
                ("gaudi_unattributed_wait", "Gaudi/framework + unmeasured wait"),
                ("gaudi_response_receive", "client response receive"),
            ]
            for axis, rank in zip(axes, (50, 95)):
                bottoms = np.zeros(len(summary))
                for column, label in gaudi_stages:
                    values = summary[f"{column}_p{rank}_us"].fillna(0).to_numpy()
                    axis.bar(x, values, bottom=bottoms, label=label)
                    bottoms += values
                axis.plot(
                    x, summary[f"gaudi_round_trip_p{rank}_us"], "ko--",
                    label="measured round trip")
                axis.set_xticks(x, labels, rotation=25, ha="right")
                axis.set_ylabel("latency (microseconds)")
                axis.set_title(f"Gaudi request p{rank} breakdown")
                axis.grid(axis="y", alpha=0.25)
            axes[1].legend(fontsize=8)
            fig.tight_layout()
            pdf.savefig(fig)
            plt.close(fig)

        algorithm_columns = [
            column for column in summary.columns
            if column.startswith("gaudi_algorithm::") and column.endswith("::p50_us")
        ]
        for column in algorithm_columns:
            algorithm = column.removeprefix("gaudi_algorithm::").removesuffix("::p50_us")
            fig, ax = plt.subplots(figsize=(12, 6))
            if len(summary) > 12 and summary["payload_bytes"].nunique() > 2:
                payloads = list(dict.fromkeys(summary["payload_bytes"].tolist()))
                intervals = list(dict.fromkeys(summary["interval_us"].tolist()))
                values = summary.pivot(
                    index="payload_bytes", columns="interval_us", values=column
                ).reindex(index=payloads, columns=intervals)
                image = ax.imshow(values.to_numpy(), aspect="auto", cmap="magma")
                ax.set_xticks(
                    range(len(intervals)),
                    ["saturation" if value == 0 else f"{1_000_000 / value:g} Hz"
                     for value in intervals],
                    rotation=25,
                    ha="right",
                )
                ax.set_yticks(
                    range(len(payloads)), [f"{value / 1024:g} KiB" for value in payloads]
                )
                fig.colorbar(image, ax=ax, label="microseconds")
            else:
                ax.bar(x, summary[column].fillna(0))
                ax.set_xticks(x, labels, rotation=25, ha="right")
                ax.set_ylabel("microseconds")
            ax.set_title(f"Gaudi algorithm median execution: {algorithm}")
            fig.tight_layout()
            pdf.savefig(fig)
            plt.close(fig)

        stage_columns = [
            ("producer_to_receiver", "source → receiver"),
            ("receiver_queue", "receiver queue"),
            ("processor_handoff", "processor handoff"),
            ("processor_to_transport", "channel + transport"),
            ("transport_to_subscriber", "ZMQ + subscriber"),
        ]
        fig, axes = plt.subplots(1, 2, figsize=(13, 6), sharey=True)
        for axis, rank in zip(axes, (50, 95)):
            width = 0.16
            for index, (column, label) in enumerate(stage_columns):
                values = summary[f"{column}_p{rank}_us"].fillna(0)
                axis.bar(x + (index - 2) * width, values, width, label=label)
            axis.set_xticks(x, labels, rotation=25, ha="right")
            axis.set_yscale("log")
            axis.set_ylabel("latency (microseconds)")
            axis.set_title(f"Per-stage p{rank} latency")
            axis.grid(axis="y", alpha=0.25)
        axes[1].legend(fontsize=8)
        fig.tight_layout()
        pdf.savefig(fig)
        plt.close(fig)

        fig, axes = plt.subplots(1, 2, figsize=(11, 5))
        axes[0].bar(x - 0.18, summary["producer_payload_MB_per_s"], 0.36, label="generated")
        axes[0].bar(x + 0.18, summary["subscriber_payload_MB_per_s"], 0.36, label="received")
        axes[0].set_xticks(x, labels, rotation=25, ha="right")
        axes[0].set_ylabel("payload MB/s")
        axes[0].set_title("Payload throughput")
        axes[0].legend()
        axes[1].bar(x, summary["pipeline_gap_percent"])
        axes[1].set_xticks(x, labels, rotation=25, ha="right")
        axes[1].set_ylabel("source-rate vs. capture-rate gap (%)")
        axes[1].set_title("Pipeline throughput gap")
        fig.tight_layout()
        pdf.savefig(fig)
        plt.close(fig)

        fig, axes = plt.subplots(1, 3, figsize=(13, 5))
        axes[0].bar(x - 0.18, summary["missing_source_sequences"], 0.36, label="sequence gaps")
        axes[0].bar(x + 0.18, summary["receiver_events_dropped"], 0.36,
                    label="receiver queue drops")
        axes[0].set_title("Observed event loss counters")
        axes[0].set_ylabel("events")
        axes[0].legend(fontsize=8)
        axes[1].bar(x - 0.18, summary["receiver_batch_queue_depth_max"], 0.36,
                    label="batch-start depth")
        axes[1].bar(x + 0.18, summary["receiver_queue_depth_max"], 0.36,
                    label="sampled depth")
        axes[1].set_title("Receiver queue occupancy")
        axes[1].set_ylabel("events")
        axes[1].legend(fontsize=8)
        axes[2].bar(x, summary["receiver_queue_mean_us"])
        axes[2].set_title("Receiver mean queue residence")
        axes[2].set_ylabel("microseconds")
        for axis in axes:
            axis.set_xticks(x, labels, rotation=30, ha="right")
            axis.grid(axis="y", alpha=0.25)
        fig.tight_layout()
        pdf.savefig(fig)
        plt.close(fig)

        fig, ax = plt.subplots(figsize=(11, 6))
        bottoms = np.zeros(len(summary))
        for column, label in [
            ("producer_to_receiver_p50_us", "MIDAS source → receiver callback"),
            ("receiver_queue_p50_us", "receiver queue"),
            ("processor_handoff_p50_us", "processor payload handoff"),
            ("processor_to_transport_p50_us", "channel + transport queue"),
            ("transport_to_subscriber_p50_us", "ZMQ + subscriber"),
        ]:
            values = summary[column].fillna(0).to_numpy()
            ax.bar(x, values, bottom=bottoms, label=label)
            bottoms += values
        ax.plot(x, summary["e2e_p95_us"], "ko--", label="end-to-end p95")
        ax.plot(x, summary["e2e_p99_us"], "r^--", label="end-to-end p99")
        ax.set_xticks(x, labels, rotation=20, ha="right")
        ax.set_ylabel("latency (microseconds)")
        latency_values = np.concatenate([
            bottoms,
            summary["e2e_p95_us"].fillna(0).to_numpy(),
            summary["e2e_p99_us"].fillna(0).to_numpy(),
        ])
        if np.any(latency_values > 0):
            ax.set_yscale("log")
        ax.set_title("Pipeline latency decomposition")
        ax.legend()
        ax.grid(axis="y", alpha=0.25)
        fig.tight_layout()
        pdf.savefig(fig)
        plt.close(fig)

        fig, axes = plt.subplots(1, 2, figsize=(11, 5))
        axes[0].bar(x - 0.18, summary["frontend_cpu_mean_percent"], 0.36, label="frontend")
        axes[0].bar(x + 0.18, summary["publisher_cpu_mean_percent"], 0.36, label="publisher")
        axes[0].set_xticks(x, labels, rotation=25, ha="right")
        axes[0].set_ylabel("mean CPU utilization (%)")
        axes[0].set_title("Process CPU utilization")
        axes[0].legend()
        axes[1].bar(x, summary["publisher_rss_MiB"])
        axes[1].set_xticks(x, labels, rotation=25, ha="right")
        axes[1].set_ylabel("peak RSS (MiB)")
        axes[1].set_title("Publisher memory")
        fig.tight_layout()
        pdf.savefig(fig)
        plt.close(fig)


def write_markdown(summary: pd.DataFrame, run_metadata: dict, output: Path) -> None:
    bottleneck_rows = []
    publication_every = int(run_metadata.get("gaudi_publish_every_events", 1))
    for _, row in summary.iterrows():
        source = row["producer_events_per_s"]
        processor = row["processor_events_per_s"]
        transport = row["transport_events_per_s"]
        subscriber = row["subscriber_events_per_s"]
        sparse_publication = bool(run_metadata.get("gaudi_socket")) and publication_every > 1
        if row["receiver_events_dropped"] > 0:
            finding = "the bounded MIDAS receiver queue dropped events"
        elif source and processor < source * 0.97:
            finding = "MIDAS ingestion / processor is not keeping pace with the source"
        elif not sparse_publication and processor and transport < processor * 0.97:
            finding = "the ZeroMQ transport is not keeping pace with processor output"
        elif sparse_publication:
            finding = (
                f"processing keeps pace and compact products are intentionally published "
                f"every {publication_every} events"
            )
        elif transport and subscriber < transport * 0.97:
            finding = "the native C++ benchmark subscriber is the observed limiter"
        else:
            latency_stages = {
                "MIDAS source → receiver": row["producer_to_receiver_p50_us"],
                "receiver queue": row["receiver_queue_p50_us"],
                "processor handoff": row["processor_handoff_p50_us"],
                "channel + transport": row["processor_to_transport_p50_us"],
                "ZMQ + subscriber": row["transport_to_subscriber_p50_us"],
            }
            latency_stages = {
                name: value for name, value in latency_stages.items() if pd.notna(value)
            }
            dominant_latency = max(latency_stages, key=latency_stages.get)
            finding = (
                "the requested rate is sustained; "
                f"{dominant_latency} dominates median latency "
                f"({latency_stages[dominant_latency]:,.1f} µs)"
            )
        bottleneck_rows.append(
            f"- **{row['name']}**: {finding}; source-to-capture rate gap "
            f"{row['pipeline_gap_percent']:.2f}%, "
            f"p95 latency {row['e2e_p95_us']:,.1f} µs, "
            f"receiver drops {row['receiver_events_dropped']:.0f}, "
            f"transport drops {row['transport_dropped']:.0f}."
        )
    columns = [
        "name",
        "payload_bytes",
        "producer_events_per_s",
        "processor_events_per_s",
        "transport_events_per_s",
        "subscriber_events_per_s",
        "subscriber_payload_MB_per_s",
        "pipeline_gap_percent",
        "sequence_gap_percent",
        "receiver_events_dropped",
        "receiver_batch_queue_depth_max",
        "e2e_p50_us",
        "e2e_p95_us",
        "e2e_p99_us",
    ]
    table = summary[columns].round(2).to_markdown(index=False)
    stage_columns = [
        "name",
        "producer_to_receiver_p50_us",
        "receiver_queue_p50_us",
        "processor_handoff_p50_us",
        "processor_to_transport_p50_us",
        "transport_to_subscriber_p50_us",
    ]
    stage_table = summary[stage_columns].round(2).to_markdown(index=False)
    gaudi_columns = [
        "name",
        "gaudi_request_send_p50_us",
        "gaudi_midas_parse_p50_us",
        "gaudi_pipeline_p50_us",
        "gaudi_serialization_p50_us",
        "gaudi_unattributed_wait_p50_us",
        "gaudi_response_receive_p50_us",
        "gaudi_round_trip_p50_us",
    ]
    gaudi_table = summary[gaudi_columns].round(2).to_markdown(index=False)
    algorithm_rows = []
    for column in summary.columns:
        if not (column.startswith("gaudi_algorithm::") and column.endswith("::p50_us")):
            continue
        algorithm = column.removeprefix("gaudi_algorithm::").removesuffix("::p50_us")
        p95_column = f"gaudi_algorithm::{algorithm}::p95_us"
        for _, row in summary.iterrows():
            if pd.notna(row[column]):
                algorithm_rows.append({
                    "scenario": row["name"],
                    "algorithm": algorithm,
                    "p50_us": row[column],
                    "p95_us": row[p95_column],
                })
    algorithm_table = (
        pd.DataFrame(algorithm_rows).round(2).to_markdown(index=False)
        if algorithm_rows else "No per-algorithm timing samples were reported."
    )
    output.write_text(
        "# MIDAS → ZMQ publisher rate-test report\n\n"
        "For raw MIDAS products, the source timestamp is written immediately before "
        "the MIDAS banks are built. For Gaudi products, which intentionally omit the "
        "benchmark-only bank, sequence numbers measure loss and end-to-end latency "
        "starts at the MIDAS receiver callback. "
        "Additional markers record the MIDAS callback, receiver-queue dequeue, "
        "`OutputMessage` creation, ZeroMQ publication, and receipt by the native C++ "
        "subscriber. Python only orchestrates the run and captures telemetry.\n\n"
        + (f"Gaudi processed-event telemetry is compared with the source while compact "
           f"products are intentionally sampled every {publication_every} events; "
           "sequence gaps between published snapshots are therefore not event loss.\n\n"
           if run_metadata.get("gaudi_socket") and publication_every > 1 else "")
        + f"Publisher revision: `{run_metadata['git_commit']}`"
        f"{' (dirty)' if run_metadata.get('git_dirty') else ''}; "
        f"receiver branch: `{run_metadata.get('receiver_git_branch')}` at "
        f"`{run_metadata.get('receiver_git_commit')}`"
        f"{' (dirty)' if run_metadata.get('receiver_git_dirty') else ''}.\n\n"
        "## Automated bottleneck indications\n\n" + "\n".join(bottleneck_rows) +
        "\n\n## Summary\n\n" + table +
        "\n\n## Median latency by stage (microseconds)\n\n" + stage_table +
        "\n\n## Gaudi request median breakdown (microseconds)\n\n" + gaudi_table +
        "\n\n## Gaudi algorithm timings (microseconds)\n\n" + algorithm_table +
        "\n\nThe subscriber stage includes native C++/libzmq receive and CSV-recording "
        "overhead. A subscriber rate below the transport rate can therefore indicate subscriber saturation, "
        "not publisher loss. The pipeline gap compares the source rate inferred from "
        "embedded source timestamps and sequence numbers with events captured during the "
        "fixed measurement window. It includes backlog and loss, so it deliberately does "
        "not claim that delayed events were permanently dropped.\n",
        encoding="utf-8",
    )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("run_directory", type=Path)
    args = parser.parse_args()
    run_dir = args.run_directory.resolve()
    summary = summarize(run_dir)
    if summary.empty:
        raise RuntimeError(f"No scenario data found in {run_dir}")
    summary.to_csv(run_dir / "summary.csv", index=False)
    (run_dir / "summary.json").write_text(
        json.dumps(summary.replace({np.nan: None}).to_dict(orient="records"), indent=2) + "\n",
        encoding="utf-8",
    )
    run_metadata = json.loads((run_dir / "run.json").read_text(encoding="utf-8"))
    build_pdf(summary, run_metadata, run_dir / "benchmark_report.pdf")
    write_markdown(summary, run_metadata, run_dir / "benchmark_report.md")
    print(run_dir / "benchmark_report.pdf")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
