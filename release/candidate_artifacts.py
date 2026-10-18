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


def command(cwd, *args, env=None):
    try:
        return subprocess.run(args, cwd=cwd, env=env, check=True, capture_output=True, text=True, timeout=600).stdout
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
    assets, probes = output / 'release', output / 'probes'
    assets.mkdir()
    probes.mkdir()
    for target in TARGETS:
        command(stage_dir, cargo, 'build', '--locked', '--offline', '--release', '--all-features', '--lib', '--target', target)
    # Unit suites belong to Checks. Integration consumes this distinct external
    # consumer linked against the exact release library and dependency graph.
    stem = 'lib' + identity['package_name'].replace('-', '_') + '.rlib'
    host = stage_dir / 'target' / TARGETS[0] / 'release'
    probe = probes / 'sdk-consumer-probe'
    command(source, 'rustc', '--edition=2024', '--target', TARGETS[0],
            str(source / 'release/sdk_consumer_probe.rs'),
            '--extern', 'remarkable_open_sdk=' + str(host / stem),
            '-L', 'dependency=' + str(host / 'deps'),
            '-L', 'dependency=' + str(stage_dir / 'target/release/deps'), '-o', str(probe),
            env={**os.environ, 'SDK_PROBE_TAG': tag, 'SDK_PROBE_SHA': sha})
    probe.chmod(0o700)
    executables = [{'name': probe.name, 'sha256': digest(probe)}]
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
    receipt = {'schema': 2, 'source_sha': sha, 'candidate_tag': tag, 'package_name': identity['package_name'],
               'manifest_sha256': digest(assets / 'manifest.json'), 'host_probes': executables}
    (output / 'build-receipt.json').write_text(json.dumps(receipt, sort_keys=True, indent=2) + '\n')
    # Transfer only tested artifacts/receipt; no full build tree, keys or caches.
    shutil.rmtree(stage_dir)
    if command(source, 'git', 'status', '--porcelain').strip():
        raise Refusal('Candidate build changed source')
    return receipt


def verify(output, sha, tag, receipt_sha, *, execute_probes=False):
    output = output.resolve()
    receipt_path = output / 'build-receipt.json'
    if digest(receipt_path) != receipt_sha:
        raise Refusal('Build receipt differs from trusted build-job output')
    receipt = json.loads(receipt_path.read_text())
    if receipt.get('source_sha') != sha or receipt.get('candidate_tag') != tag or receipt.get('schema') != 2:
        raise Refusal('Candidate/source receipt identity differs')
    assets = output / 'release'
    if digest(assets / 'manifest.json') != receipt['manifest_sha256']:
        raise Refusal('Manifest differs from trusted build receipt')
    manifest = json.loads((assets / 'manifest.json').read_text())
    if manifest.get('source_sha') != sha or manifest.get('git_tag') != tag or manifest.get('build_identity') != 'candidate':
        raise Refusal('Build manifest candidate/source differs')
    verify_downloaded_assets(manifest, assets)
    probe_entries = receipt.get('host_probes', [])
    if len(probe_entries) != 1:
        raise Refusal('Exactly one external consumer probe is required')
    probe_names = set()
    for entry in probe_entries:
        name = entry.get('name')
        if not isinstance(name, str) or Path(name).name != name or name in probe_names:
            raise Refusal('Invalid or duplicate consumer probe name')
        probe_names.add(name)
        if digest(output / 'probes' / name) != entry['sha256']:
            raise Refusal('Transferred consumer probe bytes differ')
    if {p.name for p in (output / 'probes').iterdir()} != probe_names:
        raise Refusal('Missing or undeclared consumer probe artifact')
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
    if execute_probes:
        path = output / 'probes' / probe_entries[0]['name']
        path.chmod(0o700)
        observed = json.loads(command(output, str(path), tag, sha))
        expected = {'probe': 'sdk-consumer', 'candidate': tag, 'source': sha,
                    'native_operations': 'unsupported'}
        if observed != expected:
            raise Refusal('Retained external consumer identity differs')
        print(json.dumps({'retained_consumer': observed}))
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
        verify(args.output, args.sha, args.tag, args.receipt_sha, execute_probes=args.mode == 'integration')
        print(json.dumps({'verified_source': args.sha, 'candidate': args.tag, 'executed_retained_consumer': args.mode == 'integration'}))


if __name__ == '__main__':
    main()
