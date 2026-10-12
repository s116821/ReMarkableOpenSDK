"""Generated root metadata is not source version authority; owned Cargo only."""
import io
import os
from pathlib import Path
import subprocess
import tarfile
import tomllib
import unittest
import test_stage_source
from stage_source import stage, verify_staged_source, distribution_manifest, Refusal, source_lock_inventory

class GeneratedLock(unittest.TestCase):
    setUp=test_stage_source.SourceStaging.setUp
    git=test_stage_source.SourceStaging.git
    def source(self):
        cargo=os.environ['SDK_FIXTURE_CARGO']
        subprocess.check_output([cargo,'generate-lockfile','--offline'],cwd=self.repo,env=self.env,stderr=subprocess.PIPE,timeout=60)
        (self.repo/'tests').mkdir();(self.repo/'tests/nested.lock').write_text('owned nested lock input')
        nested=self.repo/'tests'/'fixture';nested.mkdir();(nested/'Cargo.lock').write_text('owned nested Cargo lock input')
        self.git('add','.');self.git('commit','-m','test: generated default lock fixture')
        self.sha=self.git('rev-parse','HEAD');self.git('tag','v0.3.8')
        return cargo,(self.repo/'Cargo.lock').read_bytes()

    def test_generated_default_stages_only_from_tag_and_preserves_source(self):
        cargo,before=self.source()
        own=tomllib.loads(before.decode())['package'][0];self.assertEqual(own['version'],'0.0.0')
        identity=stage(self.repo,'v0.3.8',self.sha,self.out)
        self.assertEqual(identity['dependency_lock'],[])
        self.assertIn('tests/fixture/Cargo.lock',identity['build_inputs'])
        with self.assertRaises(Refusal):verify_staged_source(self.out)
        subprocess.check_output([cargo,'generate-lockfile','--offline'],cwd=self.out,env=self.env,stderr=subprocess.PIPE,timeout=60)
        self.assertEqual(verify_staged_source(self.out),identity)
        self.assertEqual(tomllib.loads((self.out/'Cargo.lock').read_text())['package'][0]['version'],'0.3.8')
        subprocess.check_output([cargo,'package','--locked','--offline'],cwd=self.out,env=self.env,stderr=subprocess.PIPE,timeout=60)
        artifact=self.out/'target/package/owned_fixture-0.3.8.crate'
        self.assertEqual(distribution_manifest(self.out,artifact)['version'],'0.3.8')
        self.assertEqual((self.repo/'Cargo.lock').read_bytes(),before);self.assertEqual(self.git('status','--porcelain'),'')
        # Packaged own default/non-tag version must refuse too.
        tampered=self.root/'tampered.crate'
        with tarfile.open(artifact,'r:gz') as src,tarfile.open(tampered,'w:gz') as dst:
            for member in src.getmembers():
                data=src.extractfile(member).read() if member.isfile() else None
                if member.name.endswith('/Cargo.lock'):data=data.replace(b'0.3.8',b'0.0.0');member.size=len(data)
                dst.addfile(member,io.BytesIO(data) if data is not None else None)
        with self.assertRaises(Refusal):distribution_manifest(self.out,tampered)
        (self.out/'tests/fixture/Cargo.lock').write_text('changed nested input')
        with self.assertRaises(Refusal):verify_staged_source(self.out)

    def test_only_unique_unsourced_generated_default_is_accepted(self):
        good={'name':'owned_fixture','version':'0.0.0','dependencies':['external']}
        external={'name':'external','version':'1.2.3','source':'registry+fixture','checksum':'a'*64}
        self.assertEqual(source_lock_inventory({'package':[good,external]},'owned_fixture'),(good,[external]))
        for nodes in [[{**good,'version':'0.3.8'}],[good,good],[{**good,'source':'registry+fixture'}],[{**good,'checksum':'a'*64}]]:
            with self.subTest(nodes=nodes),self.assertRaises(Refusal):source_lock_inventory({'package':nodes},'owned_fixture')
        self.assertEqual(source_lock_inventory({'package':[external]},'owned_fixture'),(None,[external]))

    def test_external_entry_drift_refuses_after_cargo_regeneration(self):
        cargo,_=self.source();identity=stage(self.repo,'v0.3.8',self.sha,self.out)
        subprocess.check_output([cargo,'generate-lockfile','--offline'],cwd=self.out,env=self.env,stderr=subprocess.PIPE,timeout=60)
        with (self.out/'Cargo.lock').open('a') as f:f.write('\n[[package]]\nname="unexpected"\nversion="1.2.3"\nsource="registry+fixture"\n')
        with self.assertRaises(Refusal):verify_staged_source(self.out)

import test_cargo_consumer
from stage_source import sdk_build_environment

class GeneratedGitConsumer(unittest.TestCase):
    setUp=test_cargo_consumer.CargoConsumerIdentity.setUp
    git=test_cargo_consumer.CargoConsumerIdentity.git
    run_cargo=test_cargo_consumer.CargoConsumerIdentity.run_cargo
    metadata=test_cargo_consumer.CargoConsumerIdentity.metadata
    def test_committed_default_lock_and_immutable_git_tag_runtime_identity(self):
        subprocess.check_output([self.cargo,'generate-lockfile','--offline'],cwd=self.sdk,env=self.env,stderr=subprocess.PIPE,timeout=60)
        before=(self.sdk/'Cargo.lock').read_bytes()
        self.git('add','Cargo.lock');self.git('commit','-m','test: committed Cargo default metadata')
        self.sha=self.git('rev-parse','HEAD');old=self.tag;self.tag='v0.3.8';self.git('tag',self.tag)
        manifest=self.consumer/'Cargo.toml';manifest.write_text(manifest.read_text().replace(old,self.tag))
        self.run_cargo('generate-lockfile') # new file:// fixture tag only, no registry/network dependency
        data=self.metadata();sdk=next(p for p in data['packages'] if p['name']=='owned_sdk')
        self.assertEqual(sdk['version'],'0.0.0')
        self.assertTrue(sdk['source'].endswith('#'+self.sha))
        build_env=sdk_build_environment(data,'owned_sdk',self.url,self.tag,self.sha)
        self.assertEqual(build_env['SDK_BUILD_VERSION'],'0.3.8')
        self.env.update(build_env)
        self.assertEqual(self.run_cargo('run','--quiet','--locked','--offline').strip(),'5')
        self.assertEqual((self.sdk/'Cargo.lock').read_bytes(),before)
