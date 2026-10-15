// Requirements regression using maintained YAML and GitHub's actual expression evaluator.
import {test} from 'node:test';
import assert from 'node:assert/strict';
import {readFileSync,mkdtempSync,writeFileSync,rmSync} from 'node:fs';
import {tmpdir} from 'node:os';
import {join} from 'node:path';
import {execFileSync,spawnSync} from 'node:child_process';
import {parse} from 'yaml';
import {Lexer,Parser,Evaluator,data} from '@actions/expressions';
const flow=parse(readFileSync(new URL('../../.github/workflows/ci.yml',import.meta.url),'utf8'));
function value(input){
  if(input===null||input===undefined)return new data.Null();
  if(typeof input==='string')return new data.StringData(input);
  if(typeof input==='boolean')return new data.BooleanData(input);
  if(typeof input==='number')return new data.NumberData(input);
  return new data.Dictionary(...Object.entries(input).map(([key,v])=>({key,value:value(v)})));
}
function evaluate(expression,context){
  const lexed=new Lexer(expression).lex();
  const parsed=new Parser(lexed.tokens,Object.keys(context),[]).parse();
  return new Evaluator(parsed,value(context)).evaluate().coerceString();
}
const good=()=>({github:{event_name:'push',ref:'refs/heads/main'},needs:{
  checks:{result:'success',outputs:{application:'true'}},portable_checks:{result:'success'},build:{result:'success'},integration:{result:'success'}}});

test('only the final release job may mutate official release state',()=>{
  const paths=flow.jobs.checks.steps.find(s=>s.id==='paths');
  assert.equal(paths.with['working-directory'],'sdk');
  assert.equal(paths.with.filters,'.github/release-paths.yml');
  assert.deepEqual(Object.keys(flow.jobs),['checks','portable_checks','build','integration','release']);
  assert.equal(flow.permissions.contents,'read');
  assert.equal(flow.jobs.release.permissions.contents,'write');
  for(const [name,job] of Object.entries(flow.jobs)){
    if(name!=='release')assert.notEqual(job.permissions?.contents,'write');
    for(const step of job.steps){
      if(step.with?.route?.startsWith('POST')||step.with?.route?.startsWith('PATCH'))assert.equal(name,'release');
      if(step.run?.includes('version --no-commit')&&!step.run.includes('--no-tag'))assert.equal(name,'release');
    }
  }
  const steps=flow.jobs.release.steps;
  const verified=steps.findIndex(s=>s.name==='Revalidate exact tested build');
  const eligible=steps.findIndex(s=>s.name==='Require exact merged main PR before any official tag');
  const tag=steps.findIndex(s=>s.name==='Bind official tag only after predecessor success');
  assert.ok(verified>=0&&eligible>verified&&tag>eligible);
});

test('checks gate build and integration consumes transferred build bytes',()=>{
  assert.deepEqual(flow.jobs.build.needs,['checks','portable_checks']);
  assert.deepEqual(flow.jobs.integration.needs,['checks','build']);
  assert.deepEqual(flow.jobs.release.needs,['checks','portable_checks','build','integration']);
  assert.ok(flow.jobs.integration.steps.some(s=>s.uses?.startsWith('actions/download-artifact@')&&s.with.name==='sdk-candidate-build'));
  assert.ok(flow.jobs.integration.steps.some(s=>s.run?.includes('candidate_artifacts.py integration')));
  assert.equal(flow.jobs.integration.steps.some(s=>s.run?.includes('cargo build')),false);
  assert.equal(flow.jobs.release.steps.some(s=>s.run?.includes('cargo build')),false);
  assert.equal(flow.jobs.release.concurrency['cancel-in-progress'],false);
  assert.equal(flow.jobs.release.concurrency.queue,'max');
  assert.ok(flow.jobs.checks.steps.findIndex(s=>s.name==='Require exactly one observed main first-parent commit') < flow.jobs.checks.steps.findIndex(s=>s.id==='paths'));
  const admission=flow.jobs.checks.steps.find(s=>s.name==='Require exactly one observed main first-parent commit');
  assert.match(admission.run,/--first-parent --count/);
  assert.match(admission.run,/0000000000000000000000000000000000000000/);
});

test('actual upstream expression refuses failed, skipped and canceled predecessors',()=>{
  assert.equal(evaluate(flow.jobs.release.if,good()),'true');
  for(const job of ['checks','portable_checks','build','integration'])for(const state of ['failure','skipped','cancelled']){
    const context=good();context.needs[job].result=state;
    assert.equal(evaluate(flow.jobs.release.if,context),'false',`${job}/${state}`);
  }
  for(const change of [c=>c.github.event_name='pull_request',c=>c.github.ref='refs/heads/topic',c=>c.needs.checks.outputs.application='false']){
    const context=good();change(context);assert.equal(evaluate(flow.jobs.release.if,context),'false');
  }
});

test('candidate calculation is read-only and lives before SDK build',()=>{
  const candidate=flow.jobs.checks.steps.find(s=>s.id==='candidate');
  assert.match(candidate.run,/--print-tag/);
  for(const flag of ['--no-commit','--no-tag','--no-push','--no-vcs-release'])assert.ok(candidate.run.includes(flag));
  assert.equal(candidate.if,"steps.paths.outputs.application == 'true'");
  assert.ok(flow.jobs.build.steps.some(s=>s.run?.includes('candidate_artifacts.py build')));
});

test('actual main-admission shell accepts one merge and refuses ambiguous edit/revert batches',t=>{
  const root=mkdtempSync(join(tmpdir(),'sdk-main-admission-owned-'));
  t.after(()=>rmSync(root,{recursive:true,force:true}));
  const config=join(root,'empty.gitconfig');writeFileSync(config,'');
  const env={...process.env,GIT_CONFIG_NOSYSTEM:'1',GIT_CONFIG_GLOBAL:config,
    GIT_AUTHOR_NAME:'Owned fixture',GIT_COMMITTER_NAME:'Owned fixture',
    GIT_AUTHOR_EMAIL:'owned@example.invalid',GIT_COMMITTER_EMAIL:'owned@example.invalid'};
  const git=(...args)=>execFileSync('git',args,{cwd:root,env,encoding:'utf8',stdio:['ignore','pipe','pipe']}).trim();
  git('init','--initial-branch=main');writeFileSync(join(root,'app.txt'),'original');git('add','app.txt');git('commit','-m','chore: owned base');
  const base=git('rev-parse','HEAD');
  const shell=flow.jobs.checks.steps.find(s=>s.name==='Require exactly one observed main first-parent commit').run;
  const admission=(before,source)=>spawnSync('bash',['-c',shell],{cwd:root,env:{...env,BEFORE:before,SOURCE:source},encoding:'utf8'}).status;
  writeFileSync(join(root,'app.txt'),'changed');git('commit','-am','feat: owned edit');const once=git('rev-parse','HEAD');
  assert.equal(admission(base,once),0);
  writeFileSync(join(root,'app.txt'),'original');git('commit','-am','revert: owned edit');const twice=git('rev-parse','HEAD');
  assert.notEqual(admission(base,twice),0,'a net-zero edit/revert push must not be called docs-only');
  assert.notEqual(admission('0'.repeat(40),twice),0);
  assert.notEqual(admission(twice,base),0);
  git('checkout','-b','owned-feature');writeFileSync(join(root,'api.txt'),'owned feature');git('add','api.txt');git('commit','-m','feat: owned branch');
  git('checkout','main');const before=git('rev-parse','HEAD');git('merge','--no-ff','owned-feature','-m','feat: owned ordinary merge');
  assert.equal(admission(before,git('rev-parse','HEAD')),0,'one first-parent ordinary merge is supported');
});

test('actual commit guard, retained production regressions and qualified uploader stay required',()=>{
  const steps=flow.jobs.checks.steps;
  const guard=steps.findIndex(s=>s.name==='Require actual main source eligibility without borrowing older history');
  const candidate=steps.findIndex(s=>s.id==='candidate');
  assert.ok(guard>=0&&guard<candidate);
  assert.match(steps[guard].run,/check_commit.py/);
  const regressions=steps.find(s=>s.name==='Upstream semantics and candidate-stage refusal checks').run;
  for(const name of ['test_generated_lock.py','test_package_inventory.py','test_verify_assets.py','test_cargo_consumer.py','test_commit_eligibility.py'])assert.ok(regressions.includes(name),name);
  const tag=flow.jobs.release.steps.find(s=>s.name==='Bind official tag only after predecessor success').run;
  assert.ok(tag.indexOf('check_commit.py')<tag.indexOf('observed=$(git ls-remote'));
  const uploader=flow.jobs.release.steps.find(s=>s.uses==='./.qualified/uploader');
  assert.ok(uploader);
  assert.equal(uploader.with.overwrite,false);
  assert.equal(uploader.with.promote,false);
  assert.equal(uploader.with.draft,true);
  assert.equal(uploader.with.check_duplicates,true);
  assert.equal(uploader.with.release_id,'${{ steps.release_identity.outputs.id }}');
  assert.ok(flow.jobs.release.steps.some(s=>s.run?.includes('qualify_uploader.py')&&s.run.includes('--expected-bundle 379dc8')));
  assert.equal(flow.jobs.release.steps.some(s=>s.run?.includes('awaits shared exact-action qualification')),false);
});
