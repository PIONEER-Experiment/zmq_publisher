# zmq_publisher

`zmq_publisher` schedules configurable processors and publishes their products on
ZeroMQ PUB sockets. The core has no dependency on MIDAS, an unpacker, ROOT, or
an analysis framework. Domain-specific input and processing live in runtime
plugins.

## Build and test

The default build includes the command plugin and does not require MIDAS:

```sh
./setup_env.sh
./scripts/build.sh --overwrite
ctest --test-dir build --output-on-failure
```

Run the example configuration with:

```sh
./scripts/run.sh -- --config config/config.json
```

To include the optional MIDAS input plugin:

```sh
./setup_env.sh --with-midas
./scripts/build.sh --overwrite --with-midas
ctest --test-dir build --output-on-failure
```

MIDAS discovery writes local values to the ignored
`scripts/environment/.env`. Configuration and logger paths remain runtime
arguments and are not compiled into the executable.

## Architecture

- `zmq_publisher_core` owns configuration, channels, scheduling, telemetry, and
  ZeroMQ transport.
- `ProcessorRegistry` maps configuration type names to plugin-owned factories.
- `ProcessorScheduler` runs processors on a bounded worker pool and respects
  each processor's concurrency declaration.
- `Channel` assigns sequence numbers and implements a selected delivery policy.
- `ZmqTransport` shards endpoints across dedicated I/O workers. Each worker owns
  its ZeroMQ context, sockets, and bounded output queue.

Each directory under `extensions/` is a small plugin project with its own
headers, sources, build file, and one registration entry point:

```text
extensions/<plugin>/
├── CMakeLists.txt
├── include/zmq_publisher/extensions/<plugin>/
├── src/
└── register_plugin.cpp
```

Plugins export:

```cpp
extern "C" void zmq_publisher_register_processors(
    zmq_publisher::ProcessorRegistry& registry);
```

The generic application never edits a central processor switch. A plugin owns
its configuration parsing, validation, construction, and capability declaration.

## Processor definitions and custom configuration

Every processor uses one canonical envelope. The core owns `type`, optional
instance `name`, and `schedule`; only the nested `config` object is passed to the
plugin factory:

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

The envelope is strict: all four fields shown above are recognized, `name` is
the only optional field, and unknown fields are rejected. Fixed schedules must
provide exactly one of `interval-ms` or `interval-us`. Continuous schedules may
provide `idle-backoff-us` and do not have an interval.

Each plugin converts its `config` object into a typed configuration class before
constructing the processor. A registration therefore has one obvious shape:

```cpp
registry.register_processor("my-processor", {
    [](const ConfigurationView& value, ProcessorContext& context) {
        const auto config = MyProcessorConfig::from_configuration(value);
        return std::make_unique<MyProcessor>(config, context);
    },
    ProcessorCapabilities{false, true, 1},
});
```

To add a processor, create one configuration class, one processor class, their
source files, and the plugin's `register_plugin.cpp`. No core source or central
factory switch needs to change.

## Processor contract

`Processor::process()` returns a `ProcessResult` in one of three states:

- `produced_output`: one or more immutable `OutputMessage` products are ready;
- `idle`: no product is currently available;
- `stopped`: the processor has permanently completed.

An `OutputMessage` moves an owned byte buffer into immutable shared storage or
aliases immutable storage already owned by a plugin.
Channels, sliding windows, queues, and ZeroMQ frame callbacks then pass or share
that storage without copying its payload. JSON is used only for the small wire
metadata envelope; product bytes are never expanded into or escaped inside JSON.

The optional MIDAS plugin currently publishes each `TMEvent` as raw bytes with
content type `application/x-midas-event` and schema `midas-tmevent-v1`. It does
not unpack banks or depend on an analysis pipeline.

## Scheduling

The supported modes are:

- `fixed-delay`: wait the configured interval after a call completes;
- `fixed-rate`: schedule against stable deadlines;
- `continuous`: immediately redispatch after output is produced.
- `event-driven`: sleep until the processor's input source signals that work is ready.

Continuous mode has an independent idle backoff. Set it to zero to poll as fast
as the processor and worker pool allow, or use a positive value to avoid burning
a CPU while an input source is empty:

```json
"schedule": {
  "mode": "continuous",
  "idle-backoff-us": 0
}
```

Processors are serialized by default. A plugin may declare itself thread-safe,
set a concurrency greater than one, and opt out of ordered output when parallel
calls are safe.

## Delivery and wire format

Each channel selects one explicit delivery policy:

- `each`: publish every product separately;
- `batch`: publish non-overlapping groups of `size` products;
- `latest`: publish only the final product returned by each processor call;
- `sliding-window`: publish after every product with up to the latest `size`
  products. Retained payloads are shared rather than recopied.

```json
"delivery": {
  "mode": "batch",
  "size": 16
}
```

Every publication is a multipart ZeroMQ message:

1. topic frame;
2. UTF-8 JSON metadata frame with protocol, publication sequence, and product
   metadata;
3. one binary frame per product.

The protocol identifier is `zmq-publisher-v1`. Subscribers should use the
metadata `products` array to associate each binary frame with its content type,
schema, product sequence, and byte size.

## Backpressure and transport parallelism

Transport behavior is configured under `general-settings.transport`:

```json
"transport": {
  "worker-threads": 2,
  "zmq-io-threads": 1,
  "queue-capacity": 4096,
  "queue-policy": "block"
}
```

Endpoints are consistently assigned to transport workers so one socket is
always owned by one thread. Queue capacity is per worker. Available policies are
`block`, `drop-newest`, `drop-oldest`, and `fail`. `block` provides backpressure
and preserves data; a drop policy is useful when bounded latency matters more
than completeness.

## Telemetry and rate testing

Telemetry can be logged, and optionally published on a separate PUB endpoint:

```json
"telemetry": {
  "enabled": true,
  "interval-ms": 1000,
  "endpoint": "tcp://127.0.0.1:5556",
  "topic": "METRICS"
}
```

Snapshots contain cumulative invocations, products, bytes, idle calls, failures,
drops, blocked time, processing time, queue latency, current queue depth, and
queue high-water mark for each processor, channel, and transport worker. Rates
can be calculated by differencing consecutive snapshots over the configured
interval.

Build and run the allocation-and-delivery microbenchmark with:

```sh
cmake -S . -B build -DZMQ_PUBLISHER_BUILD_BENCHMARKS=ON
cmake --build build --target zmq_publisher_channel_benchmark --parallel
build/bin/zmq_publisher_channel_benchmark
```

## Out-of-tree plugin SDK

Install the project to expose the `zmq_publisher::plugin_sdk` CMake target:

```sh
cmake --install build --prefix /path/to/prefix
```

An external plugin can then use:

```cmake
find_package(zmq_publisher 0.3 CONFIG REQUIRED)
add_library(my_processor MODULE register_plugin.cpp src/my_processor.cpp)
target_link_libraries(my_processor PRIVATE zmq_publisher::plugin_sdk)
```

## Naming

Files, directories, functions, and variables use `snake_case`; types use
`PascalCase`; private members use a trailing underscore. Public code lives under
the `zmq_publisher` namespace, with one principal class per file.
