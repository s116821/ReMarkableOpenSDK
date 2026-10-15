"""Check the actual SDK as an ordinary feature-disabled dependency; no native work.

Run with --toolchain 1.88.0 (or an already installed toolchain). Dependencies must
be cached first. No lockfile updates, downloads, publication or device access.
"""
import argparse
import json
from pathlib import Path
import subprocess
import tomllib


ROOT = Path(__file__).resolve().parents[1]
FIXTURE = ROOT / "tests" / "fixtures" / "consumer-boundary"


def run(cargo, *args):
    return subprocess.run(
        [*cargo, *args], cwd=FIXTURE, text=True, encoding="utf-8",
        stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=300,
    )


def require_success(result):
    if result.returncode:
        raise RuntimeError(result.stdout + result.stderr)
    return result.stdout


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--toolchain", required=True)
    args = parser.parse_args()
    cargo = ["cargo", "+" + args.toolchain]
    print(require_success(run(cargo, "--version")).strip(), flush=True)
    sdk_lock = tomllib.loads((ROOT / "Cargo.lock").read_text(encoding="utf-8"))
    consumer_lock = tomllib.loads((FIXTURE / "Cargo.lock").read_text(encoding="utf-8"))
    def registry_pins(lock):
        return {(p["name"], p["version"], p.get("source"), p.get("checksum"))
                for p in lock["package"] if p.get("source")}
    if not registry_pins(consumer_lock) <= registry_pins(sdk_lock):
        raise RuntimeError("Consumer registry pins differ from SDK Cargo.lock")
    common = ["--locked", "--offline"]
    metadata = json.loads(require_success(run(cargo, "metadata", *common, "--format-version", "1")))
    sdk = [p for p in metadata["packages"] if p["name"] == "remarkable-open-sdk"]
    if len(sdk) != 1 or Path(sdk[0]["manifest_path"]).resolve() != ROOT / "Cargo.toml":
        raise RuntimeError("Consumer must resolve exactly this SDK checkout")
    node = next(n for n in metadata["resolve"]["nodes"] if n["id"] == sdk[0]["id"])
    if node["features"]:
        raise RuntimeError(f"SDK features leaked into consumer: {node['features']}")
    print(require_success(run(cargo, "run", *common, "--bin", "consumer")).strip())
    for binary, module in [("forbidden-mock", "mock"), ("forbidden-development-capture", "development_capture")]:
        result = run(cargo, "check", *common, "--bin", binary, "--message-format=json")
        errors = []
        for line in result.stdout.splitlines():
            message = json.loads(line)
            if message.get("reason") == "compiler-message" and message["message"]["level"] == "error":
                errors.append(message["message"])
        expected = f"unresolved import `remarkable_open_sdk::{module}`"
        if result.returncode != 101 or not errors or any(
            (e.get("code") or {}).get("code") != "E0432" or e["message"] != expected
            for e in errors
        ):
            raise RuntimeError(f"Wrong failure for {binary}:\n{result.stdout}\n{result.stderr}")
        print(f"{binary}: expected E0432 (unavailable SDK module)")
    print("consumer-boundary: PASS (source/host only; native support remains unavailable)")


if __name__ == "__main__":
    main()
