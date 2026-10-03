// Upstream calls are isolated from Node's test reporter and confined to owned repos.
import assert from 'node:assert/strict';
import {readFileSync, existsSync, realpathSync} from 'node:fs';
import {dirname, join, basename} from 'node:path';
import {tmpdir} from 'node:os';
import {fileURLToPath} from 'node:url';
import {Writable} from 'node:stream';
import semanticRelease from 'semantic-release';
import {analyzer} from './policy.mjs';
const sink = () => new Writable({write(_chunk, _encoding, done) {done();}});
const [cwd, repositoryUrl, mode = 'dry'] = process.argv.slice(2);
const root = dirname(realpathSync(cwd));
assert.equal(dirname(root), realpathSync(tmpdir()));
assert.equal(basename(root).startsWith('sdk-semantic-owned-'), true);
assert.equal(realpathSync(cwd), join(root, 'work'));
assert.equal(realpathSync(repositoryUrl), join(root, 'remote.git'));
assert.equal(readFileSync(join(root, '.owned-fixture'), 'utf8'), 'REM-50 owned fixture\n');
assert.equal(['dry', 'observe', 'fail'].includes(mode), true);
const eventsFile = join(root, 'lifecycle.jsonl');
const plugins = [['@semantic-release/commit-analyzer', analyzer]];
if (mode !== 'dry') plugins.push([fileURLToPath(new URL('./lifecycle-fixture.mjs', import.meta.url)),
  {eventsFile, failPublication: mode === 'fail'}]);
let result, error;
try {
  result = await semanticRelease({repositoryUrl, branches: ['main'],
    tagFormat: 'v${version}', dryRun: mode === 'dry', ci: false, plugins},
    {cwd, env: process.env, stdout: sink(), stderr: sink()});
} catch (e) {error = e.message;}
const events = existsSync(eventsFile) ? readFileSync(eventsFile, 'utf8').trim().split('\n').filter(Boolean).map(JSON.parse) : [];
process.stdout.write(JSON.stringify({result, error, events}));
