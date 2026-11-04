"""Actual tracked SDK Git transport; temporary tags only, no runtime-version hook."""
import io
import json
import os
from pathlib import Path
import shutil
import subprocess
import tarfile
import tempfile
import unittest

from stage_source import Refusal, sdk_build_environment, verify_sdk_dependency
from test_cargo_consumer import fixture_environment


ROOT = Path(__file__).resolve().parents[2]


class CurrentGitConsumer(unittest.TestCase):
    def test_current_sdk_git_pin_default_features_and_development_patch(self):
        cargo = os.environ['SDK_FIXTURE_CARGO']
        with tempfile.TemporaryDirectory(prefix='sdk-current-git-owned-') as directory:
            root = Path(directory)
            env = fixture_environment(root)
            sdk = root / 'sdk'
            sdk.mkdir()

            def command(argv, cwd):
                result = subprocess.run(
                    argv, cwd=cwd, env=env, text=True, stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE, timeout=120,
                )
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                return result.stdout.strip()

            # git archive excludes untracked/private files and existing build output.
            source_sha = command(['git', 'rev-parse', 'HEAD'], ROOT)
            archive = subprocess.check_output(
                ['git', 'archive', source_sha], cwd=ROOT, env=env, timeout=30,
            )
            with tarfile.open(fileobj=io.BytesIO(archive)) as source:
                source.extractall(sdk, filter='data')
            command(['git', 'init', '--initial-branch=main'], sdk)
            command(['git', 'add', '.'], sdk)
            command(['git', 'commit', '-m', 'test: owned current SDK snapshot'], sdk)
            sha = command(['git', 'rev-parse', 'HEAD'], sdk)
            tag = 'v0.3.7'
            command(['git', 'tag', tag], sdk)
            url = sdk.as_uri()
            original_lock = (sdk / 'Cargo.lock').read_bytes()
            consumer = root / 'consumer'
            shutil.copytree(sdk / 'tests/fixtures/consumer-boundary', consumer)
            manifest = consumer / 'Cargo.toml'
            text = manifest.read_text()
            original = 'path = "../../..", default-features = false'
            self.assertEqual(text.count(original), 1)
            manifest.write_text(text.replace(
                original, 'git = ' + json.dumps(url) + ', tag = ' + json.dumps(tag)
                + ', default-features = false',
            ))

            def run(*args):
                return command([cargo, *args], consumer)

            def metadata():
                return json.loads(run('metadata', '--format-version', '1', '--locked', '--offline'))

            run('generate-lockfile')
            data = metadata()
            identity = verify_sdk_dependency(data, 'remarkable-open-sdk', url, tag, sha)
            package = next(p for p in data['packages'] if p['name'] == 'remarkable-open-sdk')
            node = next(n for n in data['resolve']['nodes'] if n['id'] == package['id'])
            self.assertEqual(node['features'], [])
            self.assertEqual(package['version'], '0.0.0')
            self.assertIn('#' + sha, (consumer / 'Cargo.lock').read_text())
            self.assertEqual(sdk_build_environment(data, 'remarkable-open-sdk', url, tag, sha), {
                'SDK_BUILD_TAG': tag, 'SDK_BUILD_SHA': sha, 'SDK_BUILD_VERSION': '0.3.7',
            })
            expected = 'ordinary-consumer: identity and pure geometry; no SDK features or native dispatch'
            self.assertEqual(run('run', '--quiet', '--locked', '--offline', '--bin', 'consumer'), expected)
            command(['git', 'commit', '--allow-empty', '-m', 'test: advanced fixture branch'], sdk)
            self.assertEqual(verify_sdk_dependency(metadata(), 'remarkable-open-sdk', url, tag, sha), identity)

            local = root / 'local-sdk'
            shutil.copytree(sdk, local, ignore=shutil.ignore_patterns('.git'))
            with (local / 'src/lib.rs').open('a') as source:
                source.write('\n// Owned local development override.\n')
            config = consumer / '.cargo/config.toml'
            config.parent.mkdir()
            config.write_text('[patch.' + json.dumps(url) + ']\nremarkable-open-sdk={path='
                              + json.dumps(str(local)) + '}\n')
            run('generate-lockfile', '--offline')
            patched = metadata()
            selected = next(p for p in patched['packages'] if p['name'] == 'remarkable-open-sdk')
            self.assertEqual(Path(selected['manifest_path']), local / 'Cargo.toml')
            self.assertEqual(run('run', '--quiet', '--locked', '--offline', '--bin', 'consumer'), expected)
            with self.assertRaises(Refusal):
                sdk_build_environment(patched, 'remarkable-open-sdk', url, tag, sha)
            config.unlink()
            run('generate-lockfile', '--offline')
            self.assertEqual(verify_sdk_dependency(metadata(), 'remarkable-open-sdk', url, tag, sha), identity)
            self.assertEqual(run('run', '--quiet', '--locked', '--offline', '--bin', 'consumer'), expected)
            self.assertEqual((sdk / 'Cargo.lock').read_bytes(), original_lock)
            self.assertEqual(command(['git', 'status', '--porcelain'], sdk), '')
            print('current-sdk-git-consumer: exact tag/lock, empty features, local patch refusal/restoration PASS')
