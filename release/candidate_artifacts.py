"""Build/verify candidate artifacts only; no version calculation, tag or publisher."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tarfile

from stage_source import Refusal, stage_candidate, verify_staged_source, distribution_manifest
from verify_assets import verify_downloaded_assets

TARGETS = ('x86_64-unknown-linux-gnu', 'armv7-unknown-linux-gnueabihf', 'aarch64-unknown-linux-gnu')


def digest(path):
    if path.is_symlink() or not path.is_file():
        raise Refusal('Artifact must be a regular file')
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def command(cwd, *args):
    try:
        return subprocess.run(args, cwd=cwd, check=True, capture_output=True, text=True, timeout=600).stdout
    except subprocess.CalledProcessError as error:
        print((error.stderr or '')[-8192:], file=sys.stderr)
        raise


def build(source, sha, tag, output, cargo='cargo'):
    source, output = source.resolve(), output.resolve()
    if output.exists():
        raise Refusal('Candidate output must be new')
    # Fetch exact source dependencies before generating the build-only own version.
    command(source, cargo, 'fetch', '--locked')
    output.mkdir(mode=0o700)
    (output / '.owned-candidate').write_text('Candidate build only; final remote tag required')
    stage_dir = output / 'stage'
    identity = stage_candidate(source, tag, sha, stage_dir)
    command(stage_dir, cargo, 'generate-lockfile', '--offline')
    verify_staged_source(stage_dir, allow_candidate=True)
    assets, tests = output / 'release', output / 'tests'
    assets.mkdir()
    tests.mkdir()
    for target in TARGETS:
        command(stage_dir, cargo, 'build', '--locked', '--offline', '--release', '--all-features', '--lib', '--target', target)
    test_messages = command(stage_dir, cargo, 'test', '--locked', '--offline', '--all-features', '--no-run', '--message-format=json')
    executables = []
    for line in test_messages.splitlines():
        value = json.loads(line)
        if value.get('reason') == 'compiler-artifact' and value.get('profile', {}).get('test') and value.get('executable'):
            executable = Path(value['executable'])
            if not executable.resolve().is_relative_to((stage_dir / 'target').resolve()):
                raise Refusal('Compiler test executable escaped build output')
            copied = tests / executable.name
            if copied.exists():
                raise Refusal('Ambiguous test executable name')
            shutil.copyfile(executable, copied)
            copied.chmod(0o700)
            executables.append({'name': copied.name, 'sha256': digest(copied)})
    if not executables:
        raise Refusal('No actual host test executable was built')
    command(stage_dir, cargo, 'package', '--locked', '--offline', '--no-verify')
    crate = stage_dir / 'target/package' / f"{identity['package_name']}-{identity['build_version']}.crate"
    manifest = distribution_manifest(stage_dir, crate, allow_candidate=True)
    shutil.copyfile(crate, assets / crate.name)
    stem = 'lib' + identity['package_name'].replace('-', '_') + '.rlib'
    for target in TARGETS:
        artifact = stage_dir / 'target' / target / 'release' / stem
        name = f"{identity['package_name']}-{identity['build_version']}-{target}.rlib"
        shutil.copyfile(artifact, assets / name)
        manifest['distributions'].append({'kind': 'rust-compiler-library', 'target': target, 'name': name,
                                          'sha256': digest(assets / name), 'size': (assets / name).stat().st_size})
    manifest['rustc'] = command(source, 'rustc', '-Vv').strip()
    manifest['library_abi'] = 'requires matching compiler/dependency graph; native operations unsupported'
    (assets / 'manifest.json').write_text(json.dumps(manifest, sort_keys=True, indent=2) + '\n')
    verify_downloaded_assets(manifest, assets)
    receipt = {'schema': 1, 'source_sha': sha, 'candidate_tag': tag, 'package_name': identity['package_name'],
               'manifest_sha256': digest(assets / 'manifest.json'), 'host_tests': executables}
    (output / 'build-receipt.json').write_text(json.dumps(receipt, sort_keys=True, indent=2) + '\n')
    # Transfer only tested artifacts/receipt; no full build tree, keys or caches.
    shutil.rmtree(stage_dir)
    if command(source, 'git', 'status', '--porcelain').strip():
        raise Refusal('Candidate build changed source')
    return receipt


def verify(output, sha, tag, receipt_sha, *, execute_tests=False):
    output = output.resolve()
    receipt_path = output / 'build-receipt.json'
    if digest(receipt_path) != receipt_sha:
        raise Refusal('Build receipt differs from trusted build-job output')
    receipt = json.loads(receipt_path.read_text())
    if receipt.get('source_sha') != sha or receipt.get('candidate_tag') != tag or receipt.get('schema') != 1:
        raise Refusal('Candidate/source receipt identity differs')
    assets = output / 'release'
    if digest(assets / 'manifest.json') != receipt['manifest_sha256']:
        raise Refusal('Manifest differs from trusted build receipt')
    manifest = json.loads((assets / 'manifest.json').read_text())
    if manifest.get('source_sha') != sha or manifest.get('git_tag') != tag or manifest.get('build_identity') != 'candidate':
        raise Refusal('Build manifest candidate/source differs')
    verify_downloaded_assets(manifest, assets)
    test_names = set()
    for entry in receipt.get('host_tests', []):
        name = entry.get('name')
        if not isinstance(name, str) or Path(name).name != name or name in test_names:
            raise Refusal('Invalid or duplicate host test name')
        test_names.add(name)
        path = output / 'tests' / name
        if digest(path) != entry['sha256']:
            raise Refusal('Transferred host test bytes differ')
        if execute_tests:
            path.chmod(0o700)
            report = command(output, str(path))
            summaries = [line for line in report.splitlines() if line.startswith('test result:')]
            if not summaries:
                raise Refusal('Built host test executable did not report completion')
            print(json.dumps({'built_test': name, 'completion': summaries}))
    if not test_names or {p.name for p in (output / 'tests').iterdir()} != test_names:
        raise Refusal('Missing or undeclared host test artifact')
    # Inspect the actual transferred source crate rather than a second source checkout.
    crates = [e for e in manifest['distributions'] if e['kind'] == 'rust-source-crate']
    if len(crates) != 1:
        raise Refusal('Exactly one source crate is required')
    import tomllib
    with tarfile.open(assets / crates[0]['name'], 'r:gz') as archive:
        prefix = receipt['package_name'] + '-' + tag[1:] + '/'
        member = archive.getmember(prefix + 'Cargo.toml')
        if not member.isfile() or member.size > 65536:
            raise Refusal('Invalid packaged manifest')
        package = tomllib.loads(archive.extractfile(member).read().decode())['package']
        if package['name'] != receipt['package_name'] or package['version'] != tag[1:]:
            raise Refusal('Transferred source crate identity differs')
    verify_downloaded_assets(manifest, assets)
    return manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('mode', choices=('build', 'verify', 'integration'))
    parser.add_argument('--source', type=Path, default=Path.cwd())
    parser.add_argument('--sha', required=True)
    parser.add_argument('--tag', required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--receipt-sha')
    parser.add_argument('--cargo', default='cargo')
    args = parser.parse_args()
    if args.mode == 'build':
        receipt = build(args.source, args.sha, args.tag, args.output, args.cargo)
        value = {'receipt_sha': digest(args.output / 'build-receipt.json'), 'manifest_sha': receipt['manifest_sha256']}
        if os.environ.get('GITHUB_OUTPUT'):
            with open(os.environ['GITHUB_OUTPUT'], 'a') as stream:
                for key, item in value.items():
                    stream.write(f'{key}={item}\n')
        print(json.dumps(value))
    else:
        verify(args.output, args.sha, args.tag, args.receipt_sha, execute_tests=args.mode == 'integration')
        print(json.dumps({'verified_source': args.sha, 'candidate': args.tag, 'executed_built_tests': args.mode == 'integration'}))


if __name__ == '__main__':
    main()
