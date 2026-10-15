"""Current-message guard cannot borrow eligibility from older failed/unreleased history."""
from pathlib import Path
import subprocess
import unittest

import python_semantic_release_fixtures as psr


class ActualCommitEligibility(unittest.TestCase):
    def setUp(self):
        self.assertTrue(psr.TOOL, 'Qualified maintained PSR is required')
        self.fixture = psr.PythonSemanticReleaseCandidate(methodName='runTest')
        self.fixture.setUp()
        self.addCleanup(self.fixture.doCleanups)
        self.fixture.baseline()
        self.python = str(Path(psr.TOOL).with_name('python'))
        self.helper = str(Path(__file__).with_name('check_commit.py').resolve())

    def guard(self, expected):
        f = self.fixture
        result = subprocess.run([self.python, self.helper, '--repo', str(f.repo), '--sha', f.git('rev-parse','HEAD'),
                                 '--config', str(f.repo/'releaserc.toml')], env=f.env, text=True,
                                capture_output=True, timeout=15)
        self.assertEqual(result.returncode, expected, result.stderr[-2000:])

    def test_bad_actual_squash_cannot_borrow_candidate_from_failed_untagged_history(self):
        f = self.fixture
        f.commit('feat(REM-56): earlier failed release remains untagged')
        for subject in ['docs(REM-56): manually edited actual squash', 'manually edited nonconventional squash',
                        'docs(REM-56)!: copied breaking body\n\nBREAKING CHANGE: not application eligibility']:
            f.commit(subject)
            raw = f.command(f.repo, psr.TOOL, '-c','releaserc.toml','--strict','version','--print-tag',
                            '--no-commit','--no-tag','--no-changelog','--skip-build','--no-push','--no-vcs-release')
            self.assertEqual(raw.stdout.strip(),'v0.5.0')  # Owned history exposes the original admission gap.
            self.guard(1)
            self.assertNotIn('v0.5.0',f.git('ls-remote','--tags','origin'))
        f.commit('fix(REM-56): actual successful scoped application change')
        self.guard(0)
        self.assertNotIn('v0.5.0',f.git('ls-remote','--tags','origin'))

    def test_supported_scoped_headers_remain_eligible_unscoped_or_empty_scope_refuse(self):
        f = self.fixture
        for subject in ['feat(REM-56): feature','fix(REM-56)!: changed API','ci(release): pipeline repair']:
            f.commit(subject)
            self.guard(0)
        for subject in ['fix: unscoped actual squash','fix( ): whitespace scope']:
            f.commit(subject)
            self.guard(1)

    def test_known_release_readme_is_excluded_but_unknown_source_markdown_is_versioned(self):
        f = self.fixture
        (f.repo/'release').mkdir()
        (f.repo/'release/README.md').write_text('Known documentation only')
        f.git('add','release/README.md')
        f.git('commit','-m','feat(docs): known README must not supply a minor release')
        args = [psr.TOOL,'-c','releaserc.toml','--strict','version','--print-tag',
                '--no-commit','--no-tag','--no-changelog','--skip-build','--no-push','--no-vcs-release']
        observed = f.command(f.repo,*args,allowed=(2,))
        self.assertEqual(observed.stdout.strip(),'v0.4.0')
        (f.repo/'src').mkdir()
        (f.repo/'src/README.md').write_text('Unknown source/build input remains relevant')
        f.git('add','src/README.md')
        f.git('commit','-m','feat(source): unknown source Markdown remains application input')
        self.assertEqual(f.command(f.repo,*args).stdout.strip(),'v0.5.0')
        self.assertNotIn('v0.5.0',f.git('ls-remote','--tags','origin'))
