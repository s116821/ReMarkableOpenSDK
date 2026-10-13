#!/usr/bin/env bash
# Source composition only; does not calculate versions, build or publish.
set -eu
if test -f Cargo.toml && ! test -L Cargo.toml; then
  test -f tools/check_consumer_boundary.py
  test -f tools/test_package_contents.py
  test -f tests/fixtures/consumer-boundary/Cargo.toml
  test -f tests/fixtures/consumer-boundary/Cargo.lock
  echo 'mode=candidate' >> "$GITHUB_OUTPUT"
  echo 'Consumer checks cover the actual SDK PR merge candidate.' >> "$GITHUB_STEP_SUMMARY"
else
  # A genuine SDK-free release checkout has no SDK src tree, either checker,
  # or consumer workspace subtree. Empty/broken/partial entries also refuse.
  if test -e Cargo.toml || test -L Cargo.toml || test -e Cargo.lock || test -L Cargo.lock || test -e src || test -L src || test -e tools/check_consumer_boundary.py || test -L tools/check_consumer_boundary.py || test -e tools/test_package_contents.py || test -L tools/test_package_contents.py || test -e tests/fixtures/consumer-boundary || test -L tests/fixtures/consumer-boundary; then
    echo 'Partial SDK input without Cargo.toml; refusing historical fallback.' >&2
    exit 1
  fi
  echo 'mode=historical' >> "$GITHUB_OUTPUT"
  echo 'HISTORICAL QUALIFICATION ONLY: SDK crate absent from candidate; current-source integration remains open.' >> "$GITHUB_STEP_SUMMARY"
fi
