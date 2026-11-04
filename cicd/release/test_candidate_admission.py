"""Selected existing candidate-admission controls; no tagger/compiler/publisher."""
import json
import unittest
import test_stage_admission as source_fixture
from stage_source import Refusal, stage, stage_candidate, verify_staged_source

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
