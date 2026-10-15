"""Actual selector controls; no SDK compilation or tablet work."""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

SCRIPT=Path(__file__).with_name('select_consumer_source.sh').resolve()

class SourceSelection(unittest.TestCase):
    def selected(self, files):
        with tempfile.TemporaryDirectory(prefix='sdk-source-selection-') as directory:
            root=Path(directory)
            for name in files:
                path=root/name;path.parent.mkdir(parents=True,exist_ok=True);path.write_text('original harmless fixture')
            env={**os.environ,'GITHUB_OUTPUT':str(root/'output'),'GITHUB_STEP_SUMMARY':str(root/'summary')}
            result=subprocess.run(['bash',str(SCRIPT)],cwd=root,env=env,capture_output=True,text=True,timeout=10)
            output=(root/'output').read_text() if (root/'output').exists() else ''
            return result,output

    def test_genuine_sdk_free_and_complete_candidate(self):
        result,output=self.selected(['release/tool.py','tools/unrelated-research.py'])
        self.assertEqual(result.returncode,0,result.stderr);self.assertEqual(output,'mode=historical\n')
        result,output=self.selected(['Cargo.toml','tools/check_consumer_boundary.py','tools/test_package_contents.py','tests/fixtures/consumer-boundary/Cargo.toml','tests/fixtures/consumer-boundary/Cargo.lock','src/lib.rs'])
        self.assertEqual(result.returncode,0,result.stderr);self.assertEqual(output,'mode=candidate\n')

    def test_all_reproduced_partial_inputs_refuse_without_success_mode(self):
        for name in ['tools/test_package_contents.py','src/capture.rs','tests/fixtures/consumer-boundary/Cargo.toml','src/lib.rs','tools/check_consumer_boundary.py','Cargo.lock']:
            with self.subTest(name=name):
                result,output=self.selected([name])
                self.assertNotEqual(result.returncode,0);self.assertEqual(output,'')

    def test_candidate_missing_required_tools_cannot_fall_back(self):
        result,output=self.selected(['Cargo.toml'])
        self.assertNotEqual(result.returncode,0);self.assertEqual(output,'')
