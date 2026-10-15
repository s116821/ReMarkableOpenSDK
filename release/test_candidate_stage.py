"""Pre-tag candidate admission; owned Git/Cargo only, never production tags."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import unittest

import python_semantic_release_fixtures as psr
import test_stage_source as source_fixture
from remote_identity import verify_remote_tag
from stage_source import Refusal, stage, stage_candidate, verify_staged_source, distribution_manifest
from verify_assets import verify_downloaded_assets


class CandidateAdmission(unittest.TestCase):
    def setUp(self):
        self.fixture = source_fixture.SourceStaging(methodName='runTest')
        self.fixture.setUp()
        self.addCleanup(self.fixture.doCleanups)

    def test_no_tag_candidate_cannot_pass_default_official_admission(self):
        f = self.fixture
        before = f.git('show-ref', '--tags')
        identity = stage_candidate(f.repo, 'v0.3.8', f.sha, f.out)
        self.assertEqual(identity['identity_kind'], 'candidate')
        self.assertIsNone(identity['git_tag'])
        self.assertEqual(identity['candidate_tag'], 'v0.3.8')
        self.assertEqual(f.git('show-ref', '--tags'), before)
        with self.assertRaises(Refusal):
            verify_staged_source(f.out, source_only=True)
        self.assertFalse(verify_staged_source(f.out, source_only=True, allow_candidate=True)['lock_verified'])
        with self.assertRaises(Refusal):
            stage(f.repo, 'v0.3.8', f.sha, f.root / 'official')
        # Removing the candidate marker cannot turn its null verified tag into authority.
        identity.pop('identity_kind')
        (f.out / 'build-identity.json').write_text(json.dumps(identity))
        with self.assertRaises(Refusal):
            verify_staged_source(f.out, source_only=True)

    def test_candidate_keeps_source_destination_and_version_refusals(self):
        f = self.fixture
        for candidate, sha in [('v01.2.3', f.sha), ('v0.3.8', '0' * 40)]:
            with self.assertRaises(Refusal):
                stage_candidate(f.repo, candidate, sha, f.out)
            self.assertFalse(f.out.exists())
        (f.repo / 'untracked').write_text('preserve')
        with self.assertRaises(Refusal):
            stage_candidate(f.repo, 'v0.3.8', f.sha, f.out)
        (f.repo / 'untracked').unlink()
        f.out.mkdir()
        marker = f.out / 'preserve'
        marker.write_text('owned')
        with self.assertRaises(Refusal):
            stage_candidate(f.repo, 'v0.3.8', f.sha, f.out)
        self.assertEqual(marker.read_text(), 'owned')


class MaintainedCandidateComposition(unittest.TestCase):
    def setUp(self):
        self.assertTrue(psr.TOOL, 'Hash-installed maintained PSR is required')
        self.cargo = os.environ.get('SDK_FIXTURE_CARGO')
        self.assertTrue(self.cargo, 'Actual Cargo is required')
        self.fixture = psr.PythonSemanticReleaseCandidate(methodName='runTest')
        self.fixture.setUp()
        self.addCleanup(self.fixture.doCleanups)
        f = self.fixture
        for name in ('CARGO_HOME', 'RUSTUP_HOME'):
            if name in os.environ:
                f.env[name] = os.environ[name]
        f.env.setdefault('RUSTUP_HOME', str(Path.home() / '.rustup'))
        f.env['RUSTUP_TOOLCHAIN'] = '1.98.1'
        f.baseline()
        (f.repo / 'Cargo.toml').write_text('[package]\nname="owned_candidate"\nedition="2024"\nlicense="MIT"\n')
        (f.repo / 'src').mkdir()
        (f.repo / 'src/main.rs').write_text('fn main() { println!("{}", env!("CARGO_PKG_VERSION")); }\n')
        f.git('add', '.')
        f.commit('feat(REM-56): owned candidate compile')
        f.git('push', 'origin', 'main')
        self.sha = f.git('rev-parse', 'HEAD')

    def candidate(self):
        f = self.fixture
        before = (f.git('show-ref', '--tags'), f.git('ls-remote', '--tags', 'origin'), f.git('status', '--porcelain'))
        args = [psr.TOOL, '-c', 'releaserc.toml', '--strict', 'version', '--print-tag',
                '--no-commit', '--no-tag', '--no-changelog', '--skip-build', '--no-push', '--no-vcs-release']
        tag = f.command(f.repo, *args).stdout.strip()
        self.assertEqual(f.command(f.repo, *args).stdout.strip(), tag)
        self.assertEqual(tag, 'v0.5.0')  # Owned fixture input, never an SDK release choice.
        self.assertEqual((f.git('show-ref', '--tags'), f.git('ls-remote', '--tags', 'origin'), f.git('status', '--porcelain')), before)
        self.assertEqual(f.git('rev-parse', 'HEAD'), self.sha)
        return tag

    def cargo_command(self, stage_dir, *args, expected=0):
        result = subprocess.run([self.cargo, *args], cwd=stage_dir, capture_output=True,
                                env=self.fixture.env, text=True, timeout=90)
        self.assertEqual(result.returncode, expected, result.stderr[-3000:])
        return result

    def test_built_verified_bytes_precede_final_maintained_tag(self):
        f = self.fixture
        tag = self.candidate()
        stage_dir = f.root / 'candidate'
        stage_candidate(f.repo, tag, self.sha, stage_dir)
        self.cargo_command(stage_dir, 'generate-lockfile', '--offline')
        verify_staged_source(stage_dir, allow_candidate=True)
        self.cargo_command(stage_dir, 'build', '--locked', '--offline')
        binary = stage_dir / 'target/debug/owned_candidate'
        self.assertEqual(subprocess.check_output([str(binary)], text=True, timeout=5).strip(), tag[1:])
        self.cargo_command(stage_dir, 'package', '--locked', '--offline')
        artifact = stage_dir / 'target/package' / f'owned_candidate-{tag[1:]}.crate'
        with self.assertRaises(Refusal):
            distribution_manifest(stage_dir, artifact)
        manifest = distribution_manifest(stage_dir, artifact, allow_candidate=True)
        self.assertEqual(manifest['build_identity'], 'candidate')
        self.assertTrue(manifest['official_remote_tag_verification_required'])
        assets = f.root / 'built-assets'
        assets.mkdir()
        shutil.copyfile(artifact, assets / artifact.name)
        (assets / 'manifest.json').write_text(json.dumps(manifest))
        self.assertTrue(verify_downloaded_assets(manifest, assets)['complete'])
        frozen = {p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in assets.iterdir()}
        self.assertNotIn(tag, f.git('ls-remote', '--tags', 'origin'))
        # The maintained tag operation occurs only after actual build/integration.
        self.assertEqual(f.version(push=True).returncode, 0)
        self.assertEqual(verify_remote_tag(f.repo, tag, self.sha)['source_sha'], self.sha)
        self.assertTrue(verify_downloaded_assets(manifest, assets)['complete'])
        self.assertEqual({p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in assets.iterdir()}, frozen)
        self.assertEqual(f.git('rev-parse', 'HEAD'), self.sha)
        self.assertEqual(f.git('status', '--porcelain'), '')

    def test_real_build_failure_leaves_remote_without_candidate_tag(self):
        f = self.fixture
        # A committed invalid source remains an actual compiler failure, not a fake result.
        (f.repo / 'src/main.rs').write_text('fn main() { undefined_owned_symbol(); }\n')
        f.git('add', '.')
        f.commit('fix(REM-56): owned compiler refusal')
        self.sha = f.git('rev-parse', 'HEAD')
        f.git('push', 'origin', 'main')
        tag = self.candidate()
        stage_dir = f.root / 'failed-build'
        stage_candidate(f.repo, tag, self.sha, stage_dir)
        self.cargo_command(stage_dir, 'generate-lockfile', '--offline')
        result = self.cargo_command(stage_dir, 'build', '--locked', '--offline', expected=101)
        self.assertIn('E0425', result.stderr)
        self.assertNotIn(tag, f.git('ls-remote', '--tags', 'origin'))
        self.assertEqual(f.git('rev-parse', 'HEAD'), self.sha)
        # This is owned fixture ordering evidence; Actions DAG failure behavior
        # requires the workflow's explicit gate and hosted evidence separately.
