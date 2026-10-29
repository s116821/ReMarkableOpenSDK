"""Retained draft byte-identity failures must not be hidden by matching names."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import tempfile
import unittest
from verify_assets import verify_downloaded_assets
from stage_source import Refusal

class DownloadedAssetIdentity(unittest.TestCase):
    def setUp(self):
        self.root=Path(tempfile.mkdtemp(prefix='sdk-assets-owned-'));self.addCleanup(shutil.rmtree,self.root)
        self.directory=self.root/'downloaded';self.directory.mkdir();self.data=b'owned artifact'
        self.expected={'schema':1,'git_tag':'v0.3.7','version':'0.3.7','source_sha':'a'*40,
                       'distributions':[{'kind':'rust-source-crate','name':'sdk.crate','size':len(self.data),'sha256':hashlib.sha256(self.data).hexdigest()}],
                       'native_operations':'unsupported','compatibility':{'rm1':'unqualified'}}
        (self.directory/'sdk.crate').write_bytes(self.data)
        (self.directory/'manifest.json').write_text(json.dumps(self.expected,indent=2))
    def verify(self,**kwargs):return verify_downloaded_assets(self.expected,self.directory,**kwargs)
    def test_complete_expected_bytes_and_semantic_manifest(self):
        result=self.verify();self.assertTrue(result['complete']);self.assertEqual(result['verified_assets'],['manifest.json','sdk.crate'])
    def test_partial_draft_subset_is_verified_but_not_complete(self):
        (self.directory/'sdk.crate').unlink()
        self.assertFalse(self.verify(require_complete=False)['complete'])
        with self.assertRaises(Refusal):self.verify()
    def test_same_name_wrong_bytes_refuse_even_in_partial_mode(self):
        (self.directory/'sdk.crate').write_bytes(b'wrong artifact')
        for complete in [True,False]:
            with self.subTest(complete=complete),self.assertRaises(Refusal):self.verify(require_complete=complete)
    def test_stale_manifest_identity_refuses(self):
        actual={**self.expected,'source_sha':'b'*40}
        (self.directory/'manifest.json').write_text(json.dumps(actual))
        with self.assertRaises(Refusal):self.verify()
    def test_symlinks_and_undeclared_assets_refuse(self):
        outside=self.root/'outside';outside.write_bytes(self.data)
        (self.directory/'sdk.crate').unlink();(self.directory/'sdk.crate').symlink_to(outside)
        with self.assertRaises(Refusal):self.verify()
        (self.directory/'sdk.crate').unlink();(self.directory/'sdk.crate').write_bytes(self.data)
        (self.directory/'extra').write_text('preserve')
        with self.assertRaises(Refusal):self.verify(require_complete=False)
        self.assertEqual((self.directory/'extra').read_text(),'preserve')
    def test_fifo_asset_refuses_without_waiting_for_a_writer(self):
        (self.directory/'sdk.crate').unlink();os.mkfifo(self.directory/'sdk.crate')
        with self.assertRaises(Refusal):self.verify()

    def test_invalid_expected_asset_names_and_duplicates_refuse(self):
        original=self.expected['distributions'][0]
        for name in ['../outside','/outside','bad\\path','manifest.json']:
            with self.subTest(name=name),self.assertRaises(Refusal):
                verify_downloaded_assets({**self.expected,'distributions':[{**original,'name':name}]},self.directory)
        with self.assertRaises(Refusal):verify_downloaded_assets({**self.expected,'distributions':[original,original]},self.directory)
    def test_duplicate_fields_types_and_oversized_manifest_refuse(self):
        path=self.directory/'manifest.json'
        path.write_text('{"schema":1,"schema":1}')
        with self.assertRaises(Refusal):self.verify()
        path.write_text(json.dumps({**self.expected,'schema':True}))
        with self.assertRaises(Refusal):self.verify()
        path.write_bytes(b' '*65537)
        with self.assertRaises(Refusal):self.verify()

if __name__=='__main__':unittest.main()
