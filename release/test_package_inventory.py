"""Owned explicit-package inventory checks; no real SDK packaging/publication."""
import io
import json
import os
from pathlib import Path
import subprocess
import tarfile
import unittest
import test_stage_source
from stage_source import stage, Refusal, verify_staged_source, distribution_manifest


class PackageInventory(unittest.TestCase):
    setUp = test_stage_source.SourceStaging.setUp
    git = test_stage_source.SourceStaging.git

    def prepare(self, include=None):
        (self.repo/'src/main.rs').unlink()
        (self.repo/'README.md').write_text('Owned fixture only\n')
        (self.repo/'research').mkdir()
        (self.repo/'research/excluded.md').write_text('Tracked research stays out\n')
        (self.repo/'tools').mkdir()
        (self.repo/'tools/private-decoy.txt').write_text('Original harmless text\n')
        paths = include if include is not None else ['/Cargo.toml','/Cargo.lock','/README.md','/src/lib.rs']
        with (self.repo/'Cargo.toml').open('a') as f:
            f.write('include = '+json.dumps(paths)+'\n')
        self.git('add','.'); self.git('commit','-m','test: owned explicit package inputs')
        self.sha=self.git('rev-parse','HEAD'); self.git('tag','v0.3.8')

    def packaged(self):
        self.prepare()
        identity=stage(self.repo,'v0.3.8',self.sha,self.out)
        cargo=os.environ['SDK_FIXTURE_CARGO']
        for args in [('generate-lockfile','--offline'),('package','--locked','--offline')]:
            subprocess.check_output([cargo,*args],cwd=self.out,env=self.env,stderr=subprocess.PIPE,timeout=60)
        return identity,self.out/'target/package/owned_fixture-0.3.8.crate'

    def rewrite(self, original, *, drop=None, append=None, change=None, append_data=None):
        path=self.root/'tampered.crate'
        with tarfile.open(original,'r:gz') as src,tarfile.open(path,'w:gz') as dst:
            for member in src.getmembers():
                name=member.name.removeprefix('owned_fixture-0.3.8/')
                if name==drop: continue
                data=src.extractfile(member).read() if member.isfile() else None
                if name==change:
                    data=data+b'\n[dependencies]\nunknown = \"9.9.9\"\n' if name=='Cargo.toml' else b'changed source'
                    member.size=len(data)
                dst.addfile(member,io.BytesIO(data) if data is not None else None)
            if append:
                member=tarfile.TarInfo('owned_fixture-0.3.8/'+append);data=append_data if append_data is not None else b'Unexpected harmless text'
                member.size=len(data);dst.addfile(member,io.BytesIO(data))
        return path

    def test_actual_declared_crate_excludes_tracked_research_but_stage_stays_verified(self):
        identity,artifact=self.packaged()
        self.assertIn('research/excluded.md',identity['build_inputs'])
        self.assertNotIn('research/excluded.md',identity['package_inputs'])
        self.assertEqual(set(identity['package_inputs']),{'Cargo.toml','README.md','src/lib.rs'})
        self.assertEqual(distribution_manifest(self.out,artifact)['source_sha'],self.sha)
        (self.out/'research/excluded.md').write_text('Changed excluded build source')
        with self.assertRaises(Refusal):verify_staged_source(self.out)

    def test_missing_changed_or_unexpected_archive_members_refuse(self):
        _,artifact=self.packaged()
        for args in [{'drop':'src/lib.rs'},{'change':'src/lib.rs'},{'change':'Cargo.toml'},
                     {'append':'tools/private-decoy.txt'},{'append':'research/excluded.md'}]:
            with self.subTest(args=args),self.assertRaises(Refusal):
                distribution_manifest(self.out,self.rewrite(artifact,**args))

    def test_patterns_refuse(self):
        self.prepare(['/Cargo.toml','/src/**'])
        with self.assertRaises(Refusal):stage(self.repo,'v0.3.8',self.sha,self.out)

    def test_noncanonical_declared_path_refuses(self):
        self.prepare(['/Cargo.toml','/Cargo.lock','/src/../src/lib.rs'])
        with self.assertRaises(Refusal):stage(self.repo,'v0.3.8',self.sha,self.out)

    def test_missing_declared_source_refuses(self):
        self.prepare(['/Cargo.toml','/Cargo.lock','/src/missing.rs'])
        with self.assertRaises(Refusal):stage(self.repo,'v0.3.8',self.sha,self.out)

    def test_generated_vcs_metadata_must_match_tagged_source(self):
        _,artifact=self.packaged()
        metadata={'git':{'sha1':self.sha},'path_in_vcs':''}
        valid=self.rewrite(artifact,append='.cargo_vcs_info.json',append_data=json.dumps(metadata).encode())
        self.assertEqual(distribution_manifest(self.out,valid)['source_sha'],self.sha)
        for bad in [b'invalid JSON', b'[]', json.dumps({'git':{'sha1':'a'*40},'path_in_vcs':''}).encode(),
                    json.dumps({'git':{'sha1':self.sha,'dirty':True},'path_in_vcs':''}).encode()]:
            with self.subTest(bad=bad),self.assertRaises(Refusal):
                distribution_manifest(self.out,self.rewrite(artifact,append='.cargo_vcs_info.json',append_data=bad))


if __name__=='__main__':unittest.main()
