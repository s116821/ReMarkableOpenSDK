// Test observer only; no release calculations, build commands, or external publisher.
import {appendFileSync} from 'node:fs';
import {execFileSync} from 'node:child_process';
function record(options, context, phase) {
  const {cwd, env, nextRelease} = context;
  let tagHead = null;
  try {
    tagHead = execFileSync('git', ['rev-parse', '--verify', `refs/tags/${nextRelease.gitTag}`],
      {cwd, env, encoding: 'utf8', stdio: ['ignore', 'pipe', 'pipe']}).trim();
  } catch {}
  appendFileSync(options.eventsFile, JSON.stringify({phase, tag: nextRelease.gitTag,
    version: nextRelease.version, gitHead: nextRelease.gitHead, tagHead}) + '\n');
}
export async function prepare(options, context) {record(options, context, 'prepare');}
export async function publish(options, context) {
  record(options, context, 'publish');
  if (options.failPublication) throw new Error('Owned fixture publication failure');
  return {name: 'Owned fixture observation'};
}
