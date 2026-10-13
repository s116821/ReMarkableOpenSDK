"""Maintained clean tag/build/publisher composition, disposable Git and loopback only."""
import json
import os
from pathlib import Path
import shutil
import subprocess
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from threading import Thread
import unittest
from urllib.parse import urlsplit, parse_qs

import python_semantic_release_fixtures as tagger_fixtures
from remote_identity import verify_remote_tag
from stage_source import stage, verify_staged_source, distribution_manifest, Refusal
from verify_assets import verify_downloaded_assets


class CleanPublisherComposition(unittest.TestCase):
    def setUp(self):
        self.f = tagger_fixtures.PythonSemanticReleaseCandidate()
        self.f.setUp()
        self.addCleanup(self.f.doCleanups)
        f = self.f
        for name in ('RUSTUP_HOME', 'CARGO_HOME'):
            if name in os.environ:
                f.env[name] = os.environ[name]
        f.env.setdefault('RUSTUP_HOME', str(Path.home() / '.rustup'))
        f.env['RUSTUP_TOOLCHAIN'] = '1.98.1'
        (f.repo / 'Cargo.toml').write_text('[package]\nname="owned_composition"\nedition="2024"\nlicense="MIT"\n')
        (f.repo / 'src').mkdir()
        (f.repo / 'src/main.rs').write_text('fn main() { println!("{}", env!("CARGO_PKG_VERSION")); }\n')
        f.git('add', '.')
        f.commit('chore(REM-50): owned version-free source')
        f.baseline()
        f.commit('fix(REM-50): intended owned release')
        self.sha = f.git('rev-parse', 'HEAD')
        self.build = f.root / 'build'
        self.assets = {}
        self.events = []
        self.release = None
        self.fail_archive = True
        outer = self

        class Handler(BaseHTTPRequestHandler):
            def log_message(self, *_args):
                pass

            def handle_request(self):
                url = urlsplit(self.path)
                raw = self.rfile.read(int(self.headers.get('Content-Length', '0')))
                event = {'method': self.command, 'path': url.path}
                outer.events.append(event)
                def reply(status, value):
                    self.send_response(status)
                    self.send_header('Content-Type', 'application/json')
                    self.end_headers()
                    self.wfile.write(json.dumps(value).encode())
                def current():
                    return {**outer.release, 'assets': [{'id': a['id'], 'name': name} for name, a in outer.assets.items()],
                            'upload_url': outer.origin + '/uploads{?name,label}', 'html_url': outer.origin + '/release/81'}
                root = '/repos/fixture/owned/releases'
                if self.command == 'GET' and url.path == root + '/tags/v0.4.1':
                    reply(200 if outer.release and not outer.release['draft'] else 404,
                          current() if outer.release and not outer.release['draft'] else {'message': 'Not Found'})
                elif self.command == 'GET' and url.path == root:
                    reply(200, [current()] if outer.release else [])
                elif self.command == 'POST' and url.path == root:
                    data = json.loads(raw)
                    outer.assertEqual(data['tag_name'], 'v0.4.1')
                    outer.assertEqual(data['target_commitish'], outer.sha)
                    outer.release = {**data, 'id': 81, 'draft': True}
                    reply(201, current())
                elif self.command == 'PATCH' and url.path == root + '/81':
                    outer.release.update(json.loads(raw))
                    event['draft_after'] = outer.release['draft']
                    reply(200, current())
                elif self.command == 'GET' and url.path == root + '/81/assets':
                    reply(200, [{'id': a['id'], 'name': n} for n, a in outer.assets.items()])
                elif self.command == 'POST' and url.path == '/uploads':
                    name = parse_qs(url.query)['name'][0]
                    event['name'] = name
                    if outer.fail_archive and name == 'sdk.crate':
                        reply(400, {'message': 'Owned partial upload failure'})
                    else:
                        outer.assertNotIn(name, outer.assets, 'Retained bytes must not be overwritten')
                        outer.assets[name] = {'id': 90 + len(outer.assets), 'bytes': raw}
                        reply(201, {'id': outer.assets[name]['id'], 'name': name})
                else:
                    reply(400, {'message': 'Unsupported owned fixture endpoint'})

            do_GET = do_POST = do_PATCH = do_DELETE = handle_request

        self.server = ThreadingHTTPServer(('127.0.0.1', 0), Handler)
        self.origin = f'http://127.0.0.1:{self.server.server_port}'
        Thread(target=self.server.serve_forever, daemon=True).start()
        self.addCleanup(self.server.server_close)
        self.addCleanup(self.server.shutdown)

    def build_verified(self):
        f = self.f
        verify_remote_tag(f.repo, 'v0.4.1', self.sha)
        stage(f.repo, 'v0.4.1', self.sha, self.build)
        cargo = os.environ['SDK_FIXTURE_CARGO']
        f.command(self.build, cargo, 'generate-lockfile', '--offline')
        verify_staged_source(self.build)
        compiled = f.command(self.build, cargo, 'run', '--locked', '--offline', '--quiet')
        self.assertEqual(compiled.stdout.strip(), '0.4.1')
        f.command(self.build, cargo, 'package', '--locked', '--offline')
        self.archive = f.root / 'sdk.crate'
        shutil.copyfile(self.build / 'target/package/owned_composition-0.4.1.crate', self.archive)
        self.expected = distribution_manifest(self.build, self.archive)
        self.manifest = f.root / 'manifest.json'
        self.manifest.write_text(json.dumps(self.expected))
        self.assertNotIn('version=', (f.repo / 'Cargo.toml').read_text())
        self.assertEqual(f.git('status', '--porcelain'), '')

    def invoke(self, draft, files):
        node, bundle = os.environ['SDK_FIXTURE_NODE'], os.environ['GH_RELEASE_ACTION']
        output = self.f.root / 'action-output'
        output.write_text('')
        env = {'PATH': os.environ['PATH'], 'GITHUB_REPOSITORY': 'fixture/owned', 'GITHUB_REF': 'refs/tags/v0.4.1',
               'GITHUB_API_URL': self.origin, 'GITHUB_OUTPUT': str(output), 'GITHUB_TOKEN': 'owned-fixture-not-a-credential',
               'INPUT_TAG_NAME': 'v0.4.1', 'INPUT_TARGET_COMMITISH': self.sha, 'INPUT_DRAFT': 'true' if draft else 'false',
               'INPUT_FILES': '\n'.join(map(str, files)), 'INPUT_OVERWRITE_FILES': 'false', 'INPUT_PRESERVE_ORDER': 'true',
               'INPUT_FAIL_ON_UNMATCHED_FILES': 'true'}
        return subprocess.run([node, bundle], cwd=self.f.root, env=env, capture_output=True, timeout=30).returncode

    def retained(self, name):
        directory = self.f.root / name
        directory.mkdir()
        for key, asset in self.assets.items():
            (directory / key).write_bytes(asset['bytes'])
        return directory

    def test_actual_tag_compile_partial_recovery_and_verified_finalization(self):
        f = self.f
        f.git('checkout', '-b', 'later-main')
        f.commit('feat(REM-50): later independent main')
        f.git('push', 'origin', 'HEAD:main')
        f.git('checkout', 'main')
        self.assertEqual(f.version(push=True).returncode, 0)
        self.build_verified()
        original_tag = verify_remote_tag(f.repo, 'v0.4.1', self.sha)
        self.assertEqual(self.invoke(True, [self.manifest, self.archive]), 1)
        self.assertTrue(self.release['draft'])
        retained = self.assets['manifest.json'].copy()
        partial = self.retained('partial')
        self.assertFalse(verify_downloaded_assets(self.expected, partial, require_complete=False)['complete'])
        self.fail_archive = False
        self.assertEqual(self.invoke(True, [self.manifest, self.archive]), 0)
        self.assertTrue(self.release['draft'])
        self.assertEqual(self.assets['manifest.json'], retained)
        complete = self.retained('complete')
        self.assertTrue(verify_downloaded_assets(self.expected, complete)['complete'])
        (complete / 'sdk.crate').write_bytes(b'tampered owned fixture')
        events = len(self.events)
        with self.assertRaises(Refusal):
            verify_downloaded_assets(self.expected, complete)
        self.assertEqual(len(self.events), events)
        self.assertTrue(self.release['draft'])
        (complete / 'sdk.crate').write_bytes(self.assets['sdk.crate']['bytes'])
        self.assertTrue(verify_downloaded_assets(self.expected, complete)['complete'])
        self.assertEqual(verify_remote_tag(f.repo, 'v0.4.1', self.sha), original_tag)
        self.assertEqual(self.invoke(False, []), 0)
        self.assertFalse(self.release['draft'])
        self.assertEqual(f.git('tag').splitlines(), ['v0.4.0', 'v0.4.1'])
        self.assertFalse(any(e['method'] == 'DELETE' for e in self.events))
        self.assertEqual(sum(e['method'] == 'POST' and e['path'].endswith('/releases') for e in self.events), 1)

    def test_local_tag_is_not_build_or_publication_authority(self):
        self.assertEqual(self.f.version(push=False).returncode, 0)
        with self.assertRaises(Refusal):
            self.build_verified()
        self.assertFalse(self.build.exists())
        self.assertEqual(self.events, [])
        self.assertIsNone(self.release)


if __name__ == '__main__':
    unittest.main()
