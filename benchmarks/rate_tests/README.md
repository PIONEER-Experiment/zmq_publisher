# MIDAS rate tests

This benchmark creates an isolated MIDAS experiment, runs a configurable MIDAS
frontend, passes its events through the MIDAS processor and ZeroMQ transport,
and records rates and latency at every observable stage.

Generated experiment state, CSV data, logs, plots, and reports are written to
the ignored `.artifacts/rate-tests/latest/` directory. Each default run replaces
the previous `latest` result, avoiding ambiguous timestamped test directories.
Use `--output PATH` for a deliberately named run; an existing named output is
only replaced when `--replace-output` is supplied.

## Build

```sh
export MIDASSYS=/home/jack/software/midas
cmake -S . -B build-rate-tests \
  -DZMQ_PUBLISHER_BUILD_MIDAS_EXTENSION=ON \
  -DZMQ_PUBLISHER_BUILD_RATE_TESTS=ON \
  -DZMQ_PUBLISHER_BUILD_TESTS=OFF
cmake --build build-rate-tests --parallel
```

## Run

```sh
python3 benchmarks/rate_tests/run_rate_tests.py --duration 4
```

Required Python packages are `pyzmq`, `psutil`, `pandas`, `numpy`, `matplotlib`,
and `tabulate`. A run produces `benchmark_report.pdf`, `benchmark_report.md`,
`summary.csv`, raw event timing CSVs, telemetry snapshots, resource samples, and
process logs.

```sh
python3 -m pip install -r benchmarks/rate_tests/requirements.txt
```

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

For example, this measures the effect of draining larger batches from the
receiver's consuming queue:

```sh
python3 benchmarks/rate_tests/run_rate_tests.py \
  --events-per-retrieval 512 \
  --scenario 1KiB_1kHz_batch512:1024:1000
```
