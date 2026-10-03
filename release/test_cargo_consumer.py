"""Owned Cargo Git/tag/path fixtures, no real Buddy/SDK or network service."""
import os
from pathlib import Path
import json
import shutil
import subprocess
import tempfile
import unittest
from stage_source import verify_sdk_dependency,sdk_build_environment,Refusal

class CargoConsumerIdentity(unittest.TestCase):
    def setUp(self):
        self.cargo=os.environ.get('SDK_FIXTURE_CARGO');self.assertTrue(self.cargo)
        self.root=Path(tempfile.mkdtemp(prefix='sdk-cargo-owned-'));self.addCleanup(shutil.rmtree,self.root)
        self.env={**os.environ,'GIT_CONFIG_NOSYSTEM':'1','GIT_CONFIG_GLOBAL':'/dev/null',
                  'GIT_AUTHOR_NAME':'Owned Fixture','GIT_COMMITTER_NAME':'Owned Fixture',
                  'GIT_AUTHOR_EMAIL':'fixture@example.invalid','GIT_COMMITTER_EMAIL':'fixture@example.invalid'}
        self.sdk=self.root/'sdk';self.sdk.mkdir();(self.sdk/'src').mkdir()
        (self.sdk/'Cargo.toml').write_text('[package]\nname="owned_sdk"\nedition="2024"\n')
        (self.sdk/'src/lib.rs').write_text('pub fn answer() -> u8 {5}\npub fn package_version() -> &\'static str {env!("CARGO_PKG_VERSION")}\npub fn build_version() -> &\'static str {env!("SDK_BUILD_VERSION")}\n')
        (self.sdk/'build.rs').write_text('fn main() {println!("cargo:rerun-if-env-changed=SDK_BUILD_VERSION"); println!("cargo:rustc-env=SDK_BUILD_VERSION={}", std::env::var("SDK_BUILD_VERSION").unwrap_or_else(|_| "development".into()));}\n')
        self.git('init','--initial-branch=main');self.git('add','.');self.git('commit','-m','feat: owned SDK fixture')
        self.sha=self.git('rev-parse','HEAD');self.tag='v0.3.7';self.git('tag',self.tag);self.url=self.sdk.as_uri()
        self.consumer=self.root/'consumer';(self.consumer/'src').mkdir(parents=True)
        (self.consumer/'Cargo.toml').write_text('[package]\nname="owned_consumer"\nedition="2024"\n[dependencies]\nowned_sdk={git='+json.dumps(self.url)+',tag='+json.dumps(self.tag)+'}\n')
        (self.consumer/'src/main.rs').write_text('fn main() {println!("{}",owned_sdk::answer());}\n')
        self.run_cargo('generate-lockfile')  # file:// Git only; there are no registry dependencies.

    def git(self,*args):return subprocess.check_output(['git',*args],cwd=self.sdk,env=self.env,stderr=subprocess.PIPE,text=True).strip()
    def run_cargo(self,*args):return subprocess.check_output([self.cargo,*args],cwd=self.consumer,env=self.env,stderr=subprocess.PIPE,text=True,timeout=60)
    def metadata(self):return json.loads(self.run_cargo('metadata','--format-version','1','--locked','--offline'))
    def verify(self,data):return verify_sdk_dependency(data,'owned_sdk',self.url,self.tag,self.sha)

    def test_git_tag_and_lock_commit_are_actual_cargo_identity(self):
        data=self.metadata();identity=self.verify(data)
        self.assertEqual(identity['git_tag'],self.tag);self.assertEqual(identity['source_sha'],self.sha)
        self.assertEqual(self.run_cargo('run','--quiet','--locked','--offline').strip(),'5')
        self.git('commit','--allow-empty','-m','fix: advanced SDK branch')
        self.assertEqual(self.verify(self.metadata()),identity,'branch advancement must not update tag pin')

    def test_temporary_local_patch_builds_but_official_identity_refuses(self):
        local=self.root/'local-sdk';shutil.copytree(self.sdk,local,ignore=shutil.ignore_patterns('.git'))
        (local/'src/lib.rs').write_text('pub fn answer() -> u8 {6}\n')
        (self.consumer/'.cargo').mkdir()
        config=self.consumer/'.cargo/config.toml'
        config.write_text('[patch.'+json.dumps(self.url)+']\nowned_sdk={path='+json.dumps(str(local))+'}\n')
        self.run_cargo('generate-lockfile','--offline')
        self.assertEqual(self.run_cargo('run','--quiet','--locked','--offline').strip(),'6')
        with self.assertRaises(Refusal):self.verify(self.metadata())
        with self.assertRaises(Refusal):sdk_build_environment(self.metadata(),'owned_sdk',self.url,self.tag,self.sha)
        config.unlink();self.run_cargo('generate-lockfile','--offline')
        self.verify(self.metadata());self.assertEqual(self.run_cargo('run','--quiet','--locked','--offline').strip(),'5')

    def test_tag_pin_requires_separate_build_time_runtime_identity(self):
        data=self.metadata()
        sdk=next(p for p in data['packages'] if p['name']=='owned_sdk')
        self.assertEqual(sdk['version'],'0.0.0','Cargo does not infer package version from tag')
        (self.consumer/'src/main.rs').write_text('fn main() {println!("{} {}",owned_sdk::package_version(),owned_sdk::build_version());}\n')
        self.assertEqual(self.run_cargo('run','--quiet','--locked','--offline').strip(),'0.0.0 development')
        build_env=sdk_build_environment(data,'owned_sdk',self.url,self.tag,self.sha)
        self.assertEqual(build_env,{'SDK_BUILD_TAG':self.tag,'SDK_BUILD_SHA':self.sha,'SDK_BUILD_VERSION':'0.3.7'})
        self.env.update(build_env)
        self.assertEqual(self.run_cargo('run','--quiet','--locked','--offline').strip(),'0.0.0 0.3.7')
        self.env.pop('SDK_BUILD_VERSION')
        self.assertEqual(self.run_cargo('run','--quiet','--locked','--offline').strip(),'0.0.0 development','Cargo must invalidate build identity cache')
        cached=Path(sdk['manifest_path'])
        before=cached.read_text()
        with cached.open('a') as f:f.write('\n# changed cached source\n')
        with self.assertRaises(Refusal):sdk_build_environment(data,'owned_sdk',self.url,self.tag,self.sha)
        cached.write_text(before)
        config_dir=cached.parent/'.cargo';config_dir.mkdir()
        (config_dir/'config.toml').write_text('# untracked developer configuration')
        with self.assertRaises(Refusal):sdk_build_environment(data,'owned_sdk',self.url,self.tag,self.sha)

    def test_runtime_identity_refuses_maintained_source_versions(self):
        data=self.metadata();sdk=next(p for p in data['packages'] if p['name']=='owned_sdk')
        manifest=Path(sdk['manifest_path']);before=manifest.read_text()
        manifest.write_text(before.replace('[package]','[package]\nversion="0.3.7"'))
        with self.assertRaises(Refusal):sdk_build_environment(data,'owned_sdk',self.url,self.tag,self.sha)
        manifest.write_text(before)
        (manifest.parent/'Cargo.lock').write_text('version=4\n[[package]]\nname="owned_sdk"\nversion="0.3.7"\n')
        with self.assertRaises(Refusal):sdk_build_environment(data,'owned_sdk',self.url,self.tag,self.sha)

    def test_wrong_commit_or_ambiguous_sdk_refuses(self):
        data=self.metadata()
        with self.assertRaises(Refusal):verify_sdk_dependency(data,'owned_sdk',self.url,self.tag,'b'*40)
        matches=[p for p in data['packages'] if p['name']=='owned_sdk'];data['packages'].append(matches[0])
        with self.assertRaises(Refusal):self.verify(data)

if __name__=='__main__':unittest.main()
