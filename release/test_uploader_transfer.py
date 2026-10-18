"""Verify actual transferred uploader identity; never execute a modified tool."""
import json
import os
from pathlib import Path
import shutil
import tempfile
import unittest

from artifacts import sha256
from qualify_uploader import verify

BUNDLE = '379dc8bce8a264c2f2badae81199160fbd485ef118d63ade01fd2113a2e1b2bc'


class UploaderTransfer(unittest.TestCase):
    def setUp(self):
        self.assertTrue(os.environ.get('SDK_UPLOADER_FIXTURE'), 'Actual retained tool is required')
        self.assertTrue(os.environ.get('SDK_UPLOADER_PROVENANCE_SHA'), 'Trusted Checks digest is required')
        self.tool = Path(os.environ['SDK_UPLOADER_FIXTURE'])
        self.expected = os.environ['SDK_UPLOADER_PROVENANCE_SHA']

    def copied_tool(self):
        directory = tempfile.TemporaryDirectory(prefix='sdk-uploader-transfer-owned-')
        self.addCleanup(directory.cleanup)
        output = Path(directory.name) / 'tool'
        shutil.copytree(self.tool, output)
        return output

    def test_actual_retained_tool_matches_trusted_provenance(self):
        verify(self.tool, BUNDLE, expected_provenance=self.expected)

    def test_joint_metadata_and_provenance_change_refuses(self):
        tool = self.copied_tool()
        with (tool / 'action.yml').open('a') as metadata:
            metadata.write('\n# Owned harmless transfer mutation.\n')
        provenance = tool / 'provenance.json'
        record = json.loads(provenance.read_text())
        record['files']['action.yml'] = sha256(tool / 'action.yml')
        provenance.write_text(json.dumps(record))
        # The existing local inventory check alone cannot authenticate its record.
        verify(tool, BUNDLE)
        with self.assertRaisesRegex(ValueError, 'provenance differs'):
            verify(tool, BUNDLE, expected_provenance=self.expected)

    def test_changed_payload_refuses_with_unchanged_trusted_record(self):
        tool = self.copied_tool()
        with (tool / 'dist/index.js').open('ab') as payload:
            payload.write(b'\n// Owned harmless transfer mutation.\n')
        self.assertEqual(sha256(tool / 'provenance.json'), self.expected)
        with self.assertRaisesRegex(ValueError, 'files changed'):
            verify(tool, BUNDLE, expected_provenance=self.expected)
