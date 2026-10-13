// Actual pinned title Action, with a local read-only API fixture and no real token.
import assert from 'node:assert/strict';
import {createServer} from 'node:http';
import {execFile} from 'node:child_process';
import {promisify} from 'node:util';
import {mkdtempSync, writeFileSync, readFileSync, rmSync} from 'node:fs';
import {join, resolve} from 'node:path';
import {tmpdir} from 'node:os';
import test from 'node:test';
const action = process.env.SEMANTIC_PR_ACTION;
assert.ok(action, 'Set SEMANTIC_PR_ACTION to the pinned title Action dist/index.js');
const bundle = resolve(action);
const workflow = readFileSync(new URL('../.github/workflows/release-qualification.yml', import.meta.url), 'utf8');
const configuredTypes = workflow.match(/types:.*&& '([^']+)' \|\| '([^']+)'/);
assert.ok(configuredTypes, 'Read the actual workflow type choices');
const exec = promisify(execFile);
for (const [title, application, accepted] of [
  ['ci(REM-50): qualify release tools', true, true],
  ['docs(REM-50): application changed', true, false],
  ['docs(REM-50): guide only', false, true],
  ['docs(REM-50)!: application changed', true, false],
  ['feat(api)!: change contract', true, true],
  ['fix: missing scope', true, false],
  ['arbitrary title', false, false],
]) {
  test(`actual semantic PR Action: ${title} (application=${application})`, async () => {
    const root = mkdtempSync(join(tmpdir(), 'sdk-title-owned-'));
    const requests = [];
    const server = createServer((req, res) => {
      requests.push([req.method, req.url]);
      if (req.method !== 'GET' || req.url !== '/repos/fixture/owned/pulls/1') {
        res.writeHead(404); res.end('{}'); return;
      }
      res.setHeader('content-type', 'application/json');
      res.end(JSON.stringify({title, labels: [], head: {sha: 'owned-fixture'}}));
    });
    try {
      await new Promise(resolve => server.listen(0, '127.0.0.1', resolve));
      const event = join(root, 'event.json');
      writeFileSync(event, JSON.stringify({pull_request: {number: 1,
        title: 'docs(REM-50): stale event title', base: {user: {login: 'fixture'}, repo: {name: 'owned'}}}}));
      const env = {PATH: process.env.PATH, GITHUB_EVENT_NAME: 'pull_request',
        GITHUB_EVENT_PATH: event, GITHUB_REPOSITORY: 'fixture/owned',
        GITHUB_TOKEN: 'owned-fixture-not-a-credential',
        INPUT_GITHUBBASEURL: `http://127.0.0.1:${server.address().port}`,
        INPUT_TYPES: configuredTypes[application ? 1 : 2], INPUT_REQUIRESCOPE: 'true'};
      let status = 0;
      try {await exec(process.execPath, [bundle], {cwd: root, env, timeout: 15000, maxBuffer: 1024 * 1024});}
      catch (e) {status = e.code; assert.equal(status, 1, 'Failure must be validation, not timeout/spawn error');}
      assert.equal(status === 0, accepted);
      assert.deepEqual(requests, [['GET', '/repos/fixture/owned/pulls/1']], 'Action uses current API title and only reads');
    } finally {
      await new Promise(resolve => server.close(resolve));
      rmSync(root, {recursive: true, force: true});
    }
  });
}
