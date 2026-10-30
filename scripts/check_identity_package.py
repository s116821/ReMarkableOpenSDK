"""Verify only the intended crate inputs are selected; no archive is built."""
import pathlib
import subprocess

root = pathlib.Path(__file__).resolve().parents[1]
result = subprocess.run(["cargo", "package", "--list", "--allow-dirty", "--offline"], cwd=root, check=True, capture_output=True, text=True)
actual = {line.replace(chr(92), "/") for line in result.stdout.splitlines()}
expected = {"Cargo.toml", "Cargo.lock", "Cargo.toml.orig", ".cargo_vcs_info.json", "LICENSE", "README.md", "src/lib.rs", "src/capture.rs", "src/evidence.rs", "src/mock.rs", "src/capabilities.rs", "src/development_capture.rs", "src/navigation.rs", "src/navigation/gesture.rs", "src/native_adapter.rs", "src/native_adapter/transport.rs", "src/native_adapter/capture.rs", "THIRD_PARTY_NOTICES.md"}
if actual != expected:
    raise SystemExit(f"Package mismatch: missing={expected-actual}; extra={actual-expected}")
print(f"PASS: exact {len(expected)}-member SDK package")
