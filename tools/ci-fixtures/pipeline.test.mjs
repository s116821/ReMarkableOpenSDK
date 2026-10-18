// Inspect actual YAML and evaluate actual expressions with GitHub's maintained library.
import test from 'node:test';
import assert from 'node:assert/strict';
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
const good=()=>({github:{event_name:'pull_request'},needs:{title:{result:'success'},checks:{result:'success',outputs:{application:'true'}},portable_checks:{result:'success'},build:{result:'success'}}});
const runs=job=>job.steps.filter(s=>s.run).map(s=>s.run).join('\n');

test('one compact core plus reusable title, with one explicit Windows platform job',()=>{
  assert.deepEqual(readdirSync(new URL('../../.github/workflows/',import.meta.url)).sort(),['ci.yml','pr-title.yml']);
  assert.deepEqual(Object.keys(flow.jobs),['title','checks','portable_checks','build','integration']);
  assert.equal(flow.jobs.title.uses,'./.github/workflows/pr-title.yml');
  assert.equal(flow.jobs.checks['runs-on'],'ubuntu-latest');
  assert.equal(flow.jobs.portable_checks['runs-on'],'windows-latest');
  assert.deepEqual(flow.jobs.build.needs,['checks','portable_checks']);
  assert.deepEqual(flow.jobs.integration.needs,['checks','build']);
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
  const download=flow.jobs.integration.steps.find(s=>s.uses?.startsWith('actions/download-artifact@'));
  assert.equal(download.with['artifact-ids'],'${{ needs.build.outputs.artifact_id }}');
  assert.equal(download.with.name,undefined);
  const integrate=flow.jobs.integration.steps.find(s=>s.run);
  assert.equal(integrate.env.RECEIPT,'${{ needs.build.outputs.receipt_sha }}');
  assert.equal(integrate.env.SOURCE,'${{ needs.build.outputs.source_sha }}');
  assert.ok(integrate.run.includes('candidate_artifacts.py integration'));
  assert.ok(integrate.run.includes('test_candidate_artifacts.py'));
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
test('no compact job receives publication permission or activation',()=>{
  assert.equal(flow.permissions.contents,'read');assert.equal(title.permissions.contents,'read');
  for(const job of Object.values(flow.jobs)){
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
