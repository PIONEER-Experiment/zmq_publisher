# MIDAS rate tests

This benchmark creates an isolated MIDAS experiment, runs a configurable MIDAS
frontend, passes its events through the MIDAS processor and ZeroMQ transport,
and records rates and latency at every observable stage.

Six frontend selections are available:

- `raw` (default) emits an arbitrary `DATA` bank for transport-only tests;
- `sampic` emits mode-correct SAMPIC data, metadata, timing, and trigger banks;
- `musip` emits normalized `Hddd` MuPix and MuTRIG hit banks;
- `hdsoc` emits valid `HDxx` event and `HTxx` timing banks;
- `wfd5` emits valid `CRxx` AMC/channel/waveform/trailer payloads;
- `all` runs all four detector frontends concurrently into one MIDAS buffer.

The detector simulators are benchmark fixtures, not production DAQ frontends.
Each detector frontend generates an independent event stream and exercises the
same Gaudi unpacker used online. With `--detector all`, the configured rate is
applied to each of the four frontends, so the requested aggregate rate is four
times that value. Every event regenerates deterministic Gaussian electronics
noise and a varying-amplitude, fast-rise/slow-decay pulse. Pulse onsets have
only small event-to-event jitter around each detector's trigger position, so
live views change without the signal wandering across the trace. The MuSiP
fixture applies the same shape to its hit-energy sequence; it remains hit data,
not a fabricated ADC bank. Runs remain reproducible.

The SAMPIC fixture defaults to `time_grouping` (`SD`, `SH`, `ST`, and `SC`).
Select another real collector contract before launching it with:

```sh
export ZMQ_RATE_TEST_SAMPIC_MODE=external_gated_trigger  # SD, SH, ST, SG
# or: default, time_grouping, vendor_passthrough
```

`vendor_passthrough` emits `SD`, `SH`, `ST`, and `SV`. These modes intentionally
do not combine mutually exclusive `SG`, `SV`, and collector-timing contracts in
one synthetic event.

For the interactive four-process MIDAS/publisher/gateway/web demo, use
`run_midas_demo.py` as documented in the top-level README. It stores only the
isolated MIDAS experiment state and frontend logs under `.artifacts/midas-demo/`;
the publisher uses the committed `config/online_demo.json` configuration.

Generated experiment state, CSV data, logs, plots, and reports are written to
the ignored `.artifacts/rate-tests/latest/` directory. Each default run replaces
the previous `latest` result, avoiding ambiguous timestamped test directories.
Use `--output PATH` for a deliberately named run; an existing named output is
only replaced when `--replace-output` is supplied.

## Build

```sh
./setup_env.sh --with-gaudi --with-benchmarks
./scripts/build.sh --with-midas --with-gaudi --with-simulators
```

## Run

```sh
python3 benchmarks/rate_tests/run_rate_tests.py --duration 4
```

`setup_env.sh --with-benchmarks` installs `pyzmq`, `psutil`, `pandas`, `numpy`,
`matplotlib`, and `tabulate` into the ignored project-local `.venv`. It never
modifies the active Conda or system Python environment. A run produces
`benchmark_report.pdf`, `benchmark_report.md`, `summary.csv`, raw event timing
CSVs, telemetry snapshots, resource samples, and process logs.

Custom scenarios use `NAME:PAYLOAD_BYTES:INTERVAL_US`:

```sh
python3 benchmarks/rate_tests/run_rate_tests.py \
  --scenario baseline:1024:1000 \
  --scenario saturation:65536:0
```

An interval of zero asks the MIDAS frontend to generate as quickly as possible.
The MIDAS processor uses event-driven scheduling: it sleeps until the receiver
queue transitions from empty to non-empty, then drains available batches. The
MIDAS receiver's `cm_yield()` timeout is separately
controlled with `--midas-yield-timeout-ms` (default: 1).
Use `--events-per-retrieval` to test the MIDAS processor's retrieval batch size
(default: 128). The consuming receiver queue defaults to 4096 events and can be
changed with `--receiver-buffer-events`.
`--delivery-batch-size N` independently packs N serialized products into one
ZeroMQ multipart message; the default of 1 sends products individually.
The MIDAS read cache defaults to 100000 bytes and can be disabled for a
low-latency comparison with `--midas-read-cache-bytes 0`.
MIDAS enforces a minimum 10,000,000-byte frontend write cache. The simulator
therefore flushes it every 1000 microseconds by default so `latest` measures the
pipeline rather than MIDAS's normal once-per-second flush. Set
`--frontend-flush-interval-us -1` to reproduce the default MIDAS batching
behavior, which can add nearly one second of latency to low-byte-rate streams.

To measure the complete MIDAS-to-Gaudi-to-ZeroMQ path, build with
`ZMQ_PUBLISHER_BUILD_GAUDI_EXTENSION=ON`. The benchmark can have the publisher
launch and own the selected sidecar job:

```sh
python3 benchmarks/rate_tests/run_rate_tests.py \
  --detector hdsoc \
  --gaudi-socket /tmp/pioneer-gaudi-hdsoc.sock \
  --gaudi-launcher extensions/gaudi/scripts/run_sidecar.sh \
  --gaudi-job-options config/gaudi/online_pipeline.py \
  --events-per-retrieval 32 \
  --gaudi-publish-every-events 100 \
  --matrix
```

The single `config/gaudi/online_pipeline.py` job handles every detector
selection. Use `--detector all` for a mixed-buffer integration or rate test.

The MUSIP fixture follows the normalized `Hddd` input expected by
`PITMidasMusip` and emits both MuPix and MuTRIG words. Its source format is
based on the dummy-data path in `musip_daq`. The online adapter installs a
small synthetic `GeoHeader` for the simulator channels before the unchanged
MUSIP unpacker initializes; production jobs should use the real geometry.

`--matrix` runs six logarithmically spaced event sizes from 1 KiB through 1 MiB
at 100, 300, 1,000, and 3,000 Hz plus saturation. The PDF adds heatmaps for
rate, bandwidth, loss/backlog, per-event cost, and each Gaudi algorithm.
Saturation scenarios intentionally overload the input. The harness restarts the
publisher-owned sidecar after each saturation point so queued events cannot
contaminate the next payload size. A capped low-rate priming run initializes
Gaudi and the configured unpackers before measurement.

The maintained detector reports are written to:

```text
.artifacts/rate-tests/detectors/hdsoc/
.artifacts/rate-tests/detectors/wfd5/
.artifacts/rate-tests/detectors/musip/
.artifacts/rate-tests/detectors/mixed/
```

Gaudi reports include a dedicated request breakdown for Unix-socket request
send, MIDAS parsing, the full Gaudi pipeline, compact-product serialization,
unmeasured framework overhead, and response receipt. Client request-send and sidecar receive happen
concurrently and are shown independently rather than summed twice.
Processor telemetry counts all acknowledged Gaudi events, while transport and
subscriber rates count published compact products. Sparse-publication reports
therefore compare the source against processed-event rate and do not treat the
intentional sequence gaps between snapshots as event loss.

For example, this measures the effect of draining larger batches from the
receiver's consuming queue:

```sh
python3 benchmarks/rate_tests/run_rate_tests.py \
  --events-per-retrieval 512 \
  --scenario 1KiB_1kHz_batch512:1024:1000
```
