// Execute the exact maintained API Actions against an owned loopback service only.
import test from 'node:test';
import assert from 'node:assert/strict';
import {readFileSync,writeFileSync,mkdtempSync,rmSync} from 'node:fs';
import {tmpdir} from 'node:os';
import {join} from 'node:path';
import {spawn,spawnSync} from 'node:child_process';
import {createServer} from 'node:http';
import {parse} from 'yaml';
import releaseAssets from '../../release/release-assets.cjs';
const flow=parse(readFileSync(new URL('../../.github/workflows/ci.yml',import.meta.url),'utf8'));
const steps=flow.jobs.release.steps;
const tag='v0.5.0',sha='a'.repeat(40);
const replacements={'github.repository_owner':'fixture','github.event.repository.name':'owned',
  'github.repository':'fixture/owned','env.RELEASE_TAG':tag,'env.RELEASE_SOURCE':sha,
  'steps.release_identity.outputs.id':'55'};
const render=value=>String(value).replace(/\$\{\{\s*([^}]+?)\s*\}\}/g,(_,key)=>{
  assert.ok(Object.hasOwn(replacements,key),key);return replacements[key];
});
function outputs(text){
  const lines=text.split(/\r?\n/),result={};
  for(let i=0;i<lines.length;i++){
    const match=lines[i].match(/^([^<]+)<<(.+)$/);
    if(match){const data=[];while(++i<lines.length&&lines[i]!==match[2])data.push(lines[i]);result[match[1]]=data.join('\n');}
  }
  return result;
}

test('maintained exact-tag lookup, explicit draft create and original-ID finalize use actual workflow inputs',async t=>{
  assert.ok(process.env.GRAPHQL_ACTION);assert.ok(process.env.REQUEST_ACTION);
  const directory=mkdtempSync(join(tmpdir(),'sdk-release-actions-owned-'));
  t.after(()=>rmSync(directory,{recursive:true,force:true}));
  let release=null,count=0;const events=[];
  const server=createServer(async(req,res)=>{
    try{
      const chunks=[];for await(const chunk of req)chunks.push(chunk);
      const body=JSON.parse(Buffer.concat(chunks).toString()||'{}');
      events.push({method:req.method,path:req.url,body});
      let data;
      if(req.method==='POST'&&req.url==='/graphql'){
        assert.deepEqual(body.variables,{owner:'fixture',repo:'owned',tag});
        assert.ok(body.query.includes('release(tagName: $tag)'));
        data={data:{repository:{release:release?{databaseId:55,tagName:tag,isDraft:release.draft,releaseAssets:{totalCount:0}}:null}}};
      }else if(req.method==='POST'&&req.url==='/repos/fixture/owned/releases'){
        assert.equal(release,null);assert.equal(body.tag_name,tag);assert.equal(body.target_commitish,sha);
        assert.equal(body.draft,true);assert.equal(body.generate_release_notes,true);
        release={id:55,tag_name:tag,draft:true};data=release;
      }else if(req.method==='PATCH'&&req.url==='/repos/fixture/owned/releases/55'){
        assert.equal(release.draft,true);assert.equal(body.draft,false);
        release={...release,...body};data=release;
      }else throw new Error('Unexpected owned route');
      res.writeHead(req.method==='POST'&&req.url.endsWith('/releases')?201:200,{'content-type':'application/json'});
      res.end(JSON.stringify(data));
    }catch(error){res.writeHead(400,{'content-type':'application/json'}).end(JSON.stringify({message:error.message}));}
  });
  await new Promise(resolve=>server.listen(0,'127.0.0.1',resolve));
  t.after(()=>new Promise(resolve=>{server.closeAllConnections();server.close(resolve)}));
  async function action(name,binary){
    const step=steps.find(s=>s.name===name);assert.ok(step,name);
    const output=join(directory,'outputs-'+count++);writeFileSync(output,'');
    const env=Object.fromEntries(Object.entries(process.env).filter(([key])=>['PATH','SYSTEMROOT','SystemRoot','TEMP','TMP'].includes(key)));
    Object.assign(env,{GITHUB_API_URL:'http://127.0.0.1:'+server.address().port,
      GITHUB_TOKEN:'owned-fixture-not-a-credential',GITHUB_ACTION:'owned-fixture',GITHUB_OUTPUT:output});
    for(const [key,value] of Object.entries(step.with))env['INPUT_'+key.toUpperCase()]=render(value);
    const result=await new Promise((resolve,reject)=>{
      const child=spawn(process.execPath,[binary],{env});let text='';
      const timer=setTimeout(()=>{child.kill();reject(new Error('Owned Action deadline'))},30000);
      child.stdout.on('data',value=>text+=value);child.stderr.on('data',value=>text+=value);
      child.on('error',reject);child.on('close',code=>{clearTimeout(timer);resolve({code,text})});
    });
    assert.equal(result.code,0,result.text);return outputs(readFileSync(output,'utf8'));
  }
  const found=await action('Observe exact release by tag, including an older draft',process.env.GRAPHQL_ACTION);
  assert.equal(releaseAssets.observe(JSON.parse(found.data),tag).absent,'true');
  const created=await action('Create only an absent explicit draft',process.env.REQUEST_ACTION);
  assert.equal(releaseAssets.created(JSON.parse(created.data),created.status,tag),'55');
  const observed=await action('Re-observe exact release immediately before finalization',process.env.GRAPHQL_ACTION);
  assert.equal(releaseAssets.observe(JSON.parse(observed.data),tag,'55').published,'false');
  await action('Finalize only the verified exact draft',process.env.REQUEST_ACTION);
  assert.equal(release.draft,false);
  assert.deepEqual(events.map(e=>e.method+' '+e.path),['POST /graphql','POST /repos/fixture/owned/releases','POST /graphql','PATCH /repos/fixture/owned/releases/55']);
});

test('actual replacement-ID shell admits original and refuses the known fallback',()=>{
  const step=steps.find(s=>s.name==='Refuse a replacement uploader release ID');
  for(const [uploaded,expected] of [['55',0],['56',1]]){
    const result=spawnSync('bash',['-c',step.run],{env:{PATH:process.env.PATH,ORIGINAL_ID:'55',UPLOADED_ID:uploaded}});
    assert.equal(result.status,expected);
  }
});
