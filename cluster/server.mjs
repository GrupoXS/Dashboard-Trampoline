import http from 'node:http';
import {readFile} from 'node:fs/promises';
import {randomBytes,timingSafeEqual} from 'node:crypto';
import {resolve} from 'node:path';
import {pathToFileURL} from 'node:url';
export const defaults={speed:0,power:0,battery:82,range:328,gear:'P',temperature:24,odometer:12480,left:false,right:false,lights:false,seatbelt:false,warning:false};
export function validate(input){
 if(!input||typeof input!=='object'||Array.isArray(input)||!Object.keys(input).length)throw Error('Envie um objeto JSON com ao menos um campo.');
 const ranges={speed:[0,260],power:[-100,300],battery:[0,100],range:[0,1000],temperature:[-40,100],odometer:[0,9999999]};
 for(const [key,value] of Object.entries(input)){
 if(!(key in defaults))throw Error(`Campo desconhecido: ${key}`);
 if(ranges[key]){if(typeof value!=='number'||!Number.isFinite(value)||value<ranges[key][0]||value>ranges[key][1])throw Error(`Valor inválido para ${key}`);}
 else if(key==='gear'){if(!['P','R','N','D','B'].includes(value))throw Error('Marcha inválida');}
 else if(typeof value!=='boolean')throw Error(`${key} deve ser booleano`);
 }return input;
}
export function createServer(key){
 let state={...defaults},updatedAt=null,sequence=0;const clients=new Set();
 const snapshot=()=>JSON.stringify({data:state,updatedAt,sequence});
 const server=http.createServer(async(req,res)=>{
 const path=new URL(req.url,'http://localhost').pathname;
 const json=(status,data)=>{res.writeHead(status,{'Content-Type':'application/json','Cache-Control':'no-store'});res.end(JSON.stringify(data));};
 if(path==='/api/state'&&req.method==='GET')return json(200,JSON.parse(snapshot()));
 if(path==='/api/events'&&req.method==='GET'){
 res.writeHead(200,{'Content-Type':'text/event-stream','Cache-Control':'no-cache','Connection':'keep-alive'});res.write(`data: ${snapshot()}\n\n`);clients.add(res);
 const timer=setInterval(()=>res.write(': heartbeat\n\n'),15000);req.on('close',()=>{clearInterval(timer);clients.delete(res);});return;
 }
 if(path==='/api/telemetry'&&req.method==='POST'){
 const supplied=Buffer.from(req.headers.authorization||'');const expected=Buffer.from(`Bearer ${key}`);
 if(supplied.length!==expected.length||!timingSafeEqual(supplied,expected))return json(401,{error:'Chave de API inválida'});
 if(!(req.headers['content-type']||'').toLowerCase().startsWith('application/json'))return json(415,{error:'Use Content-Type: application/json'});
 try{let body='';for await(const chunk of req){body+=chunk;if(Buffer.byteLength(body)>16384){json(413,{error:'Limite de 16 KB'});return;}}
 const patch=validate(JSON.parse(body));state={...state,...patch};updatedAt=new Date().toISOString();sequence++;for(const client of clients)client.write(`data: ${snapshot()}\n\n`);return json(200,{ok:true,...JSON.parse(snapshot())});
 }catch(error){return json(400,{error:error.message});}
 }
 if(req.method==='GET'&&['/','/app.js','/style.css'].includes(path)){
 try{const file=path==='/'?'index.html':path.slice(1);res.writeHead(200,{'Content-Type':file.endsWith('.html')?'text/html; charset=utf-8':file.endsWith('.js')?'text/javascript':'text/css','X-Content-Type-Options':'nosniff'});res.end(await readFile(new URL(`./public/${file}`,import.meta.url)));}catch{res.end();}return;
 }json(404,{error:'Rota não encontrada'});
 });return server;
}
if(process.argv[1]&&import.meta.url===pathToFileURL(resolve(process.argv[1])).href){
 const key=process.env.API_KEY||randomBytes(24).toString('hex');
 createServer(key).listen(Number(process.env.PORT||3000),process.env.HOST||'127.0.0.1',()=>{console.log('Volt Cluster: http://localhost:3000');if(!process.env.API_KEY)console.log(`Chave temporária da API: ${key}`);});
}
