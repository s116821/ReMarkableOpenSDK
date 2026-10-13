"""Build exact maintained publisher in an owned fixture; never publish releases."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess

UPSTREAM = 'efb35369e0ad2afab669f228072c1b0d510eae64'
LOCK_SHA256 = '27a1b2c58e814fa3bc18294a0c379fcd8d51e2ea543f4b12a883f4046ab69a59'
BUNDLE_SHA256 = 'fe5e64d6575b09f66f626d7d09fb1b9911376dfcf5f72cc24e838a39e0882f59'
PACKAGE_SHA256 = '1e72c659fd8294d3c4ca68b06546d0bae7e8eb0fa9bb90b187629060ca257d88'
LOCK = Path(__file__).with_name('vendor-locks') / 'softprops-action-gh-release.package-lock.json'


def digest(data):
    return hashlib.sha256(data).hexdigest()


def prepare(source, output, lock=LOCK):
    source = source.resolve()
    head = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=source, text=True).strip()
    if head != UPSTREAM or subprocess.check_output(['git', 'status', '--porcelain'], cwd=source, text=True):
        raise ValueError('Upstream checkout must be exact and clean')
    package = subprocess.check_output(['git', 'show', f'{UPSTREAM}:package.json'], cwd=source)
    if digest(package) != PACKAGE_SHA256 or digest(lock.read_bytes()) != LOCK_SHA256:
        raise ValueError('Publisher source/lock identity mismatch')
    original = json.loads(package)
    resolved = json.loads(lock.read_bytes())
    root = resolved['packages']['']
    for key in ('name', 'version', 'dependencies', 'devDependencies'):
        if root.get(key) != original.get(key):
            raise ValueError('Upstream supported root ranges changed')
    if output.exists() or not output.is_absolute():
        raise ValueError('Use a new absolute owned fixture directory')
    output.mkdir(mode=0o700)
    (output / '.owned-fixture').write_text('Source-built publisher only; no production credentials/publication')
    built = output / 'publisher'
    built.mkdir()
    paths = subprocess.check_output(['git', 'ls-tree', '-r', '--name-only', UPSTREAM], cwd=source, text=True).splitlines()
    for name in paths:
        path = Path(name)
        if path.is_absolute() or '..' in path.parts:
            raise ValueError('Unsafe upstream tracked path')
        mode = subprocess.check_output(['git', 'ls-tree', UPSTREAM, '--', name], cwd=source, text=True).split()[0]
        if mode not in ('100644', '100755'):
            raise ValueError('Unsupported upstream file kind')
        target = built / path
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(subprocess.check_output(['git', 'show', f'{UPSTREAM}:{name}'], cwd=source))
        target.chmod(0o755 if mode == '100755' else 0o644)
    (built / 'package-lock.json').write_bytes(lock.read_bytes())
    return built, paths


def qualify(source, output):
    built, paths = prepare(source, output)
    env = {key: os.environ[key] for key in ('PATH', 'SystemRoot', 'HOME', 'USERPROFILE', 'LOCALAPPDATA', 'TEMP', 'TMP', 'TMPDIR', 'LANG') if key in os.environ}
    config = output / 'empty.npmrc'
    config.write_text('')
    env.update({'CI': 'true', 'npm_config_userconfig': str(config), 'npm_config_globalconfig': str(output / 'empty-global.npmrc')})
    (output / 'empty-global.npmrc').write_text('')
    npm = shutil.which('npm', path=env.get('PATH'))
    if not npm:
        raise ValueError('Supported Node24/npm prerequisite unavailable')
    def run(*args, capture=False):
        return subprocess.run([npm, *args], cwd=built, env=env, check=True, timeout=300,
                              stdout=subprocess.PIPE if capture else None, text=capture)
    run('ci', '--ignore-scripts', '--no-fund')
    audit = json.loads(run('audit', '--json', capture=True).stdout)
    if audit['metadata']['vulnerabilities']['total'] != 0:
        raise ValueError('Publisher dependency audit is not clean')
    run('run', 'typecheck')
    run('test', '--', '--run')
    run('run', 'build')
    for name in paths:
        if name in ('package-lock.json', 'dist/index.js'):
            continue
        expected = subprocess.check_output(['git', 'show', f'{UPSTREAM}:{name}'], cwd=source)
        if (built / name).read_bytes() != expected:
            raise ValueError('Publisher check changed upstream source/license')
    if digest((built / 'dist/index.js').read_bytes()) != BUNDLE_SHA256:
        raise ValueError('Source-built publisher bytes differ from qualified bundle')
    node = subprocess.check_output(['node', '--version'], env=env, text=True).strip()
    if not node.startswith('v24.'):
        raise ValueError('Node24 source build required')
    receipt = {'schema': 1, 'candidate': 'maintained-source-built-publisher', 'upstream_commit': UPSTREAM,
               'package_json_sha256': PACKAGE_SHA256, 'lock_sha256': LOCK_SHA256,
               'bundle_sha256': digest((built / 'dist/index.js').read_bytes()), 'node': node,
               'known_dependency_advisories': 0, 'upstream_source_unchanged': True,
               'official_upstream_bundle': False, 'production_selected': False}
    (output / 'source-built-receipt.json').write_text(json.dumps(receipt, sort_keys=True, indent=2) + '\n')
    print(json.dumps(receipt, sort_keys=True))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    qualify(args.source, args.output)
