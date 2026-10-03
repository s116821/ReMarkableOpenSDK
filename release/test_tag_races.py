"""Actual maintained tag Action entrypoint with owned bare remotes; no Docker claim."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ENTRY = os.environ.get('CREATE_TAG_ENTRYPOINT')

class UpstreamTagRaces(unittest.TestCase):
    def setUp(self):
        self.assertTrue(ENTRY, 'Set CREATE_TAG_ENTRYPOINT to exact inspected upstream entrypoint')
        self.output={};self.root=Path(tempfile.mkdtemp(prefix='sdk-tag-race-owned-'));self.addCleanup(shutil.rmtree,self.root)
        self.remote=self.root/'remote.git'
        self.env={'PATH':os.environ['PATH'],'GIT_CONFIG_NOSYSTEM':'1','GIT_CONFIG_GLOBAL':str(self.root/'global-config'),
                  'GIT_AUTHOR_NAME':'Owned Fixture','GIT_COMMITTER_NAME':'Owned Fixture',
                  'GIT_AUTHOR_EMAIL':'fixture@example.invalid','GIT_COMMITTER_EMAIL':'fixture@example.invalid',
                  'GIT_AUTHOR_DATE':'2026-01-01T00:00:00Z','GIT_COMMITTER_DATE':'2026-01-01T00:00:00Z'}
        self.git(self.root,'init','--bare','--initial-branch=main',str(self.remote))
        self.a=self.root/'a';self.git(self.root,'clone',str(self.remote),str(self.a))
        self.git(self.a,'commit','--allow-empty','-m','chore: fixture baseline');self.git(self.a,'tag','v0.1.0')
        self.git(self.a,'push','origin','main','--tags');self.base=self.git(self.a,'rev-parse','v0.1.0')
        self.git(self.a,'commit','--allow-empty','-m','feat: fixture');self.first=self.git(self.a,'rev-parse','HEAD')
        self.git(self.a,'push','origin','main')
        self.b=self.root/'b';self.git(self.root,'clone',str(self.remote),str(self.b))
        self.git(self.b,'commit','--allow-empty','-m','fix: fixture');self.second=self.git(self.b,'rev-parse','HEAD')
        self.git(self.b,'push','origin','main')

    def git(self,cwd,*args):
        return subprocess.check_output(['git',*args],cwd=cwd,env=self.env,stderr=subprocess.PIPE,text=True).strip()

    def run_action(self,cwd,sha,tag='v0.2.0'):
        env={**self.env,'GIT_CONFIG_GLOBAL':str(cwd/'.global-config'),'GITHUB_WORKSPACE':str(cwd),'GITHUB_ACTOR':'owned-fixture',
             'GITHUB_REPOSITORY':'fixture/owned','GITHUB_SHA':sha,'INPUT_COMMIT_SHA':sha,
             'INPUT_TAG':tag,'INPUT_GITHUB_TOKEN':'','INPUT_FORCE_PUSH_TAG':'false','INPUT_TAG_EXISTS_ERROR':'true',
             'GITHUB_OUTPUT':str(cwd/'.output'),'GITHUB_ENV':str(cwd/'.env')}
        return subprocess.Popen(['sh',ENTRY],cwd=cwd,env=env,stdout=subprocess.PIPE,stderr=subprocess.PIPE)

    def finish(self,proc):
        try:self.output[proc.pid]=proc.communicate(timeout=15)
        except subprocess.TimeoutExpired:proc.kill();proc.communicate();self.fail('Owned Action deadline')
        return proc.returncode

    def remote_sha(self,tag):return self.git(self.remote,'rev-parse',f'refs/tags/{tag}^{{commit}}')

    def test_concurrent_different_sources_cannot_retarget_tag(self):
        a=self.run_action(self.a,self.first);b=self.run_action(self.b,self.second)
        self.assertEqual(sorted([self.finish(a),self.finish(b)]),[0,1])
        for proc in [a,b]:self.assertIn(b"Push tag 'v0.2.0'",self.output[proc.pid][0])
        self.assertIn(self.remote_sha('v0.2.0'),[self.first,self.second])
        self.assertEqual(self.remote_sha('v0.1.0'),self.base)

    def test_main_advancement_does_not_change_explicit_source(self):
        self.assertEqual(self.git(self.remote,'rev-parse','main'),self.second)
        self.assertEqual(self.finish(self.run_action(self.a,self.first)),0)
        self.assertEqual(self.remote_sha('v0.2.0'),self.first)
        self.assertNotEqual(self.remote_sha('v0.2.0'),self.second)

    def test_existing_local_tag_is_refused_without_force(self):
        self.assertEqual(self.finish(self.run_action(self.a,self.first)),0)
        self.assertEqual(self.finish(self.run_action(self.a,self.second)),1)
        self.assertEqual(self.remote_sha('v0.2.0'),self.first)

if __name__=='__main__':unittest.main()
