// Owned local repositories only; no production tag/release/network publication.
import assert from 'node:assert/strict';
import {execFileSync, execFile} from 'node:child_process';
import {promisify} from 'node:util';
import {fileURLToPath} from 'node:url';
import {mkdtempSync, rmSync, readFileSync, writeFileSync} from 'node:fs';
import {tmpdir} from 'node:os';
import {join} from 'node:path';
import test from 'node:test';
import {analyzeCommits} from '@semantic-release/commit-analyzer';

const quiet = {log() {}, error() {}, success() {}};
import {analyzer} from './policy.mjs';
const exec = promisify(execFile);
const runner = fileURLToPath(new URL('./semantic-run.mjs', import.meta.url));
const env = {
  PATH: process.env.PATH,
  GIT_CONFIG_NOSYSTEM: '1', GIT_CONFIG_GLOBAL: '/dev/null',
  GIT_AUTHOR_NAME: 'Owned Fixture', GIT_AUTHOR_EMAIL: 'fixture@example.invalid',
  GIT_COMMITTER_NAME: 'Owned Fixture', GIT_COMMITTER_EMAIL: 'fixture@example.invalid',
  GIT_AUTHOR_DATE: '2026-01-01T00:00:00Z', GIT_COMMITTER_DATE: '2026-01-01T00:00:00Z',
};
const git = (cwd, ...args) => execFileSync('git', args, {cwd, env, encoding: 'utf8', stdio: ['ignore', 'pipe', 'pipe']}).trim();

async function withRepo(run) {
  const root = mkdtempSync(join(tmpdir(), 'sdk-semantic-owned-'));
  const remote = join(root, 'remote.git');
  const cwd = join(root, 'work');
  try {
    writeFileSync(join(root, '.owned-fixture'), 'REM-50 owned fixture\n');
    git(root, 'init', '--bare', '--initial-branch=main', remote);
    git(root, 'clone', remote, cwd);
    git(cwd, 'commit', '--allow-empty', '-m', 'chore: fixture base');
    git(cwd, 'tag', 'v0.1.0');
    git(cwd, 'push', 'origin', 'main', '--tags');
    await run({cwd, remote, git: (...args) => git(cwd, ...args),
      commit(subject) {git(cwd, 'commit', '--allow-empty', '-m', subject); git(cwd, 'push', 'origin', 'main');},
      advanceRemote(subject) {
        const other = join(root, 'other');
        git(root, 'clone', remote, other);
        git(other, 'commit', '--allow-empty', '-m', subject);
        git(other, 'push', 'origin', 'main');
      },
      async release(mode = 'dry') {
        const before = git(cwd, 'show-ref', '--tags');
        // Isolate upstream stdout interception from Node's test reporter.
        const {stdout} = await exec(process.execPath, [runner, cwd, remote, mode], {env, maxBuffer: 1024 * 1024});
        const response = JSON.parse(stdout);
        if (mode === 'dry') {
          assert.equal(response.error, undefined);
          assert.equal(git(cwd, 'show-ref', '--tags'), before, 'dry run must not mint tags');
          return response.result;
        }
        rmSync(join(root, 'lifecycle.jsonl'), {force: true});
        return response;
      }});
  } finally {rmSync(root, {recursive: true, force: true});}
}

for (const [subject, expected] of [
  ['feat(REM-50): example', 'minor'], ['fix(REM-50): example', 'patch'],
  ['perf(REM-50): example', 'patch'], ['feat(REM-50)!: example', 'major'],
  ['docs: example', null], ['docs: example\n\nBREAKING CHANGE: fixture only', null],
  ['build: dependency update', 'patch'], ['ci: build update', 'patch'],
  ['chore: app change', 'patch'], ['refactor!: app change', 'major'],
]) {
  test(`actual upstream analyzer: ${subject.split('\n')[0]}`, async () => {
    assert.equal(await analyzeCommits(analyzer, {cwd: process.cwd(), commits: [{message: subject, hash: 'fixture'}], logger: quiet}), expected);
  });
}

test('actual semantic-release: same-second feature → tagged feature → fix', async () => {
  await withRepo(async repo => {
    repo.commit('feat(REM-50): new API');
    const feature = await repo.release();
    assert.equal(feature.nextRelease.version, '0.2.0');
    assert.equal(feature.nextRelease.gitHead, repo.git('rev-parse', 'HEAD'));
    repo.git('tag', 'v0.2.0'); repo.git('push', 'origin', '--tags');
    repo.commit('fix(REM-50): fix API');
    const fix = await repo.release();
    assert.equal(fix.nextRelease.version, '0.2.1');
    assert.equal(fix.nextRelease.gitHead, repo.git('rev-parse', 'HEAD'));
    assert.equal(repo.git('log', '--format=%ct').split('\n').every(t => t === repo.git('log', '-1', '--format=%ct')), true);
  });
});

test('actual semantic-release: docs-only no release, next feature releases', async () => {
  await withRepo(async repo => {
    repo.commit('docs: baseline guide');
    assert.equal(await repo.release(), false);
    repo.commit('feat(REM-50): implemented change');
    assert.equal((await repo.release()).nextRelease.version, '0.2.0');
  });
});

test('actual semantic-release: repeat dry run preserves version and old tags', async () => {
  await withRepo(async repo => {
    const base = repo.git('rev-parse', 'v0.1.0');
    repo.commit('fix(REM-50): example');
    assert.equal((await repo.release()).nextRelease.version, '0.1.1');
    assert.equal((await repo.release()).nextRelease.version, '0.1.1');
    assert.equal(repo.git('rev-parse', 'v0.1.0'), base);
  });
});

// Assert own versions remain absent even after a clean dependency install.
test('tooling manifests do not maintain a project version', () => {
  for (const name of ['package.json', 'package-lock.json']) {
    const manifest = JSON.parse(readFileSync(new URL(name, import.meta.url), 'utf8'));
    assert.equal(Object.hasOwn(manifest, 'version'), false, name);
    if (manifest.packages) assert.equal(Object.hasOwn(manifest.packages[''], 'version'), false, name);
  }
});

test('actual upstream lifecycle creates tag before publish, after prepare', async () => {
  await withRepo(async repo => {
    repo.commit('feat: fixture API');
    const sha = repo.git('rev-parse', 'HEAD');
    const observation = await repo.release('observe');
    assert.equal(observation.error, undefined);
    assert.deepEqual(observation.events.map(e => e.phase), ['prepare', 'publish']);
    assert.equal(observation.events[0].tagHead, null);
    assert.equal(observation.events[1].tagHead, sha);
    assert.equal(observation.events[1].version, observation.events[1].tag.slice(1));
    assert.equal(observation.result.nextRelease.gitHead, sha);
  });
});

test('actual upstream retry skips already tagged failed publication', async () => {
  await withRepo(async repo => {
    repo.commit('fix: fixture API');
    const failure = await repo.release('fail');
    assert.equal(failure.error, 'Owned fixture publication failure');
    assert.equal(failure.events.at(-1).tagHead, repo.git('rev-parse', 'HEAD'));
    const tags = repo.git('show-ref', '--tags');
    const retry = await repo.release('observe');
    assert.equal(retry.error, undefined);
    assert.equal(retry.result, false);
    assert.deepEqual(retry.events, []);
    assert.equal(repo.git('show-ref', '--tags'), tags);
  });
});

test('actual upstream refuses stale main checkout before tag/publication', async () => {
  await withRepo(async repo => {
    repo.commit('feat: fixture API');
    repo.advanceRemote('fix: newer main');
    const tags = repo.git('show-ref', '--tags');
    const stale = await repo.release('observe');
    assert.equal(stale.error, undefined);
    assert.equal(stale.result, false);
    assert.deepEqual(stale.events, []);
    assert.equal(repo.git('show-ref', '--tags'), tags);
  });
});
