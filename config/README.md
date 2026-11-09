# Configuration

- `config.json` is the small default publisher configuration used by
  `scripts/run.sh` when no `--config` argument is supplied.
- `examples/` contains complete reference configurations for optional MIDAS
  and Gaudi extensions, including the local online DQM demo.
- `gaudi/` contains Gaudi Python job options. These files define the analysis
  and publication pipeline executed by the sidecar.
- `logger/default.json` contains the default logging sinks, format, and level.

Run an example by passing its path explicitly:

```bash
./scripts/run.sh --config config/examples/online_demo.json
```

Use `--logger-config` after `--` to override the executable's default logger
configuration when launching through `scripts/run.sh`:

```bash
./scripts/run.sh -- --logger-config config/logger/default.json
```
