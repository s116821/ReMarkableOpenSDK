"""Actual maintained read-only candidate CLI; no SDK tag or publication."""
from pathlib import Path
import unittest
import python_semantic_release_fixtures as psr


class ReadOnlyMetadata(unittest.TestCase):
    def fixture(self, baseline="v0.4.0"):
        fixture = psr.PythonSemanticReleaseCandidate(methodName="runTest")
        fixture.setUp()
        self.addCleanup(lambda: self.assertTrue(fixture.doCleanups()))
        fixture.baseline(baseline)
        return fixture

    def inspect(self, fixture, expected):
        refs = fixture.git("show-ref", "--tags")
        source = fixture.git("rev-parse", "HEAD")
        result = fixture.command(
            fixture.repo, psr.TOOL, "-c", "releaserc.toml", "--strict", "version",
            "--print-tag", "--no-commit", "--no-tag", "--no-changelog",
            "--skip-build", "--no-push", "--no-vcs-release", allowed=(0, 2),
        )
        self.assertEqual(result.stdout.strip(), expected)
        self.assertEqual(fixture.git("show-ref", "--tags"), refs)
        self.assertEqual(fixture.git("rev-parse", "HEAD"), source)
        self.assertEqual(fixture.git("status", "--porcelain"), "")
        self.assertFalse((fixture.repo / "CHANGELOG.md").exists())

    def test_read_only_conventional_types_and_docs(self):
        vectors = [("feat: optional scope", "v0.5.0"),
                   ("feat(api)!: prezero breaking change", "v0.5.0")]
        vectors += [(f"{kind}(REM-50): owned application", "v0.4.1")
                    for kind in ["fix", "perf", "build", "ci", "chore", "refactor", "test", "revert"]]
        vectors += [("docs: guide", "v0.4.0"),
                    ("docs(REM-50)!: guide\n\nfeat(api)!: copied example\nBREAKING CHANGE: copied", "v0.4.0")]
        for subject, expected in vectors:
            with self.subTest(subject=subject):
                fixture = self.fixture()
                fixture.commit(subject)
                self.inspect(fixture, expected)

    def test_guidance_ignored_but_executable_tool_inventory_observed(self):
        fixture = self.fixture()
        for name, expected in [("release/README.md", "v0.4.0"),
                               ("docs/reference/dependency-license-inventory.json", "v0.4.0"),
                               ("tools/dependency-license-inventory.json", "v0.5.0")]:
            path = fixture.repo / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text("Owned harmless fixture input\n")
            fixture.git("add", name)
            fixture.git("commit", "-m", "feat(REM-50): change one owned path")
            self.inspect(fixture, expected)

    def test_stable_breaking_candidate_is_read_only(self):
        fixture = self.fixture("v1.2.3")
        fixture.commit("fix!: stable API change")
        self.inspect(fixture, "v2.0.0")
