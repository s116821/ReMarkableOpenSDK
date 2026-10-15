import os
from pathlib import Path
import tempfile
import unittest
from qualify_source_publisher import prepare, LOCK, LOCK_SHA256, digest


class PublisherSourceGuards(unittest.TestCase):
    def setUp(self):
        self.source = Path(os.environ.get('SDK_PUBLISHER_SOURCE', '.upstream/source-publisher')).resolve()
        if not self.source.is_dir():
            self.skipTest('Exact source checkout required; hosted/local explicit probe supplies it')

    def test_actual_source_copied_with_supported_lock_and_unchanged_license(self):
        with tempfile.TemporaryDirectory(prefix='sdk-publisher-prepare-owned-') as root:
            output = Path(root) / 'attempt'
            built, paths = prepare(self.source, output)
            self.assertEqual(digest((built / 'package-lock.json').read_bytes()), LOCK_SHA256)
            self.assertIn('src/main.ts', paths)
            self.assertEqual((built / 'LICENSE').read_bytes(), (self.source / 'LICENSE').read_bytes())
            with self.assertRaises(ValueError):
                prepare(self.source, output)

    def test_changed_lock_refuses_before_any_staging_or_install(self):
        with tempfile.TemporaryDirectory(prefix='sdk-publisher-tamper-owned-') as root:
            root = Path(root)
            bad = root / 'lock'
            bad.write_bytes(LOCK.read_bytes() + b' ')
            output = root / 'attempt'
            with self.assertRaises(ValueError):
                prepare(self.source, output, lock=bad)
            self.assertFalse(output.exists())


if __name__ == '__main__':
    unittest.main()
