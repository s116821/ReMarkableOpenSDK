// Actual upstream Action bundle; fixture code supplies inputs and checks outputs only.
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import {mkdtempSync, mkdirSync, writeFileSync, readFileSync, rmSync, renameSync} from 'node:fs';
import {dirname, join, resolve} from 'node:path';
import {tmpdir} from 'node:os';
import test from 'node:test';
const action = process.env.PATHS_FILTER_ACTION;
assert.ok(action, 'Set PATHS_FILTER_ACTION to the pinned dorny/paths-filter dist/index.js');
const bundle = resolve(action);
const filters = readFileSync(new URL('../.github/sdk-paths.yml', import.meta.url), 'utf8');
for (const [name, files, expected, rename, push] of [
  ['root guide', ['README.md'], false],
  ['OpenSpec requirement', ['openspec/changes/demo/specs/releases/spec.md'], false],
  ['former inventory documentation', ['docs/reference/dependency-license-inventory.json'], false],
  ['named release guidance', ['release/README.md'], false],
  ['license inventory input', ['tools/dependency-license-inventory.json'], true],
  ['documentation asset', ['docs/screenshots/sample.png'], false],
  ['Rust source', ['src/lib.rs'], true],
  ['nested source Markdown', ['src/README.md'], true],
  ['dependency lock', ['Cargo.lock'], true],
  ['workflow', ['.github/workflows/build.yml'], true],
  ['hidden build config', ['.cargo/config.toml'], true],
  ['unknown path', ['unknown/data'], true],
  ['mixed docs and code', ['README.md', 'src/lib.rs'], true],
  ['documentation renamed into source', ['src/example.rs'], true, 'docs/example.md'],
  ['source renamed into documentation', ['docs/example.md'], true, 'src/example.rs'],
  ['main push documentation', ['README.md'], false, null, true],
  ['main push mixed', ['README.md', 'src/lib.rs'], true, null, true],
  ['main push docs renamed into source', ['src/example.rs'], true, 'docs/example.md', true],
  ['main push source renamed into docs', ['docs/example.md'], true, 'src/example.rs', true],
]) {
  test(`actual paths-filter: ${name}`, () => {
    const cwd = mkdtempSync(join(tmpdir(), 'sdk-paths-owned-'));
    const output = join(cwd, '.action-output');
    const env = {PATH: process.env.PATH, PATHEXT: process.env.PATHEXT, GIT_CONFIG_NOSYSTEM: '1', GIT_CONFIG_GLOBAL: '/dev/null',
      GIT_AUTHOR_NAME: 'Owned Fixture', GIT_COMMITTER_NAME: 'Owned Fixture',
      GIT_AUTHOR_EMAIL: 'fixture@example.invalid', GIT_COMMITTER_EMAIL: 'fixture@example.invalid',
      GITHUB_OUTPUT: output, GITHUB_EVENT_NAME: 'workflow_dispatch',
      GITHUB_REPOSITORY: 'fixture/owned', INPUT_BASE: 'HEAD', INPUT_TOKEN: '',
      INPUT_FILTERS: filters, 'INPUT_PREDICATE-QUANTIFIER': 'every'};
    const git = (...args) => execFileSync('git', args, {cwd, env, stdio: 'pipe'});
    const put = path => {mkdirSync(dirname(join(cwd, path)), {recursive: true}); writeFileSync(join(cwd, path), 'owned fixture\n');};
    try {
      git('init', '--initial-branch=main');
      if (rename) {put(rename); git('add', '--', rename);}
      git('commit', '--allow-empty', '-m', 'chore: fixture baseline');
      const before = git('rev-parse', 'HEAD').toString().trim();
      if (rename) {
        mkdirSync(dirname(join(cwd, files[0])), {recursive: true});
        renameSync(join(cwd, rename), join(cwd, files[0]));
        git('add', '--all');
      } else {for (const file of files) put(file); git('add', '--', ...files);}
      if (push) {
        git('commit', '-m', 'chore: owned fixture change');
        const after = git('rev-parse', 'HEAD').toString().trim();
        const event = join(cwd, '.push-event.json');
        writeFileSync(event, JSON.stringify({before, after, ref: 'refs/heads/main', repository: {default_branch: 'main'}}));
        Object.assign(env, {GITHUB_EVENT_NAME: 'push', GITHUB_EVENT_PATH: event,
          GITHUB_REF: 'refs/heads/main', GITHUB_SHA: after, INPUT_BASE: before});
      }
      writeFileSync(output, '');
      try {
        execFileSync(process.execPath, [bundle], {cwd, env, stdio: 'pipe', timeout: 15000, maxBuffer: 1024 * 1024});
      } catch (e) {
        assert.fail(`Owned fixture Action failed: ${String(e.stdout).slice(0, 4000)} ${String(e.stderr).slice(0, 1000)}`);
      }
      const text = readFileSync(output, 'utf8').replaceAll('\r\n', '\n');
      assert.match(text, new RegExp(`^application<<[^\\n]+\\n${expected}\\n`, 'm'));
    } finally {rmSync(cwd, {recursive: true, force: true});}
  });
}
