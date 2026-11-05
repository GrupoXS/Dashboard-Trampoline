import {spawn, spawnSync} from 'node:child_process';
import assert from 'node:assert/strict';
import {mkdtemp, rm} from 'node:fs/promises';
import {tmpdir} from 'node:os';
import {join} from 'node:path';
import {fileURLToPath} from 'node:url';
import {createServer} from '../cluster/server.mjs';

const sourceDir = fileURLToPath(new URL('.', import.meta.url));
const repoRoot = fileURLToPath(new URL('..', import.meta.url));

async function run(command, args, options = {}) {
  const child = spawn(command, args, options);
  let stdout = '';
  let stderr = '';
  child.stdout?.setEncoding('utf8');
  child.stderr?.setEncoding('utf8');
  child.stdout?.on('data', chunk => { stdout += chunk; });
  child.stderr?.on('data', chunk => { stderr += chunk; });
  const [code, signal] = await new Promise((resolve, reject) => {
    child.once('error', reject);
    child.once('close', (exitCode, exitSignal) => resolve([exitCode, exitSignal]));
  });
  return {code, signal, stdout, stderr};
}

const temporaryDir = await mkdtemp(join(tmpdir(), 'cluster-telemetry-'));
const binary = join(temporaryDir, 'vehicle-simulator');
const compiler = process.env.CC || 'cc';
const build = spawnSync(compiler, [
  '-std=c11', '-Wall', '-Wextra', '-Werror', '-pedantic',
  join(sourceDir, 'vehicle_simulator.c'),
  join(sourceDir, 'cluster_client.c'),
  '-o', binary
], {encoding: 'utf8'});
assert.equal(build.status, 0, `C build failed:\n${build.stdout}${build.stderr}`);

const server = createServer('integration-secret');
await new Promise((resolve, reject) => {
  server.once('error', reject);
  server.listen(0, '127.0.0.1', resolve);
});
const port = server.address().port;
const abort = new AbortController();
let reader;

try {
  const response = await fetch(`http://127.0.0.1:${port}/api/events`, {signal: abort.signal});
  assert.equal(response.status, 200);
  reader = response.body.getReader();
  await reader.read(); // Initial snapshot.

  const result = await run(binary, ['--once'], {
    cwd: repoRoot,
    env: {...process.env, API_KEY: 'integration-secret', CLUSTER_PORT: String(port)},
    stdio: ['ignore', 'pipe', 'pipe']
  });
  assert.equal(result.code, 0, `C client failed:\n${result.stdout}${result.stderr}`);

  const stateResponse = await fetch(`http://127.0.0.1:${port}/api/state`);
  const state = await stateResponse.json();
  assert.equal(state.sequence, 1);
  assert.equal(state.data.gear, 'D');
  assert.equal(state.data.battery, 76);
  assert.equal(state.data.range, 304);
  assert.equal(state.data.speed, 48);
  assert.equal(state.data.temperature, 25);
  assert.equal(state.data.lights, true);

  const event = new TextDecoder().decode((await reader.read()).value);
  assert.match(event, /"sequence":1/);
  assert.match(event, /"battery":76/);
} finally {
  abort.abort();
  await reader?.cancel().catch(() => {});
  server.closeAllConnections();
  await new Promise(resolve => server.close(resolve));
  await rm(temporaryDir, {recursive: true, force: true});
}

console.log('C telemetry client → Node API → SSE integration passed');
