"""List-only source-package containment regressions; never build/archive/publish."""
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
SOURCES = {
    "src/lib.rs", "src/capabilities.rs", "src/capture.rs",
    "src/development_capture.rs", "src/evidence.rs", "src/mock.rs",
    "src/navigation.rs", "src/navigation/gesture.rs",
}
REQUIRED = SOURCES | {"Cargo.toml", "Cargo.lock", "README.md"}
GENERATED = {"Cargo.toml.orig", ".cargo_vcs_info.json"}


def listed(root):
    result = subprocess.run(
        ["cargo", "package", "--list", "--allow-dirty", "--locked", "--offline"],
        cwd=root, capture_output=True, text=True, encoding="utf-8", timeout=60,
    )
    if result.returncode:
        raise AssertionError(result.stdout + result.stderr)
    paths = [Path(line).as_posix() for line in result.stdout.splitlines()]
    if len(set(paths)) != len(paths):
        raise AssertionError("Duplicate package member")
    return set(paths)


class PackageContents(unittest.TestCase):
    def assert_contents(self, paths):
        self.assertTrue(REQUIRED <= paths, REQUIRED - paths)
        self.assertFalse(paths - REQUIRED - GENERATED, paths - REQUIRED - GENERATED)

    def test_actual_checkout_contains_only_traced_rust_package(self):
        self.assert_contents(listed(ROOT))

    def test_untracked_decoys_are_excluded_and_old_manifest_reproduces_leak(self):
        with tempfile.TemporaryDirectory(prefix="sdk-package-contents-") as directory:
            root = Path(directory)
            for name in REQUIRED:
                target = root / name
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(ROOT / name, target)
            # A real isolated index distinguishes ordinary source from untracked
            # decoys. No remote, commit, user identity or historical file is used.
            subprocess.run(["git", "init", "--quiet"], cwd=root, check=True, capture_output=True)
            subprocess.run(["git", "add", "--", *sorted(REQUIRED)], cwd=root, check=True, capture_output=True)
            decoys = {"tools/private-development.inc", "native/unselected.cpp", "private-observation.json"}
            for name in decoys:
                path = root / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text("original harmless fixture text\n", encoding="utf-8")
            self.assert_contents(listed(root))
            manifest = root / "Cargo.toml"
            before = manifest.read_text(encoding="utf-8")
            without, count = re.subn(r"(?ms)^include = \[.*?^\]\n", "", before)
            self.assertEqual(count, 1)
            manifest.write_text(without, encoding="utf-8")
            self.assertTrue(decoys <= listed(root), "Old manifest must reproduce actual leak")


if __name__ == "__main__":
    unittest.main()
