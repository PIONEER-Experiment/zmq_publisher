# zmq_publisher

`zmq_publisher` runs configurable processors and publishes their products on
ZeroMQ PUB sockets. The core has no dependency on MIDAS, an unpacker, ROOT, or
an analysis framework; those integrations are runtime plugins.

## Quick start

The default build includes the command plugin and requires only CMake, a C++17
compiler, and ZeroMQ/cppzmq:

```sh
./scripts/setup_env.sh
./scripts/build.sh --overwrite
ctest --test-dir build --output-on-failure
./scripts/run.sh --config config/config.json
```

Optional integrations are selected explicitly:

```sh
# Raw MIDAS events
./scripts/setup_env.sh --with-midas
./scripts/build.sh --overwrite --with-midas

# MIDAS events processed by the PIONEER Gaudi sidecar
./scripts/setup_env.sh --with-gaudi
./scripts/build.sh --overwrite --with-gaudi

# Full local DQM demo, including detector simulator frontends
./scripts/build.sh --overwrite --with-gaudi --with-simulators
```

`scripts/setup_env.sh` never searches the filesystem or guesses an experiment. On its
first run it copies the committed
[`environment/.env.example`](scripts/environment/.env.example) to the
gitignored `scripts/environment/.env.local`; edit that local file with the explicit
paths for the machine, then rerun validation. The optional benchmark/reporting
Python environment is similarly explicit:

```sh
./scripts/setup_env.sh --with-gaudi --with-benchmarks
source scripts/environment/.env.local
source .venv/bin/activate
```

For an already-built checkout, `run.sh` can perform the same validation before
launching. This keeps setup and execution to one command without hiding which
optional toolchain is required:

```sh
./scripts/run.sh --setup generic
./scripts/run.sh --setup gaudi --config config/examples/online_demo.json
```

The C++ build does not require the Python environment. See
[the Gaudi extension guide](extensions/gaudi/README.md) for its container,
sidecar, and run instructions.

Runtime examples are kept under `config/`:

- [`config.json`](config/config.json): generic command processor;
- [`midas.json`](config/examples/midas.json): raw MIDAS
  publication;
- [`gaudi.json`](config/examples/gaudi.json):
  MIDAS-to-Gaudi processing;
- [`online_demo.json`](config/examples/online_demo.json): full local detector demo;
- [`gaudi/`](config/gaudi/): publisher-owned Gaudi job options.

## Design

- `zmq_publisher_core` owns configuration, channels, scheduling, telemetry, and
  ZeroMQ transport.
- `ProcessorRegistry` maps configured type names to plugin-owned factories.
- `ProcessorScheduler` executes processors on a bounded worker pool.
- `Channel` applies a delivery policy and assigns publication sequence numbers.
- `ZmqTransport` shards endpoints across dedicated I/O workers with bounded
  queues.

Each extension is a small plugin project:

```text
extensions/<plugin>/
├── CMakeLists.txt
├── include/zmq_publisher/extensions/<plugin>/
├── src/
└── register_plugin.cpp
```

It exports one registration entry point:

```cpp
extern "C" void zmq_publisher_register_processors(
    zmq_publisher::ProcessorRegistry& registry);
```

The plugin owns its typed configuration, validation, processor construction,
and capability declaration. Adding a processor does not require a core change
or another central factory switch.

## Processor configuration

Every processor uses the same strict envelope. The core consumes `type`, the
optional `name`, and `schedule`; only `config` is passed to the plugin factory:

```json
{
  "type": "command",
  "name": "clock",
  "schedule": {
    "mode": "fixed-delay",
    "interval-ms": 1000
  },
  "config": {
    "command": ["/bin/date", "--iso-8601=seconds"]
  }
}
```

Supported schedules are:

- `fixed-delay`: wait the interval after each call completes;
- `fixed-rate`: schedule against stable deadlines;
- `continuous`: immediately redispatch, optionally using `idle-backoff-us` when
  no work is available;
- `event-driven`: sleep until the processor's input source signals work.

Processors are serialized by default. A plugin may declare itself thread-safe,
allow multiple concurrent calls, and disable ordered output when that is safe.

`Processor::process()` returns `produced_output`, `idle`, or `stopped`.
Products are immutable byte buffers: owned data is moved into shared storage,
and plugin-owned immutable storage may be aliased. Channels and transport share
those buffers without copying their payloads. JSON is used only for the small
wire metadata envelope.

The MIDAS plugin publishes raw `TMEvent` bytes as
`application/x-midas-event`/`midas-tmevent-v1`; it does not unpack banks. The
Gaudi plugin delegates unpacking and analysis to the sidecar.

## Delivery and wire format

A channel uses one delivery policy:

- `each`: publish every product;
- `batch`: publish non-overlapping groups of a configured size;
- `latest`: publish only the last product returned by a processor call;
- `sliding-window`: publish the latest configured number of products after each
  new product.

Every `zmq-publisher-v1` publication is a multipart ZeroMQ message:

1. topic;
2. UTF-8 JSON metadata containing publication and product metadata;
3. one binary frame per product.

Subscribers use the metadata `products` array to associate frames with their
content type, schema, sequence, and byte size.

## Backpressure and telemetry

Transport settings live under `general-settings.transport`. Endpoints are
consistently assigned to workers so each socket remains single-thread-owned.
Queue capacity is per worker; policies are `block`, `drop-newest`,
`drop-oldest`, and `fail`. Use `block` to preserve data, or a drop policy when
bounded latency matters more than completeness.

Telemetry can be logged and optionally published on a separate PUB endpoint.
Snapshots report processor, channel, and transport-worker counters including
invocations, products, bytes, idle calls, failures, drops, blocked time,
processing time, queue latency, depth, and high-water mark. Rates are obtained
by differencing consecutive snapshots.

The end-to-end MIDAS benchmark and report workflow is documented in
[benchmarks/rate_tests/README.md](benchmarks/rate_tests/README.md).

## Full DQM demo

The interactive demo runs as four independently restartable processes: one
MIDAS supervisor (four randomized detector frontends plus acquisition), this
publisher and its Gaudi sidecar, the C++ DQM gateway, and the React development
server. Build the publisher fixtures and online Gaudi module once:

```sh
./scripts/setup_env.sh --with-gaudi
./scripts/build.sh --overwrite --with-gaudi --with-simulators

# Only needed when the image is absent.
apptainer pull "$PIONEER_GAUDI_IMAGE" \
  docker://ghcr.io/pioneer-experiment/pioneer:latest-x86_64
./extensions/gaudi/scripts/build_sidecar.sh
```

Then use one terminal per process:

```sh
# Terminal 0: owns the isolated experiment, all frontends, and acquisition.
python3 benchmarks/rate_tests/run_midas_demo.py

# Terminal 1: ordinary publisher build with the committed demo configuration.
./scripts/run.sh --config config/examples/online_demo.json \
  --experiment ZMQ_PUBLISHER_DEMO \
  --exptab .artifacts/midas-demo/experiment/exptab

# Terminal 2
cd /home/jack/cpp_projects/dqm_gateway
./scripts/run.sh

# Terminal 3
cd /home/jack/large_projects/DQM_webpage/frontend
npm start
```

Open `http://localhost:3000`. The gateway API is at
`http://127.0.0.1:8000/api/v1/catalog`. Ctrl-C in terminal 0 stops acquisition
and all four child frontends. Only generated MIDAS state and logs are kept in
the ignored `.artifacts/midas-demo/` directory and replaced on the next run;
the executable configuration remains versioned under `config/`.

The online analysis stages are selected in `config/gaudi/online_pipeline.py`.
`PulseIntegrals` baseline-corrects and integrates sampled detector traces, then
calibrates each histogram range from `RangeCalibrationEvents` detector events
using `RangeSigmas` standard deviations. A histogram is intentionally absent
until its calibration window is complete. MuSiP publishes timed hit energies
through the separate `/api/v1/hit-series` gateway view rather than pretending
that its hits are waveform samples.

## Out-of-tree plugins

Install the project to export the `zmq_publisher::plugin_sdk` CMake target:

```sh
cmake --install build --prefix /path/to/prefix
```

```cmake
find_package(zmq_publisher 0.3 CONFIG REQUIRED)
add_library(my_processor MODULE register_plugin.cpp src/my_processor.cpp)
target_link_libraries(my_processor PRIVATE zmq_publisher::plugin_sdk)
```

Files, directories, functions, and variables use `snake_case`; types use
`PascalCase`; private members use a trailing underscore. Public code lives in
the `zmq_publisher` namespace, with one principal class per file.
