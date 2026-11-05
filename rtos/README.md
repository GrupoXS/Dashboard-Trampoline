# POSIX telemetry prototype

This is the first integration slice between a C telemetry producer and the existing VOLT cluster API. It simulates electric-vehicle signals and sends the cluster's current JSON schema to `POST /api/telemetry`; the Node server then forwards updates to the browser through SSE.

> This standalone POSIX simulator is an integration probe, not yet a Trampoline RTOS application. The local environment does not currently have `goil`; OIL configuration and Trampoline tasks are the next implementation phase after validating the official POSIX toolchain.

## Requirements

- POSIX environment (Linux, macOS, or WSL2)
- C compiler (`cc` or `gcc`)
- Node.js 20 or newer for the integration test and cluster server

## Run the end-to-end integration test

From this directory:

```sh
make test
```

The test compiles the C producer into a temporary directory, starts the existing Node server on an ephemeral loopback port, sends one authenticated telemetry snapshot, and verifies both the API state and SSE event. No API key or running server is required.

## Run the simulator against the browser dashboard

Start the cluster in another terminal with a known local key:

```sh
cd ../cluster
API_KEY=local-demo-key npm start
```

Then, from `rtos/`, build and run the continuous simulator:

```sh
make
API_KEY=local-demo-key make run
```

The C process posts once per second to `127.0.0.1:3000`. Open <http://localhost:3000> and stop the simulator with `Ctrl+C`.

Send a single sample instead:

```sh
API_KEY=local-demo-key CLUSTER_PORT=3000 ./build/vehicle-simulator --once
```

`CLUSTER_PORT` can be set for a non-default server port. The client always targets loopback; it does not expose a telemetry listener. The API key is read from the environment, never embedded in the binary or page.

## Timing and RTOS boundary

The HTTP client uses a two-second socket timeout and is synchronous. Do **not** call it from a time-critical task or while holding a shared-data resource. For Trampoline integration, keep the periodic simulation/diagnostic tasks isolated and place network I/O in a low-priority, non-critical path (or a POSIX worker outside the RTOS scheduler). The HTTP prototype validates the UI/API contract; its timings do not establish hard-real-time behavior.

The sample currently sends: `speed` (km/h), `power` (kW), `battery` (%), `range` (km), `gear`, `temperature` (°C), `odometer` (km), and boolean `left`, `right`, `lights`, `seatbelt`, `warning`. Ranges and types are validated by the Node API.
