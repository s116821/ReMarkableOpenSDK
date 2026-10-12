"""List-only source-package containment regressions; never build/archive/publish."""
from pathlib import Path
import hashlib
import json
import re
import shutil
import subprocess
import tempfile
import tomllib
import unittest


ROOT = Path(__file__).resolve().parents[1]
SOURCES = {
    "src/lib.rs", "src/capabilities.rs", "src/capture.rs",
    "src/development_capture.rs", "src/evidence.rs", "src/mock.rs",
    "src/navigation.rs", "src/navigation/gesture.rs",
}
REQUIRED = SOURCES | {"Cargo.toml", "Cargo.lock", "README.md", "LICENSE", "THIRD_PARTY_NOTICES.md"}
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
    def test_license_metadata_and_complete_original_notices(self):
        self.assertEqual(
            hashlib.sha256((ROOT / "LICENSE").read_bytes()).hexdigest(),
            "3972dc9744f6499f0f9b2dbf76696f2ae7ad8af9b23dde66d6af86c9dfb36986",
            "LICENSE must remain the unmodified canonical GNU GPLv3 text",
        )
        manifest = tomllib.loads((ROOT / "Cargo.toml").read_text(encoding="utf-8"))
        self.assertEqual(manifest["package"]["license"], "GPL-3.0-only")
        self.assertIs(manifest["package"]["publish"], False)
        result = subprocess.run(
            ["cargo", "metadata", "--format-version", "1", "--no-deps", "--locked", "--offline"],
            cwd=ROOT, capture_output=True, text=True, encoding="utf-8", check=True, timeout=60,
        )
        package = next(p for p in json.loads(result.stdout)["packages"] if p["name"] == "remarkable-open-sdk")
        self.assertEqual(package["license"], "GPL-3.0-only")
        self.assertEqual(package["publish"], [])
        inventory = json.loads((ROOT / "docs/reference/dependency-license-inventory.json").read_text(encoding="utf-8"))
        lock = tomllib.loads((ROOT / "Cargo.lock").read_text(encoding="utf-8"))
        current_pins = {(p["name"], p["version"], p["checksum"])
                        for p in lock["package"] if p.get("source")}
        notice_pins = {(p["name"], p["version"], p["registry_checksum"])
                       for p in inventory["packages"]}
        self.assertEqual(current_pins, notice_pins, "Dependency changes require a refreshed notice inventory")
        bundle = (ROOT / "THIRD_PARTY_NOTICES.md").read_bytes()
        chunks = re.findall(
            rb"### SHA-256 ([0-9a-f]{64})\n\nOriginal bytes: ([0-9]+)\n\n```text\n(.*?)```",
            bundle, re.S,
        )
        hashes = []
        for digest, size, payload in chunks:
            length = int(size)
            self.assertIn(payload[length:], (b"", b"\n"))
            self.assertEqual(hashlib.sha256(payload[:length]).hexdigest().encode(), digest)
            hashes.append(digest.decode())
        expected = {n["sha256"] for p in inventory["packages"] for n in p["license_notice_files"]}
        self.assertEqual(len(hashes), len(set(hashes)))
        self.assertEqual(set(hashes), expected)
        for p in inventory["packages"]:
            for n in p["license_notice_files"]:
                row = f"| {p['name']} {p['version']} | {p['declared_license']} | [{n['path']}]({n['url']}) | [{n['sha256']}](#notice-{n['sha256']}) |"
                self.assertIn(row.encode(), bundle)

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
