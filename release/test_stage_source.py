"""Owned source/tag fixtures verify ephemeral build identity, not production SDK."""
import io
import json
import tarfile
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import tomllib
import unittest
from stage_source import stage,Refusal,verify_staged_source,distribution_manifest

class SourceStaging(unittest.TestCase):
    def setUp(self):
        self.root=Path(tempfile.mkdtemp(prefix='sdk-stage-owned-'));self.addCleanup(shutil.rmtree,self.root)
        self.repo=self.root/'source';self.repo.mkdir();self.out=self.root/'stage'
        self.env={**os.environ,'GIT_CONFIG_NOSYSTEM':'1','GIT_CONFIG_GLOBAL':'/dev/null',
                  'GIT_AUTHOR_NAME':'Owned Fixture','GIT_COMMITTER_NAME':'Owned Fixture',
                  'GIT_AUTHOR_EMAIL':'fixture@example.invalid','GIT_COMMITTER_EMAIL':'fixture@example.invalid'}
        self.git('init','--initial-branch=main')
        (self.repo/'Cargo.toml').write_text('[package]\nname="owned_fixture"\nedition="2024"\nlicense="MIT"\n')
        (self.repo/'src').mkdir()
        (self.repo/'src/lib.rs').write_text('pub fn version() -> &\'static str { env!("CARGO_PKG_VERSION") }\n')
        (self.repo/'src/main.rs').write_text('fn main() { println!("{}", owned_fixture::version()); }\n')
        self.git('add','.');self.git('commit','-m','feat: owned fixture');self.sha=self.git('rev-parse','HEAD')
        self.git('tag','v0.3.7')
    def git(self,*args):return subprocess.check_output(['git',*args],cwd=self.repo,env=self.env,stderr=subprocess.PIPE,text=True).strip()

    def test_exact_tag_injects_only_staging_version(self):
        before=self.git('status','--porcelain');identity=stage(self.repo,'v0.3.7',self.sha,self.out)
        self.assertNotIn('version',tomllib.loads((self.repo/'Cargo.toml').read_text())['package'])
        self.assertEqual(tomllib.loads((self.out/'Cargo.toml').read_text())['package']['version'],'0.3.7')
        self.assertEqual(self.git('status','--porcelain'),before)
        self.assertEqual(identity['source_sha'],self.sha);self.assertEqual(identity['git_tag'],'v0.3.7')
        self.assertFalse(identity['qualification']['source_authority'])
        self.assertEqual(json.loads((self.out/'build-identity.json').read_text()),identity)

    def test_wrong_sha_or_missing_tag_refuses_without_output(self):
        for tag,sha in [('v0.3.8',self.sha),('v0.3.7','a'*40),('--unsafe',self.sha)]:
            with self.subTest(tag=tag,sha=sha),self.assertRaises(Refusal):stage(self.repo,tag,sha,self.out)
            self.assertFalse(self.out.exists())

    def test_dirty_source_refuses(self):
        (self.repo/'untracked').write_text('owned fixture')
        with self.assertRaises(Refusal):stage(self.repo,'v0.3.7',self.sha,self.out)
        self.assertFalse(self.out.exists())

    def test_source_version_field_refuses(self):
        (self.repo/'Cargo.toml').write_text('[package]\nname="owned_fixture"\nversion="0.0.0"\n')
        self.git('add','.');self.git('commit','-m','chore: rejected fixture');sha=self.git('rev-parse','HEAD');self.git('tag','v0.3.8')
        with self.assertRaises(Refusal):stage(self.repo,'v0.3.8',sha,self.out)
        self.assertFalse(self.out.exists())

    def test_workspace_version_refuses(self):
        with (self.repo/'Cargo.toml').open('a') as f:f.write('[workspace.package]\nversion="0.0.0"\n')
        self.git('add','.');self.git('commit','-m','chore: rejected fixture');sha=self.git('rev-parse','HEAD');self.git('tag','v0.3.8')
        with self.assertRaises(Refusal):stage(self.repo,'v0.3.8',sha,self.out)
        self.assertFalse(self.out.exists())

    def test_source_root_lock_version_refuses(self):
        (self.repo/'Cargo.lock').write_text('version=4\n[[package]]\nname="owned_fixture"\nversion="0.0.0"\n')
        self.git('add','.');self.git('commit','-m','chore: rejected fixture');sha=self.git('rev-parse','HEAD');self.git('tag','v0.3.8')
        with self.assertRaises(Refusal):stage(self.repo,'v0.3.8',sha,self.out)
        self.assertFalse(self.out.exists())

    def test_existing_destination_preserved(self):
        self.out.mkdir();marker=self.out/'marker';marker.write_text('preserve')
        with self.assertRaises(Refusal):stage(self.repo,'v0.3.7',self.sha,self.out)
        self.assertEqual(marker.read_text(),'preserve')

    def test_staged_source_drift_refuses(self):
        stage(self.repo,'v0.3.7',self.sha,self.out)
        (self.out/'src/lib.rs').write_text('changed source')
        with self.assertRaises(Refusal):verify_staged_source(self.out)

    def test_new_local_override_file_refuses(self):
        stage(self.repo,'v0.3.7',self.sha,self.out)
        (self.out/'.cargo').mkdir();(self.out/'.cargo/config.toml').write_text('# local override')
        with self.assertRaises(Refusal):verify_staged_source(self.out)

    def test_actual_source_crate_manifest_is_tagged_and_unqualified(self):
        cargo=os.environ.get('SDK_FIXTURE_CARGO');self.assertTrue(cargo)
        stage(self.repo,'v0.3.7',self.sha,self.out)
        subprocess.check_output([cargo,'generate-lockfile','--offline'],cwd=self.out,env=self.env,stderr=subprocess.PIPE,timeout=60)
        verify_staged_source(self.out)
        subprocess.check_output([cargo,'package','--locked','--offline'],cwd=self.out,env=self.env,stderr=subprocess.PIPE,timeout=60)
        artifact=self.out/'target/package/owned_fixture-0.3.7.crate'
        manifest=distribution_manifest(self.out,artifact)
        self.assertEqual(manifest['git_tag'],'v0.3.7');self.assertEqual(manifest['source_sha'],self.sha)
        self.assertEqual(manifest['native_operations'],'unsupported')
        self.assertTrue(all(v=='unqualified' for v in manifest['compatibility'].values()))
        self.assertEqual(manifest['distributions'][0]['size'],artifact.stat().st_size)

    def test_wrong_packaged_bytes_refuse(self):
        cargo=os.environ.get('SDK_FIXTURE_CARGO');self.assertTrue(cargo)
        stage(self.repo,'v0.3.7',self.sha,self.out)
        subprocess.check_output([cargo,'generate-lockfile','--offline'],cwd=self.out,env=self.env,stderr=subprocess.PIPE,timeout=60)
        subprocess.check_output([cargo,'package','--locked','--offline'],cwd=self.out,env=self.env,stderr=subprocess.PIPE,timeout=60)
        original=self.out/'target/package/owned_fixture-0.3.7.crate'
        for changed in ['src/lib.rs','Cargo.toml','Cargo.lock']:
            with self.subTest(changed=changed):
                tampered=self.root/'tampered.crate'
                with tarfile.open(original,'r:gz') as source,tarfile.open(tampered,'w:gz') as dest:
                    for member in source.getmembers():
                        data=source.extractfile(member).read() if member.isfile() else None
                        if member.name=='owned_fixture-0.3.7/'+changed:
                            if changed=='Cargo.toml':data=data.replace(b'0.3.7',b'0.3.8')
                            elif changed=='Cargo.lock':data+=b'\n[[package]]\nname="unexpected"\nversion="1.0.0"\n'
                            else:data=b'changed packaged source'
                            member.size=len(data)
                        dest.addfile(member,io.BytesIO(data) if data is not None else None)
                with self.assertRaises(Refusal):distribution_manifest(self.out,tampered)

    def test_actual_host_and_cross_fixture_compile(self):
        cargo=os.environ.get('SDK_FIXTURE_CARGO');self.assertTrue(cargo,'Set SDK_FIXTURE_CARGO to installed pinned Cargo')
        stage(self.repo,'v0.3.7',self.sha,self.out)
        run=lambda args:subprocess.check_output([cargo,*args],cwd=self.out,env=self.env,stderr=subprocess.PIPE,text=True,timeout=60)
        run(['generate-lockfile','--offline'])
        for target in ['x86_64-unknown-linux-gnu','armv7-unknown-linux-gnueabihf','aarch64-unknown-linux-gnu']:
            with self.subTest(target=target):
                run(['build','--locked','--offline','--lib','--target',target])
                self.assertTrue((self.out/'target'/target/'debug/libowned_fixture.rlib').is_file())
        self.assertEqual(run(['run','--locked','--offline','--quiet']).strip(),'0.3.7')
        self.assertNotIn('version',tomllib.loads((self.repo/'Cargo.toml').read_text())['package'])

if __name__=='__main__':unittest.main()
