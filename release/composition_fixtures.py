"""Actual upstream tag -> verified build -> draft recovery, owned fixtures only.

This is a test composition, not a production release coordinator. Its Git pushes
reach a disposable bare repository and its API calls reach loopback TLS only.
"""
import json
import os
from pathlib import Path
import shutil
import unittest

from github_cli_fixtures import GithubCliCandidate
from python_semantic_release_fixtures import PythonSemanticReleaseCandidate
from remote_identity import verify_remote_tag
from stage_source import Refusal, distribution_manifest, stage, verify_staged_source
from verify_assets import verify_downloaded_assets


class MaintainedComposition(GithubCliCandidate):
    def setUp(self):
        super().setUp()
        self.version_fixture = PythonSemanticReleaseCandidate()
        self.version_fixture.setUp()
        self.addCleanup(self.version_fixture.doCleanups)
        f = self.version_fixture
        # Whitelist compiler locations, not the parent's provider credentials.
        for name in ("RUSTUP_HOME", "CARGO_HOME"):
            if name in os.environ:
                f.env[name] = os.environ[name]
        # Hosted rustup does not export RUSTUP_HOME. The isolated child HOME must
        # not hide the installed toolchain or fall back to an implicit version.
        f.env.setdefault("RUSTUP_HOME", str(Path.home() / ".rustup"))
        f.env["RUSTUP_TOOLCHAIN"] = "1.98.1"
        (f.repo / "Cargo.toml").write_text(
            '[package]\nname="owned_composition"\nedition="2024"\nlicense="MIT"\n'
        )
        (f.repo / "src").mkdir()
        (f.repo / "src/main.rs").write_text(
            'fn main() { println!("{}", env!("CARGO_PKG_VERSION")); }\n'
        )
        f.git("add", ".")
        f.commit("chore(REM-50): owned version-free source")
        f.baseline()
        f.commit("fix(REM-50): owned intended release")
        self.sha = f.git("rev-parse", "HEAD")
        self.build = self.root / "tagged-build"

    def build_verified_source(self):
        f = self.version_fixture
        verify_remote_tag(f.repo, "v0.4.1", self.sha)
        stage(f.repo, "v0.4.1", self.sha, self.build)
        cargo = os.environ.get("SDK_FIXTURE_CARGO")
        self.assertTrue(cargo, "Set SDK_FIXTURE_CARGO to pinned Rust 1.98.1 Cargo")
        f.command(self.build, cargo, "generate-lockfile", "--offline")
        verify_staged_source(self.build)
        output = f.command(self.build, cargo, "run", "--locked", "--offline", "--quiet")
        self.assertEqual(output.stdout.strip(), "0.4.1")
        f.command(self.build, cargo, "package", "--locked", "--offline")
        shutil.copyfile(self.build / "target/package/owned_composition-0.4.1.crate", self.archive)
        self.expected = distribution_manifest(self.build, self.archive)
        self.manifest.write_text(json.dumps(self.expected))
        self.assertEqual(self.expected["source_sha"], self.sha)
        self.assertNotIn('version=', (f.repo / "Cargo.toml").read_text())
        self.assertEqual(f.git("rev-parse", "HEAD"), self.sha)
        self.assertEqual(f.git("status", "--porcelain"), "")

    def test_tagged_compiled_distribution_recovers_without_retagging(self):
        f = self.version_fixture
        # Pin to the job's checkout even when main has already advanced remotely.
        f.git("checkout", "-b", "later-main")
        f.commit("feat(REM-50): later independent work")
        f.git("push", "origin", "HEAD:main")
        f.git("checkout", "main")
        self.assertEqual(f.version(push=True).returncode, 0)
        self.build_verified_source()
        remote_tag = verify_remote_tag(f.repo, "v0.4.1", self.sha)
        self.cli("create", "v0.4.1", "--verify-tag", "--draft", "--notes", "Owned composition")
        self.cli("upload", "v0.4.1", str(self.manifest))
        self.fail_archive = True
        self.cli("upload", "v0.4.1", str(self.archive), expected=1)
        self.assertTrue(self.release["draft"])
        retained = self.assets["manifest.json"].copy()
        downloaded = self.root / "retained-subset"
        self.cli("download", "v0.4.1", "--dir", str(downloaded))
        self.assertFalse(verify_downloaded_assets(self.expected, downloaded, require_complete=False)["complete"])
        self.fail_archive = False
        # Recovery consumes the existing tag/build identity, not the version CLI.
        self.assertEqual(verify_remote_tag(f.repo, "v0.4.1", self.sha), remote_tag)
        self.cli("upload", "v0.4.1", str(self.archive))
        complete = self.root / "complete-build-assets"
        self.cli("download", "v0.4.1", "--dir", str(complete))
        self.assertTrue(verify_downloaded_assets(self.expected, complete)["complete"])
        verify_remote_tag(f.repo, "v0.4.1", self.sha)
        self.cli("edit", "v0.4.1", "--draft=false")
        self.assertEqual(self.assets["manifest.json"], retained)
        self.assertEqual(f.git("tag").splitlines(), ["v0.4.0", "v0.4.1"])
        self.assertFalse(any(e["method"] == "DELETE" for e in self.events))

    def test_local_tag_success_cannot_build_or_contact_publisher(self):
        f = self.version_fixture
        self.assertEqual(f.version(push=False).returncode, 0)
        with self.assertRaises(Refusal):
            self.build_verified_source()
        self.assertFalse(self.build.exists())
        self.assertEqual(self.events, [])
        self.assertIsNone(self.release)


def load_tests(loader, tests, pattern):
    # Do not rerun the already-covered inherited publisher tests in this suite.
    return unittest.TestSuite(
        MaintainedComposition(name) for name in MaintainedComposition.__dict__
        if name.startswith("test_")
    )


if __name__ == "__main__":
    unittest.main()
