# zmq_publisher

`zmq_publisher` runs configurable processors and publishes their output on ZeroMQ
PUB sockets. The core has no dependency on MIDAS, an unpacker, ROOT, or an
analysis pipeline. Domain-specific behavior is provided by processor extension
libraries loaded at runtime.

## Architecture

- `zmq_publisher_core` owns configuration, channels, scheduling, and transport.
- `ProcessorRegistry` maps configuration type names to extension-owned factories.
- `ProcessorScheduler` schedules processors by deadline on a bounded worker pool.
- `ZmqTransport` owns all ZeroMQ sockets on one dedicated I/O thread.
- Extensions parse and validate their own processor configuration.

Each directory under `extensions/` is a small, self-contained plugin project:

```text
extensions/<plugin>/
├── CMakeLists.txt
├── include/zmq_publisher/extensions/<plugin>/
├── src/
└── register_plugin.cpp
```

Each shared extension exports:

```cpp
extern "C" void zmq_publisher_register_processors(
    zmq_publisher::ProcessorRegistry& registry);
```

The command extension is built by default. The MIDAS extension is optional and
publishes raw MIDAS event/bank data; it does not use an analysis pipeline or
unpacker.

## Build

Check the generic build prerequisites, build, and run the Catch2 test suite:

```sh
./scripts/setup_env.sh
./scripts/build.sh --overwrite
ctest --test-dir build --output-on-failure
```

The generic build does not require `MIDASSYS`. `build.sh` builds the command
extension and tests by default. Use `./scripts/build.sh --help` to select a job
count or reuse an existing build directory.

To build the optional MIDAS extension:

```sh
./scripts/setup_env.sh --with-midas
./scripts/build.sh --overwrite --with-midas
ctest --test-dir build --output-on-failure
```

The setup script writes discovered MIDAS values to the ignored
`scripts/environment/.env`; the build script sources it automatically for a
MIDAS build.

## Run

```sh
build/bin/zmq_publisher --config config/config.json
```

Or use the repository script:

```sh
./scripts/run.sh -- --config config/config.json
```

Configuration and logger paths are runtime arguments; they are not derived
from compiled source paths.

## Tests

Tests use Catch2, fetched at configure time through CPM and pinned to a release.
CTest discovers individual test cases, so failures report the named behavior
instead of terminating at a bare assertion.

## Processor configuration

The core recognizes only processor identity and scheduling:

```json
{
  "type": "command",
  "schedule": {
    "mode": "fixed-delay",
    "interval-ms": 1000
  },
  "config": {
    "command": ["/bin/date", "--iso-8601=seconds"]
  }
}
```

Supported schedule modes are `fixed-delay`, `fixed-rate`, and `continuous`.
Continuous processors are rate-limited internally to avoid a zero-delay busy
loop. A processor is serialized by default; extensions must explicitly declare
thread safety before concurrent calls are permitted.

For backward compatibility, `processor` may be used instead of `type`,
`period-ms` may be used instead of `schedule`, and the command extension accepts
a command string through `/bin/sh -c`. Argument arrays are preferred.

Channel configuration uses `endpoint`, `topic`, and `buffer-capacity`. The
legacy keys `zmq-address`, `name`, and `num-events-in-circular-buffer` remain
accepted so existing deployments can migrate incrementally.

## Naming

Files, directories, functions, and variables use `snake_case`; types use
`PascalCase`; private data members use a trailing underscore. Public code lives
under the `zmq_publisher` namespace, with one principal class per file.
