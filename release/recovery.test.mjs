// Actual upstream publisher against a localhost-only owned API fixture.
import assert from 'node:assert/strict';
import {createServer} from 'node:http';
import {execFile} from 'node:child_process';
import {promisify} from 'node:util';
import {mkdtempSync, writeFileSync, rmSync} from 'node:fs';
import {join, resolve} from 'node:path';
import {tmpdir} from 'node:os';
import test from 'node:test';
const bundle = process.env.GH_RELEASE_ACTION;
assert.ok(bundle, 'Set GH_RELEASE_ACTION to pinned upstream dist/index.js');
const exec = promisify(execFile);

test('actual publisher recovers partial draft without replacing existing assets', async () => {
  const root = mkdtempSync(join(tmpdir(), 'sdk-recovery-owned-'));
  const requests = [];
  const assets = [];
  let release, failUpload = true, origin;
  const server = createServer(async (req, res) => {
    const url = new URL(req.url, origin);
    const chunks = []; for await (const chunk of req) chunks.push(chunk);
    const body = Buffer.concat(chunks);
    const entry = {method: req.method, path: url.pathname, draftBefore: release?.draft};
    requests.push(entry);
    const json = (status, value) => {entry.status=status; entry.assetCount=assets.length; res.writeHead(status, {'content-type':'application/json'}); res.end(JSON.stringify(value));};
    const current = () => ({...release, assets: assets.map(a=>({id:a.id,name:a.name})), upload_url:`${origin}/uploads{?name,label}`, html_url:`${origin}/release/81`});
    if (req.method === 'GET' && url.pathname === '/repos/fixture/owned/releases/tags/v0.1.0') {
      json(release && !release.draft ? 200 : 404, release && !release.draft ? current() : {message:'Not Found'});
    } else if (req.method === 'GET' && url.pathname === '/repos/fixture/owned/releases') {
      json(200, release ? [current()] : []);
    } else if (req.method === 'POST' && url.pathname === '/repos/fixture/owned/releases') {
      const input = JSON.parse(body); assert.equal(input.tag_name, 'v0.1.0');
      release = {...input, id:81}; entry.draftAfter = release.draft; json(201,current());
    } else if (req.method === 'PATCH' && url.pathname === '/repos/fixture/owned/releases/81') {
      Object.assign(release,JSON.parse(body)); entry.draftAfter = release.draft; json(200,current());
    } else if (req.method === 'GET' && url.pathname === '/repos/fixture/owned/releases/81/assets') {
      json(200,assets.map(a=>({id:a.id,name:a.name})));
    } else if (req.method === 'POST' && url.pathname === '/uploads') {
      const name = url.searchParams.get('name'); entry.name = name;
      if (failUpload && name === 'sdk.tar') {json(400,{message:'Owned upload failure'}); return;}
      const asset = {id:90+assets.length,name,bytes:body};assets.push(asset);json(201,{id:asset.id,name});
    } else {json(400,{message:'Unexpected owned fixture endpoint'});}
  });
  try {
    await new Promise(resolve=>server.listen(0,'127.0.0.1',resolve));origin=`http://127.0.0.1:${server.address().port}`;
    const manifest = join(root,'manifest.json'), archive=join(root,'sdk.tar'), output=join(root,'output');
    writeFileSync(manifest,'{"fixture":"owned"}');writeFileSync(archive,'owned SDK bytes');writeFileSync(output,'');
    const env = {PATH:process.env.PATH,GITHUB_REPOSITORY:'fixture/owned',GITHUB_REF:'refs/tags/v0.1.0',
      GITHUB_API_URL:origin,GITHUB_OUTPUT:output,GITHUB_TOKEN:'owned-fixture-not-a-credential',
      INPUT_TAG_NAME:'v0.1.0',INPUT_TARGET_COMMITISH:'a'.repeat(40),INPUT_FILES:`${manifest}\n${archive}`,
      INPUT_OVERWRITE_FILES:'false',INPUT_PRESERVE_ORDER:'true',INPUT_FAIL_ON_UNMATCHED_FILES:'true'};
    async function invoke() {
      try {await exec(process.execPath,[resolve(bundle)],{cwd:root,env,timeout:20000,maxBuffer:1024*1024});return 0;}
      catch(e) {assert.equal(e.code,1,`Owned Action error: ${String(e.stdout).slice(-1600)} ${String(e.stderr).slice(-300)}`); return 1;}
    }
    assert.equal(await invoke(),1);assert.equal(release.draft,true);assert.equal(assets.length,1);
    const first = assets[0];failUpload=false;
    assert.equal(await invoke(),0);assert.equal(release.draft,false);assert.equal(assets.length,2);
    assert.equal(assets[0],first);assert.equal(first.bytes.toString(),'{"fixture":"owned"}');
    assert.equal(await invoke(),0);assert.equal(assets.length,2);
    assert.equal(requests.filter(r=>r.method==='POST'&&r.path==='/repos/fixture/owned/releases').length,1);
    assert.equal(requests.some(r=>r.method==='DELETE'||r.path.includes('/git/')),false);
    const publish = requests.findIndex(r=>r.method==='PATCH'&&r.draftAfter===false);
    const uploaded = requests.findIndex(r=>r.method==='POST'&&r.name==='sdk.tar'&&r.status===201);
    assert.ok(uploaded>=0&&publish>uploaded,'all declared uploads precede publication');
    assert.equal(requests[publish].assetCount,2);
  } finally {await new Promise(resolve=>server.close(resolve));rmSync(root,{recursive:true,force:true});}
});
