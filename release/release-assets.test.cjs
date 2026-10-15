const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');
const download = require('./release-assets.cjs');

function fixture() {
  const root = fs.mkdtempSync(path.join(os.tmpdir(),'sdk-retained-readonly-'));
  const keys = ['RELEASE_TAG','RELEASE_SOURCE','EXPECTED_MANIFEST','RETAINED_DIRECTORY','ELIGIBILITY_ONLY','REQUIRE_COMPLETE','RELEASE_ID'];
  const previous = Object.fromEntries(keys.map(k=>[k,process.env[k]]));
  const expected = {git_tag:'v0.5.0',source_sha:'a'.repeat(40),distributions:[{name:'sdk.crate',size:3}]};
  const manifest = path.join(root,'expected.json');
  fs.writeFileSync(manifest,JSON.stringify(expected));
  Object.assign(process.env,{RELEASE_TAG:expected.git_tag,RELEASE_SOURCE:expected.source_sha,
    EXPECTED_MANIFEST:manifest,RETAINED_DIRECTORY:path.join(root,'retained'),ELIGIBILITY_ONLY:'false',REQUIRE_COMPLETE:'false',RELEASE_ID:''});
  const routes = {
    associated:[{merged_at:'owned',merge_commit_sha:expected.source_sha,base:{ref:'main'}}],
    releases:[],assets:[]
  };
  const calls=[]; const outputs={};
  const repos = {listPullRequestsAssociatedWithCommit:'associated',listReleases:'releases',listReleaseAssets:'assets',
    getRelease:async args=>{calls.push(['release-id',args.release_id]);return {data:routes.releases[0]}},
    getReleaseAsset:async args=>{calls.push(['download',args.asset_id]);return {data:Buffer.from('abc')}}};
  const github={rest:{repos},paginate:async (route,args)=>{calls.push([route,args]);return routes[route]}};
  return {root,routes,calls,outputs,args:{github,context:{repo:{owner:'owned',repo:'fixture'}},core:{setOutput:(k,v)=>outputs[k]=v}},
    close(){for(const k of keys){if(previous[k]===undefined)delete process.env[k];else process.env[k]=previous[k]}fs.rmSync(root,{recursive:true})}};
}

test('direct push cannot pass main-merge eligibility before tag or release observation',async()=>{
  const f=fixture();try{f.routes.associated=[];process.env.ELIGIBILITY_ONLY='true';
    await assert.rejects(download(f.args),/merged main PR/);assert.equal(f.calls.length,1);
    assert.equal(fs.existsSync(process.env.RETAINED_DIRECTORY),false);
  }finally{f.close()}
});
test('eligible main merge observation returns without asset or release calls',async()=>{
  const f=fixture();try{process.env.ELIGIBILITY_ONLY='true';await download(f.args);
    assert.equal(f.calls.length,1);assert.equal(fs.existsSync(process.env.RETAINED_DIRECTORY),false);
  }finally{f.close()}
});
test('absent release creates only an empty owned download directory',async()=>{
  const f=fixture();try{await download(f.args);assert.equal(f.outputs.published,'false');
    assert.deepEqual(fs.readdirSync(process.env.RETAINED_DIRECTORY),[]);
    assert.equal(f.calls.some(([route])=>route==='download'),false);
  }finally{f.close()}
});
test('partial draft downloads retained bytes without any write API',async()=>{
  const f=fixture();try{process.env.RELEASE_ID='1';f.routes.releases=[{id:1,tag_name:'v0.5.0',draft:true}];f.routes.assets=[{id:2,name:'sdk.crate',size:3}];
    await download(f.args);assert.equal(f.outputs.published,'false');
    assert.equal(fs.readFileSync(path.join(process.env.RETAINED_DIRECTORY,'sdk.crate'),'utf8'),'abc');
    assert.deepEqual(f.calls.map(([r])=>r),['associated','release-id','assets','download']);
  }finally{f.close()}
});
test('undeclared, duplicate and incomplete published assets refuse',async()=>{
  for(const assets of [[{id:2,name:'../escape',size:3}],[{id:2,name:'sdk.crate',size:3},{id:3,name:'sdk.crate',size:3}],[]]){
    const f=fixture();try{process.env.RELEASE_ID='1';f.routes.releases=[{id:1,tag_name:'v0.5.0',draft:false}];f.routes.assets=assets;
      await assert.rejects(download(f.args),/Unexpected|Incomplete/);
      assert.equal(f.outputs.published,undefined);assert.equal(fs.existsSync(path.join(f.root,'escape')),false);
    }finally{f.close()}
  }
});

test('GraphQL observation distinguishes explicit absence, exact old draft and malformed state',()=>{
  const tag='v0.5.0';
  assert.deepEqual(download.observe({repository:{release:null}},tag),{absent:'true',published:'false',id:''});
  const good={databaseId:55,tagName:tag,isDraft:true,releaseAssets:{totalCount:1}};
  assert.equal(download.observe({repository:{release:good}},tag,'55').id,'55');
  for(const value of [undefined,null,{}, {repository:{}}, {repository:{release:0}},
    {repository:{release:{...good,databaseId:'55'}}},{repository:{release:{...good,isDraft:'true'}}},
    {repository:{release:{...good,releaseAssets:{totalCount:6}}}}]) assert.throws(()=>download.observe(value,tag));
  assert.throws(()=>download.observe({repository:{release:good}},tag,'56'));
  assert.throws(()=>download.observe({repository:{release:null}},tag,'55'));
  assert.equal(download.created({id:55,tag_name:tag,draft:true},'201',tag),'55');
  assert.throws(()=>download.created({id:55,tag_name:tag,draft:false},'201',tag));
});

test('wrong source, wrong base and unmerged associated PRs cannot supply eligibility',async()=>{
  for(const changed of [{merge_commit_sha:'b'.repeat(40)},{base:{ref:'feature'}},{merged_at:null}]){
    const f=fixture();try{process.env.ELIGIBILITY_ONLY='true';
      f.routes.associated=[{...f.routes.associated[0],...changed}];
      await assert.rejects(download(f.args),/merged main PR/);
      assert.equal(f.calls.length,1);
      assert.equal(fs.existsSync(process.env.RETAINED_DIRECTORY),false);
    }finally{f.close()}
  }
});
test('unavailable associated PR read refuses without release calls or local output',async()=>{
  const f=fixture();try{process.env.ELIGIBILITY_ONLY='true';
    f.args.github.paginate=async()=>{throw new Error('Owned API unavailable')};
    await assert.rejects(download(f.args),/Owned API unavailable/);
    assert.deepEqual(f.calls,[]);
    assert.equal(fs.existsSync(process.env.RETAINED_DIRECTORY),false);
  }finally{f.close()}
});
