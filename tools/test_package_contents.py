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
SOURCES = {"src/lib.rs", "src/capture.rs", "src/evidence.rs", "src/mock.rs", "src/capabilities.rs", "src/development_capture.rs", "src/navigation.rs", "src/navigation/gesture.rs", "src/native_adapter.rs", "src/native_adapter/transport.rs", "src/native_adapter/capture.rs"}
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
    def test_development_version_is_generated_not_declared(self):
        for root in (ROOT, ROOT / "tests/fixtures/consumer-boundary"):
            manifest = tomllib.loads((root / "Cargo.toml").read_text(encoding="utf-8"))
            self.assertNotIn("version", manifest["package"])
            self.assertNotIn("version", manifest.get("workspace", {}).get("package", {}))
            lock = tomllib.loads((root / "Cargo.lock").read_text(encoding="utf-8"))
            own = [p for p in lock["package"] if p["name"] == manifest["package"]["name"]]
            self.assertEqual(len(own), 1)
            self.assertEqual(own[0]["version"], "0.0.0")
            self.assertNotIn("source", own[0])
            self.assertNotIn("checksum", own[0])
            result = subprocess.run(
                ["cargo", "metadata", "--format-version", "1", "--no-deps", "--locked", "--offline"],
                cwd=root, capture_output=True, text=True, encoding="utf-8", check=True, timeout=60,
            )
            package = next(p for p in json.loads(result.stdout)["packages"] if p["name"] == own[0]["name"])
            self.assertEqual(package["version"], "0.0.0")

    def test_license_metadata_and_current_gesture_provenance(self):
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
        # Optional dependency pins are preserved while the default graph stays empty.
        self.assertTrue(all(d["optional"] for d in package["dependencies"] if d["kind"] is None))
        self.assertEqual(package["features"]["default"], [])
        self.assertEqual({d["name"] for d in package["dependencies"]}, {"image", "sha2", "serde", "serde_json", "nix"})
        notices = (ROOT / "THIRD_PARTY_NOTICES.md").read_text(encoding="utf-8")
        self.assertIn("ded1caddccd30d6452fc2613a017f0118b0c575b", notices)
        self.assertIn("GPL-3.0-only", notices)

    def test_locked_registry_and_complete_original_notices(self):
        inventory = json.loads((ROOT / "tools/dependency-license-inventory.json").read_text(encoding="utf-8"))
        lock = tomllib.loads((ROOT / "Cargo.lock").read_text(encoding="utf-8"))
        registry = {(p["name"], p["version"], p["source"], p["checksum"])
                    for p in lock["package"] if p.get("source")}
        expected = {(p["name"], p["version"], "registry+https://github.com/rust-lang/crates.io-index", p["registry_checksum"])
                    for p in inventory["packages"]}
        self.assertEqual(registry, expected)
        result = subprocess.run(
            ["cargo", "metadata", "--format-version", "1", "--locked", "--offline", "--all-features"],
            cwd=ROOT, capture_output=True, text=True, encoding="utf-8", check=True, timeout=60,
        )
        packages = {(p["name"], p["version"]): p for p in json.loads(result.stdout)["packages"] if p.get("source")}
        bundle = (ROOT / "THIRD_PARTY_NOTICES.md").read_bytes()
        for package in inventory["packages"]:
            metadata = packages[(package["name"], package["version"])]
            root = Path(metadata["manifest_path"]).parent
            for notice in package["license_notice_files"]:
                with self.subTest(package=package["name"], notice=notice["path"]):
                    original = (root / notice["path"]).read_bytes()
                    self.assertEqual(hashlib.sha256(original).hexdigest(), notice["sha256"])
                    self.assertIn(original, bundle)

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
            without, count = re.subn(r"(?ms)^include = \[.*?\]\n", "", before)
            self.assertEqual(count, 1)
            manifest.write_text(without, encoding="utf-8")
            self.assertTrue(decoys <= listed(root), "Old manifest must reproduce actual leak")


if __name__ == "__main__":
    unittest.main()
