"""Original cross-platform tagged source fixture; no Cargo/native build."""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
from stage_source import stage, verify_staged_source, Refusal

class PortableStage(unittest.TestCase):
    def check_paths(self, ending):
        with tempfile.TemporaryDirectory(prefix='sdk-portable-stage-') as directory:
            root=Path(directory);repo=root/'source';repo.mkdir();out=root/'stage'
            config=root/'empty.gitconfig';config.write_text('')
            env={**os.environ,'GIT_CONFIG_NOSYSTEM':'1','GIT_CONFIG_GLOBAL':str(config),
                 'GIT_AUTHOR_NAME':'Owned Fixture','GIT_COMMITTER_NAME':'Owned Fixture',
                 'GIT_AUTHOR_EMAIL':'fixture@example.invalid','GIT_COMMITTER_EMAIL':'fixture@example.invalid'}
            def git(*args):
                return subprocess.check_output(['git',*args],cwd=repo,env=env,stderr=subprocess.PIPE,text=True).strip()
            git('init','--initial-branch=main');git('config','core.autocrlf','false')
            (repo/'src').mkdir();(repo/'src/lib.rs').write_text('pub fn answer()->u8{1}\n')
            (repo/'README.md').write_text('Original owned portable fixture\n')
            manifest='[package]\nname="owned_portable"\nedition="2024"\ninclude=["/Cargo.toml","/Cargo.lock","/README.md","/src/lib.rs"]\n'
            (repo/'Cargo.toml').write_bytes(manifest.replace('\n',ending).encode())
            git('add','.');git('commit','-m','test: original portable fixture');sha=git('rev-parse','HEAD');git('tag','v0.3.7')
            identity=stage(repo,'v0.3.7',sha,out)
            self.assertIn(('version = \"0.3.7\"'+ending).encode(),(out/'Cargo.toml').read_bytes())
            self.assertNotIn(b'\r\r\n',(out/'Cargo.toml').read_bytes())
            self.assertIn('src/lib.rs',identity['build_inputs']);self.assertIn('src/lib.rs',identity['package_inputs'])
            self.assertFalse(any('\\' in name for name in identity['build_inputs']))
            target=out/'target'/'nested';target.mkdir(parents=True);(target/'generated.txt').write_text('owned build output')
            source_check=verify_staged_source(out,source_only=True)
            self.assertEqual(source_check.pop('verification_phase'),'source-only')
            self.assertIs(source_check.pop('lock_verified'),False)
            self.assertEqual(source_check,identity)
            with self.assertRaises(Refusal):verify_staged_source(out)
            (out/'src/lib.rs').write_text('changed staged fixture')
            with self.assertRaises(Refusal):verify_staged_source(out)
            self.assertEqual(git('rev-parse','HEAD'),sha);self.assertEqual(git('status','--porcelain'),'')

    def test_literal_posix_inputs_target_exclusion_and_source_drift(self):
        self.check_paths('\n')

    def test_crlf_manifest_preserves_line_endings_and_posix_keys(self):
        self.check_paths('\r\n')
