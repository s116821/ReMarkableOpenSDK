// Inspect actual YAML and evaluate actual expressions with GitHub's maintained library.
import test from 'node:test';
import assert from 'node:assert/strict';
import {spawnSync} from 'node:child_process';
import {readFileSync,readdirSync} from 'node:fs';
import {parse} from 'yaml';
import {Lexer,Parser,Evaluator,data} from '@actions/expressions';
const flow=parse(readFileSync(new URL('../../.github/workflows/ci.yml',import.meta.url),'utf8'));
const title=parse(readFileSync(new URL('../../.github/workflows/pr-title.yml',import.meta.url),'utf8'));
function value(input){
  if(input===null||input===undefined)return new data.Null();
  if(typeof input==='string')return new data.StringData(input);
  if(typeof input==='boolean')return new data.BooleanData(input);
  if(typeof input==='number')return new data.NumberData(input);
  return new data.Dictionary(...Object.entries(input).map(([key,v])=>({key,value:value(v)})));
}
const always={name:'always',minArgs:0,maxArgs:0,call:()=>new data.BooleanData(true)};
function evaluate(expression,context,{cancelled=false}={}){
  const text=expression.replace(/^\$\{\{\s*|\s*\}\}$/g,'');
  const lexed=new Lexer(text).lex();
  const cancel={name:'cancelled',minArgs:0,maxArgs:0,call:()=>new data.BooleanData(cancelled)};
  const parsed=new Parser(lexed.tokens,Object.keys(context),[always,cancel]).parse();
  return new Evaluator(parsed,value(context),new Map([['always',always],['cancelled',cancel]])).evaluate().coerceString();
}
const good=()=>({github:{event_name:'pull_request'},needs:{title:{result:'success',outputs:{body_only:'false',application:'true'}},checks:{result:'success',outputs:{application:'true'}},portable_checks:{result:'success'},build:{result:'success'}}});
const runs=job=>job.steps.filter(s=>s.run).map(s=>s.run).join('\n');

test('one compact core plus reusable title, with one explicit Windows platform job',()=>{
  assert.deepEqual(readdirSync(new URL('../../.github/workflows/',import.meta.url)).sort(),['ci.yml','pr-title.yml']);
  assert.deepEqual(Object.keys(flow.jobs),['title','checks','portable_checks','build','integration','release']);
  assert.equal(flow.jobs.title.uses,'./.github/workflows/pr-title.yml');
  assert.equal(flow.jobs.checks['runs-on'],'ubuntu-latest');
  assert.equal(flow.jobs.portable_checks['runs-on'],'windows-latest');
  assert.deepEqual(flow.jobs.build.needs,['title','checks','portable_checks']);
  assert.deepEqual(flow.jobs.integration.needs,['title','checks','build']);
  assert.equal(flow.defaults.run.shell,'bash');
  assert.equal(title.defaults.run.shell,'bash');
});
test('corrected metadata runs the same title and core without skipped failure replacement',()=>{
  assert.deepEqual(flow.on.pull_request.types,['opened','reopened','synchronize','edited']);
  assert.deepEqual(flow.on.push.branches,['main']);
  assert.equal(title.on.pull_request,undefined,'no standalone title status or parallel workflow');
  assert.ok(title.on.workflow_call.outputs.application);
  assert.deepEqual(Object.keys(title.jobs),['title']);
  assert.equal(evaluate(flow.jobs.checks.if,good()),'true');
  const corrected=good();corrected.github.event={action:'edited'};
  assert.equal(evaluate(flow.jobs.checks.if,corrected),'true','title correction resumes real checks on unchanged source');
  assert.equal(evaluate(flow.jobs.build.if,corrected),'true');
  corrected.needs.checks.result='failure';
  assert.equal(evaluate(flow.jobs.build.if,corrected),'false','body edits cannot turn failed application checks into skipped success');
  for(const state of ['failure','skipped','cancelled']){
    const c=good();c.needs.title.result=state;
    assert.equal(evaluate(flow.jobs.checks.if,c),'false',state);
  }
  const c=good();c.github.event_name='push';c.needs.title.result='skipped';
  assert.equal(evaluate(flow.jobs.checks.if,c),'true','main has no PR-title requirement');
});
test('maintained path admission has one owner per event and docs skip Rust',()=>{
  const classifier=s=>s.uses?.startsWith('dorny/paths-filter@');
  const main=flow.jobs.checks.steps.filter(classifier),pr=title.jobs.title.steps.filter(classifier);
  assert.equal(main.length,1);assert.equal(pr.length,1);
  assert.equal(main[0].if,"github.event_name == 'push'");
  assert.equal(main[0].with.filters,'.github/sdk-paths.yml');
  assert.equal(pr[0].with.filters,'.github/sdk-paths.yml');
  for(const name of ['build','integration','portable_checks']){
    const c=good();c.needs.checks.outputs.application='false';
    assert.equal(evaluate(flow.jobs[name].if,c),'false',name);
  }
  for(const step of flow.jobs.checks.steps){
    if(step.uses?.startsWith('dtolnay/')||/cargo |SDK_FIXTURE_CARGO|sdk-semantic-tools|test_stage_admission/.test(step.run??''))
      assert.match(step.if??'',/steps\.admission\.outputs\.application == 'true'/,step.name??step.uses);
  }
});
test('failed skipped or cancelled required predecessors never admit build/integration',()=>{
  assert.equal(evaluate(flow.jobs.build.if,good()),'true');
  assert.equal(evaluate(flow.jobs.integration.if,good()),'true');
  for(const [stage,predecessors] of [['build',['checks','portable_checks']],['integration',['checks','build']]]){
    for(const predecessor of predecessors)for(const result of ['failure','skipped','cancelled']){
      const c=good();c.needs[predecessor].result=result;
      assert.equal(evaluate(flow.jobs[stage].if,c),'false',`${stage}/${predecessor}/${result}`);
    }
  }
});
test('existing checks have one stage/platform owner with no duplicate SDK unit suite',()=>{
  const all=Object.values(flow.jobs).filter(j=>j.steps).map(runs).join('\n');
  assert.equal((all.match(/cargo \+1\.88\.0 test --locked --offline --all-features/g)??[]).length,1);
  assert.equal((all.match(/pip-audit/g)??[]).length,1);
  for(const pattern of ['test_generated_package_lock.py','test_package_inventory.py','test_verify_assets.py','python_semantic_release_fixtures.py','test_semantic_metadata.py','test_commit_eligibility.py','release-assets.test.cjs','test_package_contents.py']){
    assert.equal(all.split(pattern).length-1,1,pattern);
    assert.ok(runs(flow.jobs.checks).includes(pattern),pattern);
  }
  for(const job of [flow.jobs.checks,flow.jobs.portable_checks])
    for(const pattern of ['test_stage_admission.py','test_candidate_admission.py','test_portable_stage.py'])assert.equal(runs(job).split(pattern).length-1,1);
  const checks=runs(flow.jobs.checks);
  assert.equal(checks.split('-p test_rm1_baseline.py').length-1,1);
  assert.ok(checks.includes('py_compile tools/collect_rm1_baseline.py tools/test_rm1_baseline.py'));
  for(const text of ['check_consumer_boundary.py --toolchain 1.88.0','check_consumer_boundary.py --toolchain 1.98.1','clippy','-fsanitize=address,undefined','qemu-arm'])assert.ok(checks.includes(text),text);
  assert.equal(flow.jobs.build.steps.some(s=>/cargo.*test|unittest/.test(s.run??'')),false);
  assert.equal(flow.jobs.integration.steps.some(s=>/cargo|rustc|SDK unit/.test(s.run??'')),false);
});
test('candidate is read-only after checks and integration uses the exact artifact ID',()=>{
  const candidate=flow.jobs.build.steps.find(s=>s.id==='version');
  for(const flag of ['--print-tag','--no-commit','--no-tag','--no-changelog','--skip-build','--no-push','--no-vcs-release'])assert.ok(candidate.run.includes(flag),flag);
  const checkSteps=flow.jobs.checks.steps;
  const guard=checkSteps.findIndex(s=>s.name==='Require actual application main message eligibility');
  const observe=checkSteps.findIndex(s=>s.name==='Observe exact merged main PR without release writes');
  assert.ok(guard>=0&&observe>guard);
  assert.equal(checkSteps[observe].env.ELIGIBILITY_ONLY,'true');
  const download=flow.jobs.integration.steps.find(s=>s.with?.['artifact-ids']==='${{ needs.build.outputs.artifact_id }}');
  assert.equal(download.with['artifact-ids'],'${{ needs.build.outputs.artifact_id }}');
  assert.equal(download.with.name,undefined);
  const integrate=flow.jobs.integration.steps.find(s=>s.run?.includes('candidate_artifacts.py integration'));
  assert.equal(integrate.env.RECEIPT,'${{ needs.build.outputs.receipt_sha }}');
  assert.equal(integrate.env.SOURCE,'${{ needs.build.outputs.source_sha }}');
  assert.ok(integrate.run.includes('candidate_artifacts.py integration'));
  assert.ok(integrate.run.includes('test_candidate_artifacts.py'));
  const uploader=flow.jobs.integration.steps.find(s=>s.with?.['artifact-ids']==='${{ needs.checks.outputs.uploader_artifact_id }}');
  assert.ok(uploader);
  assert.equal(uploader.with.name,undefined);
  assert.equal(flow.jobs.checks.outputs.uploader_artifact_id,'${{ steps.uploader_artifacts.outputs.artifact-id }}');
  assert.equal(flow.jobs.checks.outputs.uploader_provenance_sha,'${{ steps.uploader.outputs.provenance_sha256 }}');
  const verify=flow.jobs.integration.steps.find(s=>s.run?.includes('--expected-provenance'));
  assert.equal(verify.env.SDK_UPLOADER_PROVENANCE_SHA,'${{ needs.checks.outputs.uploader_provenance_sha }}');
  assert.ok(verify.run.includes('--expected-provenance "$SDK_UPLOADER_PROVENANCE_SHA"'));
});
test('retained probe is a real built-library consumer rather than unit replay',()=>{
  const helper=readFileSync(new URL('../../release/candidate_artifacts.py',import.meta.url),'utf8');
  assert.ok(helper.includes('release/sdk_consumer_probe.rs'));
  assert.ok(helper.includes("'--extern', 'remarkable_open_sdk='"));
  assert.ok(helper.includes("'dependency=' + str(host / 'deps')"));
  assert.equal(helper.includes("'test', '--locked'"),false);
  assert.ok(helper.includes("'host_probes': executables"));
  assert.ok(helper.includes("receipt.get('schema') != 2"));
  assert.ok(helper.indexOf('Transferred consumer probe bytes differ')<helper.indexOf('if execute_probes:'));
  assert.ok(helper.indexOf('Transferred source crate identity differs')<helper.indexOf('if execute_probes:'));
  const upload=flow.jobs.build.steps.find(s=>s.uses?.startsWith('actions/upload-artifact@'));
  assert.ok(upload.with.path.includes('/probes/'));
  assert.equal(upload.with.path.includes('/tests/'),false);
});
test('only the final Release job receives publication permission or activation',()=>{
  assert.equal(flow.permissions.contents,'read');assert.equal(title.permissions.contents,'read');
  assert.equal(flow.jobs.release.permissions.contents,'write');
  for(const [name,job] of Object.entries(flow.jobs)){
    if(name==='release')continue;
    assert.notEqual(job.permissions?.contents,'write');
    if(!job.steps)continue;
    for(const step of job.steps){
      assert.equal(step.with?.route?.startsWith('POST'),undefined);
      assert.equal(step.with?.route?.startsWith('PATCH'),undefined);
      assert.equal((step.run??'').includes('--vcs-release'),false);
    }
  }
});

// GitHub adds implicit success() when job.if has no status function. Expression
// truth alone does not prove scheduling across the intentionally skipped title.
test('main successors explicitly override implicit status while retaining their required gates',()=>{
  const c=good();c.github.event_name='push';c.needs.title.result='skipped';
  for(const name of ['checks','portable_checks','build','integration']){
    assert.match(flow.jobs[name].if,/!cancelled\(\)/,`${name}: explicit cancellation-aware status guard`);
    assert.equal(evaluate(flow.jobs[name].if,c),'true',name);
  }
});
test('workflow cancellation refuses every application stage even after prior success',()=>{
  for(const name of ['checks','portable_checks','build','integration'])
    assert.equal(evaluate(flow.jobs[name].if,good(),{cancelled:true}),'false',name);
});


test('final Release admits only uncancelled eligible main with all four successful stages',()=>{
  const c=good();c.github={event_name:'push',ref:'refs/heads/main'};
  c.needs.integration={result:'success'};
  assert.deepEqual(flow.jobs.release.needs,['title','checks','portable_checks','build','integration']);
  assert.equal(evaluate(flow.jobs.release.if,c),'true');
  assert.equal(evaluate(flow.jobs.release.if,c,{cancelled:true}),'false');
  for(const name of flow.jobs.release.needs.filter(n=>n!=='title'))for(const result of ['failure','skipped','cancelled']){
    const bad=structuredClone(c);bad.needs[name].result=result;
    assert.equal(evaluate(flow.jobs.release.if,bad),'false',`${name}/${result}`);
  }
  for(const mutate of [x=>x.github.event_name='pull_request',x=>x.github.ref='refs/heads/topic',x=>x.needs.checks.outputs.application='false']){
    const bad=structuredClone(c);mutate(bad);assert.equal(evaluate(flow.jobs.release.if,bad),'false');
  }
});

test('final Release consumes exact tested artifacts without rebuilding or replaying probes',()=>{
  const release=flow.jobs.release,steps=release.steps,text=runs(release);
  assert.equal(release.env.RELEASE_TAG,'${{ needs.build.outputs.tag }}');
  const downloads=steps.filter(s=>s.uses?.startsWith('actions/download-artifact@'));
  assert.deepEqual(downloads.map(s=>s.with['artifact-ids']),[
    '${{ needs.build.outputs.artifact_id }}','${{ needs.checks.outputs.uploader_artifact_id }}']);
  for(const step of downloads)assert.equal(step.with.name,undefined);
  const verified=steps.findIndex(s=>s.name==='Revalidate exact tested build');
  const eligible=steps.findIndex(s=>s.name==='Require exact merged main PR before any official tag');
  const tag=steps.findIndex(s=>s.name==='Bind official tag only after predecessor success');
  assert.ok(verified>=0&&eligible>verified&&tag>eligible);
  assert.equal(steps[verified].env.RECEIPT,'${{ needs.build.outputs.receipt_sha }}');
  assert.equal(steps[verified].env.BUILT_SOURCE,'${{ needs.build.outputs.source_sha }}');
  assert.equal(steps[verified].env.UPLOADER_PROVENANCE,'${{ needs.checks.outputs.uploader_provenance_sha }}');
  assert.ok(steps[verified].run.includes('--expected-provenance "$UPLOADER_PROVENANCE"'));
  assert.ok(steps[verified].run.includes('candidate_artifacts.py verify'));
  assert.equal(/cargo |rustc |npm |unittest|candidate_artifacts.py (?:build|integration)/.test(text),false);
  assert.equal(text.includes('pip-audit'),false,'audit remains owned by Checks');
  assert.ok(steps[tag].run.includes('test "$candidate" = "$RELEASE_TAG"'));
  assert.ok(steps[tag].run.includes('verify_remote_tag'));
  assert.ok(steps[tag].run.includes('--no-commit --no-changelog --skip-build --push --no-vcs-release'));
});

test('maintained finalization retains original ID and verified missing-only bytes',()=>{
  const steps=flow.jobs.release.steps;
  const upload=steps.find(s=>s.id==='upload');
  assert.equal(upload.uses,'./.qualified/uploader');
  for(const key of ['overwrite','promote','prerelease','make_latest'])assert.equal(upload.with[key],false,key);
  for(const key of ['draft','check_duplicates','file_glob'])assert.equal(upload.with[key],true,key);
  const replacement=steps.findIndex(s=>s.name==='Refuse a replacement uploader release ID');
  const complete=steps.findIndex(s=>s.name==='Verify complete bytes and re-observe remote tag before finalization');
  const observed=steps.findIndex(s=>s.name==='Require the same release before its final mutation');
  const publish=steps.findIndex(s=>s.name==='Finalize only the verified exact draft');
  assert.ok(replacement>=0&&complete>replacement&&observed>complete&&publish>observed);
  assert.equal(steps[replacement].env.ORIGINAL_ID,'${{ steps.release_identity.outputs.id }}');
  assert.equal(steps[replacement].env.UPLOADED_ID,'${{ steps.upload.outputs.release_id }}');
  assert.ok(steps[replacement].run.includes('test "$UPLOADED_ID" = "$ORIGINAL_ID"'));
  assert.ok(steps[complete].run.includes('verify_downloaded_assets'));
  assert.ok(steps[observed].with.script.includes('process.env.RELEASE_ID'));
  assert.equal(steps[publish].with.route,'PATCH /repos/{repo}/releases/{release_id}');
  assert.equal(steps[publish].with.release_id,'${{ steps.release_identity.outputs.id }}');
  assert.equal(steps[publish].with.draft,false);
  for(const [name,job] of Object.entries(flow.jobs))for(const step of job.steps??[])
    if(/^(POST|PATCH|DELETE) /.test(step.with?.route??''))assert.equal(name,'release');
});

test('native queue covers main candidate through publication and keeps PR runs independent',()=>{
  assert.equal(flow.concurrency.queue,'max');
  assert.equal(flow.concurrency['cancel-in-progress'],false);
  const c=good();c.github.run_id='one';
  assert.equal(evaluate(flow.concurrency.group,c),'one');
  c.github.run_id='two';assert.equal(evaluate(flow.concurrency.group,c),'two');
  c.github.event_name='push';assert.equal(evaluate(flow.concurrency.group,c),'sdk-main-delivery');
  assert.equal(flow.jobs.release.concurrency,undefined,'one workflow lock avoids a nested deadlock');
});


function actualBodyOnly(event){
  const step=title.jobs.title.steps.find(s=>s.id==='metadata');
  const match=step.run.match(/jq -r '([\s\S]*?)' "\$GITHUB_EVENT_PATH"/);
  assert.ok(match,'execute actual classifier, not a fixture copy');
  const r=spawnSync(process.env.METADATA_JQ||'jq',['-r',match[1]],{input:JSON.stringify(event),encoding:'utf8'});
  assert.ifError(r.error);assert.equal(r.status,0,r.stderr);assert.match(r.stdout.trim(),/^(true|false)$/);return r.stdout.trim();
}
test('exact body classifier rejects base mixed unknown and malformed changes',()=>{
  for(const from of ['', 'old', '{"title":"text","base":{}}'])assert.equal(actualBodyOnly({action:'edited',changes:{body:{from}}}),'true');
  const base={ref:{from:'old'},sha:{from:'a'.repeat(40)}};
  for(const changes of [undefined,null,{},[], 'body',{title:{from:'old'}},{base},{body:{from:'old'},base},{body:{from:'old'},title:{from:'old'}},{body:{from:'old'},unknown:true},{unknown:true},{body:{}},{body:{from:null}},{body:{from:7}},{body:{from:true}},{body:{from:'old',unknown:true}},{body:'old'}])assert.equal(actualBodyOnly({action:'edited',changes}),'false',JSON.stringify(changes));
  assert.equal(actualBodyOnly({action:'synchronize',changes:{body:{from:'old'}}}),'false');
});
test('both reusable output levels preserve exact metadata custody',()=>{
  assert.equal(title.on.workflow_call.outputs.body_only.value,'${{ jobs.title.outputs.body_only }}');
  assert.equal(title.jobs.title.outputs.body_only,'${{ steps.metadata.outputs.body_only }}');
  assert.ok(title.jobs.title.steps.findIndex(s=>s.id==='metadata')<title.jobs.title.steps.findIndex(s=>s.uses?.startsWith('amannn/')));
});
test('all flat omitted contexts are distinct while normal executed names and guards stay',()=>{
  const names={checks:'Checks (Linux)',portable_checks:'Checks (Windows staging)',build:'Build',integration:'Integration',release:'Release'};
  const c=good();c.needs.integration={result:'success'};
  for(const [job,name] of Object.entries(names)){assert.ok([flow.jobs[job].needs].flat().includes('title'));assert.equal(evaluate(flow.jobs[job].name,c),name);}
  c.needs.title.outputs.body_only='true';c.needs.checks.result='skipped';c.needs.portable_checks.result='skipped';c.needs.build.result='skipped';
  for(const [job,name] of Object.entries(names)){assert.notEqual(evaluate(flow.jobs[job].name,c),name);assert.equal(evaluate(flow.jobs[job].if,c),'false',job);}
  c.needs.title.outputs.body_only='false';c.needs.checks.result='failure';
  for(const job of ['portable_checks','build','integration','release'])assert.equal(evaluate(flow.jobs[job].if,c),'false',job);
});
test('main intentionally skipped title remains nonrequired for final Release',()=>{
  const c=good();c.github={event_name:'push',ref:'refs/heads/main'};c.needs.title={result:'skipped',outputs:{}};c.needs.integration={result:'success'};
  for(const job of ['checks','portable_checks','build','integration','release'])assert.equal(evaluate(flow.jobs[job].if,c),'true',job);
  assert.equal(evaluate(flow.jobs.release.name,c),'Release');
  assert.equal(evaluate(flow.jobs.release.if,c,{cancelled:true}),'false');
});
