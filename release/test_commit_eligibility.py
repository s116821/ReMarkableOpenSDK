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

    def guard(self, expected, source=None):
        f = self.fixture
        refs, head = f.git("show-ref", "--tags"), f.git("rev-parse", "HEAD")
        result = subprocess.run([self.python, self.helper, '--repo', str(f.repo), '--sha', source or f.git('rev-parse','HEAD'),
                                 '--config', str(f.repo/'releaserc.toml')], env=f.env, text=True,
                                capture_output=True, timeout=15)
        self.assertEqual(result.returncode, expected, result.stderr[-2000:])
        self.assertEqual(f.git("show-ref", "--tags"), refs)
        self.assertEqual(f.git("rev-parse", "HEAD"), head)
        self.assertEqual(f.git("status", "--porcelain"), "")

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

    def test_supported_optional_scopes_remain_eligible_empty_scope_refuses(self):
        f = self.fixture
        for subject in ['feat(REM-56): feature','fix(REM-56)!: changed API','ci(release): pipeline repair',
                        'fix: optional scope', 'feat!: optional breaking scope']:
            f.commit(subject)
            self.guard(0)
        for subject in ['fix(): empty scope', 'fix()!: empty breaking scope', 'fix( ): whitespace scope',
                        'unsupported(api): unknown type']:
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

    def test_exact_checkout_and_source_identity_refuse_without_mutation(self):
        f = self.fixture
        f.commit('fix: exact current source')
        self.guard(1, 'not-an-exact-sha')
        self.guard(1, f.git('rev-parse', 'HEAD^1'))
        self.guard(0)

    def test_actual_workflow_push_observation_refuses_ambiguous_history(self):
        import textwrap
        f = self.fixture
        workflow = (Path(__file__).resolve().parents[1] / '.github/workflows/ci.yml').read_text()
        block = workflow.split('      - name: Require exactly one observed main first-parent commit\n', 1)[1]
        script = block.split('        run: |\n', 1)[1].split('      - uses:', 1)[0]
        script = textwrap.dedent(script)
        before = f.git('rev-parse', 'HEAD')
        f.commit('fix: one observed step')
        head = f.git('rev-parse', 'HEAD')
        refs = f.git('show-ref', '--tags')
        for observed, expected in [(before, 0), ('0' * 40, 1), (head, 1)]:
            env = dict(f.env, BEFORE=observed, SOURCE=head)
            result = subprocess.run(['bash', '-e', '-o', 'pipefail', '-c', script], cwd=f.repo,
                                    env=env, text=True, capture_output=True, timeout=15)
            self.assertEqual(result.returncode, expected, result.stderr)
        f.commit('fix: second unobserved step')
        result = subprocess.run(['bash', '-e', '-o', 'pipefail', '-c', script], cwd=f.repo,
                                env=dict(f.env, BEFORE=before, SOURCE=f.git('rev-parse', 'HEAD')),
                                text=True, capture_output=True, timeout=15)
        self.assertEqual(result.returncode, 1, result.stderr)
        self.assertEqual(f.git('show-ref', '--tags'), refs)
        self.assertEqual(f.git('status', '--porcelain'), '')
