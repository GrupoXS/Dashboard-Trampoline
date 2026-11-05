import {test} from 'node:test';
import assert from 'node:assert/strict';
import {spawn} from 'node:child_process';
import {createServer as createTcpServer} from 'node:net';
import {fileURLToPath} from 'node:url';
import {createServer} from './server.mjs';
test('HTTP, autenticação, validação e transmissão SSE',async()=>{
 const server=createServer('test-secret');await new Promise(r=>server.listen(0,'127.0.0.1',r));const url=`http://127.0.0.1:${server.address().port}`;const abort=new AbortController();
 try{
 assert.equal((await fetch(url)).status,200);
 const stream=await fetch(url+'/api/events',{signal:abort.signal});const reader=stream.body.getReader();await reader.read();
 const post=(data,key='test-secret')=>fetch(url+'/api/telemetry',{method:'POST',headers:{'Content-Type':'application/json',Authorization:`Bearer ${key}`},body:JSON.stringify(data)});
 assert.equal((await post({speed:88},'wrong')).status,401);
 assert.equal((await post({speed:999})).status,400);
 assert.equal((await post({unknown:true})).status,400);
 const response=await post({speed:88,gear:'D',battery:76});assert.equal(response.status,200);assert.equal((await response.json()).data.range,328);
 const event=new TextDecoder().decode((await reader.read()).value);assert.match(event,/"speed":88/);
 const state=await(await fetch(url+'/api/state')).json();assert.equal(state.sequence,1);assert.equal(state.data.gear,'D');
 }finally{abort.abort();server.closeAllConnections();await new Promise(r=>server.close(r));}
});

test('entrypoint inicia e serve o site',async()=>{
 const probe=createTcpServer();await new Promise(r=>probe.listen(0,'127.0.0.1',r));const port=probe.address().port;await new Promise((r,j)=>probe.close(e=>e?j(e):r()));
 const child=spawn(process.execPath,[fileURLToPath(new URL('./server.mjs',import.meta.url))],{env:{...process.env,HOST:'127.0.0.1',PORT:String(port),API_KEY:'startup-test-secret'},stdio:['ignore','pipe','pipe']});
 let stderr='';child.stderr.setEncoding('utf8');child.stderr.on('data',chunk=>{stderr+=chunk;});
 const url=`http://127.0.0.1:${port}`;
 try{
  let page;
  for(let attempt=0;attempt<50;attempt++){
   if(child.exitCode!==null)throw Error(`Servidor encerrou (${child.exitCode}): ${stderr}`);
   try{page=await fetch(url);break;}catch(error){if(attempt===49)throw error;await new Promise(r=>setTimeout(r,100));}
  }
  assert.equal(page.status,200);assert.match(await page.text(),/VOLT · Cluster elétrico/);
  const app=await fetch(url+'/app.js');assert.equal(app.status,200);assert.match(app.headers.get('content-type'),/javascript/);
  const style=await fetch(url+'/style.css');assert.equal(style.status,200);assert.match(style.headers.get('content-type'),/css/);
 }finally{
  if(child.exitCode===null){child.kill('SIGTERM');await new Promise(r=>child.once('exit',r));}
 }
});
