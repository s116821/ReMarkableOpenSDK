// Actual upstream dry run in a separate process because semantic-release hooks stdout.
import {Writable} from 'node:stream';
import semanticRelease from 'semantic-release';
import {analyzer} from './policy.mjs';
const sink = () => new Writable({write(_chunk, _encoding, done) {done();}});
const [cwd, repositoryUrl] = process.argv.slice(2);
const result = await semanticRelease({repositoryUrl, branches: ['main'],
  tagFormat: 'v${version}', dryRun: true, ci: false,
  plugins: [['@semantic-release/commit-analyzer', analyzer]]},
  {cwd, env: process.env, stdout: sink(), stderr: sink()});
process.stdout.write(JSON.stringify(result));
