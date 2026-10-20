# MIDAS-to-Gaudi extension

This optional plugin reads immutable MIDAS events and sends them over a
persistent Unix-domain socket to a Gaudi sidecar. Gaudi processes every event
but returns compact online products only when publication is due. The generic
publisher does not link against Gaudi, detector code, or ROOT, and no process or
container is started per event.

## Build

Build the publisher plugin on the host:

```sh
./setup_env.sh --with-gaudi
./scripts/build.sh --overwrite --with-gaudi
ctest --test-dir build --output-on-failure
```

The build uses the sibling `../midas_receiver` checkout when present. Set
`MIDAS_RECEIVER_SOURCE` in `scripts/environment/.env.local` if it lives elsewhere.

The reusable Gaudi components live in `reco_testbeam/online_dqm`; the publisher
owns the build overlay and sidecar launcher. From the publisher checkout:

```sh
./extensions/gaudi/scripts/build_sidecar.sh
```

If the image is not cached yet, create it once with:

```sh
apptainer pull "$PIONEER_GAUDI_IMAGE" \
  docker://ghcr.io/pioneer-experiment/pioneer:latest-x86_64
```

## Run

Normally the publisher owns the sidecar lifecycle through `sidecar.launcher`
and `sidecar.job-options`, so start only the publisher:

```sh
./scripts/run.sh --config config/gaudi_config.example.json
```

For a persistent, manually managed sidecar, omit those two configuration fields
and run:

```sh
./extensions/gaudi/scripts/run_sidecar.sh \
  --job-options config/gaudi/online_pipeline.py
```

The Unix socket defaults to `/tmp/pioneer-gaudi.sock`. Override it with
`PIONEER_GAUDI_SOCKET` for the sidecar and the matching `socket-path` in the
publisher configuration. A Unix socket avoids TCP framing overhead and permits
the host and Apptainer process to share the same kernel buffers.

## Full mixed-detector test

Build the sidecar once from the publisher checkout:

```sh
./extensions/gaudi/scripts/build_sidecar.sh
```

Build the publisher and all four simulator frontends:

```sh
cd /home/jack/cpp_projects/midas_stuff/midas_publisher
./setup_env.sh --with-gaudi --with-benchmarks
./scripts/build.sh --with-midas --with-gaudi --with-simulators
```

In separate terminals, start the gateway and frontend:

```sh
cd /home/jack/cpp_projects/dqm_gateway
./scripts/build.sh
./scripts/run.sh
```

```sh
cd /home/jack/large_projects/DQM_webpage/frontend
npm install
npm start
```

Then run a 60-second mixed-buffer simulation. Each detector requests 100 Hz,
all events are processed by Gaudi, and display products are published every 100
events:

```sh
cd /home/jack/cpp_projects/midas_stuff/midas_publisher
.venv/bin/python benchmarks/rate_tests/run_rate_tests.py \
  --detector all --duration 60 --warmup 1 \
  --scenario live:65536:10000 \
  --data-endpoint tcp://127.0.0.1:5555 \
  --metrics-endpoint tcp://127.0.0.1:5556 \
  --gaudi-socket /tmp/pioneer-gaudi-live.sock \
  --gaudi-launcher extensions/gaudi/scripts/run_sidecar.sh \
  --gaudi-job-options config/gaudi/online_pipeline.py \
  --events-per-retrieval 32 \
  --gaudi-publish-every-events 100 --gaudi-publish-every-ms 0
```

Open `http://127.0.0.1:3000`. The default dashboard requests one channel from
each detector through `/api/v1/waveforms`; its figure settings select another
module, submodule, or channel without increasing the payload sent to the page.
The benchmark PDF and Markdown report replace the prior contents of
`.artifacts/rate-tests/latest/`.

## Event routing and products

`PIMidasDecoder` dispatches banks independently, so detector frontends may write
separate events into one MIDAS buffer without event building:

- SAMPIC: `SDxx`, `STxx`, and `SCxx`;
- MUSIP: `Hddd`;
- HDSoC: `HDxx` and `HTxx`;
- WFD5: `CRxx`.

Missing detector banks are normal. Begin-run events update `PIODBHeader` and
publish one `pioneer-midas-odb-v1` control product; end-run events are only
acknowledged. A requested publication contains bounded `dqm-waveform-v1`
products for detector data present in the current event and, when enabled, one
`dqm-histogram-v1` cumulative histogram product.

The MUSIP benchmark sets `PIONEER_GAUDI_MUSIP_SIMULATION_GEOMETRY=1` to install
minimal synthetic geometry for its generated channels. Do not use this setting
for detector data; production jobs must provide the real `GeoHeader`. The
existing MUSIP unpacker is unchanged.

The processor configuration deliberately separates ingestion from publication:

```json
"processing": {
  "events-per-request": 32,
  "maximum-batch-bytes": 16777216
},
"publication": {
  "every-events": 10000,
  "every-ms": 1000
}
```

`events-per-request` bounds how many events the processor drains from MIDAS in
one scheduler call. The socket protocol then performs one ordered
request/response exchange per event. `every-events` and `every-ms` are
independent publication triggers; zero disables a trigger. Acknowledged events
count as processed work in telemetry even when no DQM product is published.

Product bounds are controlled with `PIONEER_GAUDI_MAX_WAVEFORMS` and
`PIONEER_GAUDI_MAX_SAMPLES_PER_WAVEFORM`. Set
`PIONEER_GAUDI_INCLUDE_HISTOGRAMS=0` to omit histogram snapshots. ROOT output is
intentionally absent from this online path; archival products belong to the
nearline pipeline.

The wire protocol is local, request/response, and versioned by the `PIGA0002`
and `PIGO0002` magic values. A response contains a typed product manifest plus
concatenated binary payloads. One persistent connection preserves event order;
the processor declares concurrency one and uses event-driven wakeup.

Successful products expose nanosecond timing attributes for rate studies:

- `gaudi.client.request-send-ns`, `response-wait-ns`, `response-receive-ns`,
  and `round-trip-ns` are measured by the publisher.
- `gaudi.sidecar.socket-receive-ns`, `midas-parse-ns`, `pipeline-ns`, and
  `serialization-ns` are measured inside the Gaudi process and returned in
  response metadata.
- `gaudi.algorithm.<instance>-ns` is supplied automatically by the Gaudi timing
  auditor for every configured pipeline algorithm.

Client request-send and sidecar socket-receive overlap and must not be added
together. The rate-test report accounts for this and reports remaining
unattributed response wait separately.
