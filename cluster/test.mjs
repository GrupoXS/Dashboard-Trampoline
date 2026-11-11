import {test} from 'node:test';
import assert from 'node:assert/strict';
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
