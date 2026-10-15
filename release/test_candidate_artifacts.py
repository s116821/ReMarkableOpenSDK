"""Byte/refusal controls consume actual transferred SDK build outputs, never fake builds."""
import json
import os
from pathlib import Path
import shutil
import tempfile
import unittest

from candidate_artifacts import verify
from stage_source import Refusal


class TransferredArtifactRefusals(unittest.TestCase):
    def setUp(self):
        self.assertTrue(os.environ.get('SDK_CANDIDATE_FIXTURE'), 'Actual candidate build output required')
        self.assertTrue(os.environ.get('SDK_CANDIDATE_RECEIPT_SHA'), 'Trusted build-job receipt digest required')
        original = Path(os.environ['SDK_CANDIDATE_FIXTURE'])
        self.digest = os.environ['SDK_CANDIDATE_RECEIPT_SHA']
        receipt = json.loads((original / 'build-receipt.json').read_text())
        self.sha, self.tag = receipt['source_sha'], receipt['candidate_tag']
        verify(original, self.sha, self.tag, self.digest)
        self.root = Path(tempfile.mkdtemp(prefix='sdk-artifact-refusal-owned-'))
        self.addCleanup(shutil.rmtree, self.root)
        self.output = self.root / 'copy'
        shutil.copytree(original, self.output)
        self.receipt = receipt

    def test_changed_source_or_receipt_refuses(self):
        with self.assertRaises(Refusal):
            verify(self.output, '0' * 40, self.tag, self.digest)
        (self.output / 'build-receipt.json').write_text('{}')
        with self.assertRaises(Refusal):
            verify(self.output, self.sha, self.tag, self.digest)

    def test_changed_crate_bytes_refuse_before_test_execution(self):
        manifest = json.loads((self.output / 'release/manifest.json').read_text())
        crate = next(e for e in manifest['distributions'] if e['kind'] == 'rust-source-crate')
        (self.output / 'release' / crate['name']).write_bytes(b'owned tampered crate')
        with self.assertRaises(Refusal):
            verify(self.output, self.sha, self.tag, self.digest, execute_tests=True)

    def test_changed_test_executable_is_never_executed(self):
        executable = self.output / 'tests' / self.receipt['host_tests'][0]['name']
        marker = self.root / 'must-not-run'
        executable.write_text('#!/bin/sh\ntouch "' + str(marker) + '"\n')
        executable.chmod(0o700)
        with self.assertRaises(Refusal):
            verify(self.output, self.sha, self.tag, self.digest, execute_tests=True)
        self.assertFalse(marker.exists())

    def test_symlink_library_is_refused(self):
        manifest = json.loads((self.output / 'release/manifest.json').read_text())
        library = next(e for e in manifest['distributions'] if e['kind'] == 'rust-compiler-library')
        artifact = self.output / 'release' / library['name']
        regular = self.root / 'preserved-library'
        artifact.rename(regular)
        artifact.symlink_to(regular)
        with self.assertRaises(Refusal):
            verify(self.output, self.sha, self.tag, self.digest, execute_tests=True)
