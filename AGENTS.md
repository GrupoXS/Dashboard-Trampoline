# Repository notes

- This repo has two independently tested parts: `cluster/` (Node.js API + VOLT browser UI) and `rtos/` (C POSIX telemetry producer). There is no root package/workspace; run commands from the indicated directory.
- Requires Node.js 20+. The cluster has no npm dependencies: `cd cluster && npm test` runs its Node test suite; `API_KEY=local-demo-key npm start` starts the dashboard at `http://localhost:3000`.
- `cd rtos && make test` runs the end-to-end C → HTTP API → SSE integration test. It compiles with `CC` (default `cc`) in a temporary directory and needs a C compiler; it starts its own in-process Node server, so no server or API key is needed. `make` builds `build/vehicle-simulator`; `API_KEY=local-demo-key make run` requires the cluster server running. `CLUSTER_PORT` overrides port 3000.
- The current `rtos/` program is a standalone POSIX integration prototype, not a Trampoline application: there is no OIL config or RTOS task setup yet. Its HTTP client is synchronous with a two-second socket timeout; keep network I/O out of critical tasks and shared-resource holds if integrating with Trampoline.
- Telemetry contract and field ranges are enforced in `cluster/server.mjs` and documented in `cluster/README.md`. POST requires `Authorization: Bearer <API_KEY>`; server state is in memory, and GET state/events are unauthenticated. Never put API keys in browser code or versioned files.
- `rtos/build/` is generated output (see `rtos/.gitignore`); don't treat the built simulator as source.
