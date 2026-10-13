"""Actual upstream binary in owned local repositories; no semantic calculator here."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

BINARY = os.environ.get('GIT_CLIFF_BINARY')
CONFIG = Path(__file__).with_name('git-cliff.toml').resolve()

class GitCliffCandidate(unittest.TestCase):
    def setUp(self):
        self.assertTrue(BINARY, 'Set GIT_CLIFF_BINARY to verified upstream 2.14.2 binary')
        self.root = Path(tempfile.mkdtemp(prefix='sdk-cliff-owned-'))
        self.addCleanup(shutil.rmtree, self.root)
        self.env = {'PATH': os.environ['PATH'], 'GIT_CONFIG_NOSYSTEM': '1', 'GIT_CONFIG_GLOBAL': '/dev/null',
                    'GIT_AUTHOR_NAME': 'Owned Fixture', 'GIT_COMMITTER_NAME': 'Owned Fixture',
                    'GIT_AUTHOR_EMAIL': 'fixture@example.invalid', 'GIT_COMMITTER_EMAIL': 'fixture@example.invalid',
                    'GIT_AUTHOR_DATE': '2026-01-01T00:00:00Z', 'GIT_COMMITTER_DATE': '2026-01-01T00:00:00Z'}
        self.git('init', '--initial-branch=main')
        self.commit('chore: fixture baseline')
        self.git('tag', 'v0.1.0')

    def git(self, *args):
        return subprocess.check_output(['git', *args], cwd=self.root, env=self.env, stderr=subprocess.PIPE, text=True).strip()

    def commit(self, subject):
        self.git('commit', '--allow-empty', '-m', subject)

    def bump(self):
        before = self.git('show-ref', '--tags')
        result = subprocess.check_output([BINARY, '--config', str(CONFIG), '--repository', str(self.root),
                                          '--bumped-version', '--unreleased', '--use-branch-tags', '--no-exec', '--offline'], cwd=self.root, env=self.env,
                                         stderr=subprocess.PIPE, text=True, timeout=15).strip()
        self.assertEqual(self.git('show-ref', '--tags'), before, 'calculator must not alter tags')
        return result

    def test_semantic_types(self):
        for subject, expected in [('feat(api): fixture','v0.2.0'), ('fix(api): fixture','v0.1.1'),
                                  ('perf(api): fixture','v0.1.1'), ('build(api): fixture','v0.1.1'),
                                  ('ci(api): fixture','v0.1.1'), ('chore(api): fixture','v0.1.1'),
                                  ('test(api): fixture','v0.1.1'), ('revert(api): fixture','v0.1.1'),
                                  ('refactor(api): fixture','v0.1.1'), ('feat(api)!: fixture','v0.2.0')]:
            with self.subTest(subject=subject):
                self.git('checkout', '--detach', 'v0.1.0')
                self.commit(subject)
                self.assertEqual(self.bump(), expected)

    def test_equal_time_feature_then_fix(self):
        self.commit('feat(api): fixture'); self.assertEqual(self.bump(), 'v0.2.0')
        self.git('tag', 'v0.2.0'); self.commit('fix(api): fixture')
        self.assertEqual(self.bump(), 'v0.2.1')
        self.assertEqual(len(set(self.git('log', '--format=%ct').splitlines())), 1)

    def test_docs_and_breaking_docs_are_no_bump(self):
        self.commit('docs(api): fixture'); self.assertEqual(self.bump(), 'v0.1.0')
        self.commit('docs(api)!: fixture\n\nBREAKING CHANGE: fixture')
        self.assertEqual(self.bump(), 'v0.1.0')
        self.commit('feat(api): fixture'); self.assertEqual(self.bump(), 'v0.2.0')

    def test_repeat_immutable(self):
        self.commit('fix(api): fixture'); self.assertEqual(self.bump(), self.bump())

    def test_full_semver_breaking(self):
        self.git('tag', 'v1.2.0'); self.commit('feat(api)!: fixture')
        self.assertEqual(self.bump(), 'v2.0.0')

    def test_old_checkout_ignores_descendant_tag(self):
        self.commit('feat(api): fixture'); feature = self.git('rev-parse', 'HEAD')
        self.git('tag', 'v0.2.0'); self.commit('fix(api): fixture'); self.git('tag', 'v0.2.1')
        self.git('checkout', '--detach', feature)
        self.assertEqual(self.bump(), 'v0.2.0')

if __name__ == '__main__': unittest.main()
