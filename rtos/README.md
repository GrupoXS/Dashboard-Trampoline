# Trampoline POSIX vehicle dashboard

The directory contains both the original standalone integration probe and a Trampoline POSIX application. The Trampoline application simulates vehicle signals in periodic tasks, runs diagnostics, signals warnings with an OSEK event, and snapshots telemetry for a separate POSIX worker to send to the VOLT cluster API. Blocking HTTP never runs in an RTOS task or while a Trampoline resource is held.

> The Trampoline application is configured in `vehicle_dashboard.oil`. Its build requires a separately installed Trampoline checkout and GOIL; this environment does not currently have `goil`, so the generated RTOS binary cannot be validated here.

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

## Build and run the Trampoline application

Install/build GOIL and the Trampoline POSIX runtime as described in the [official Trampoline documentation](https://github.com/TrampolineRTOS/trampoline). Then, from `rtos/`, provide the absolute path to that checkout:

```sh
make trampoline-build TRAMPOLINE_PATH=/path/to/trampoline
API_KEY=local-demo-key make trampoline-run TRAMPOLINE_PATH=/path/to/trampoline
```

Start the cluster server first. The OIL configuration schedules signal simulation and diagnostics every 100 ms and requests telemetry every second. The worker retains only the newest pending snapshot if network delivery is slower than the producer. `WarningHandlerTask` waits for `WarningEvent`; normal diagnostic checks leave the warning clear, while out-of-range values set the warning and signal the handler.

`make run` remains the original standalone POSIX integration probe; it does not use Trampoline.

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
