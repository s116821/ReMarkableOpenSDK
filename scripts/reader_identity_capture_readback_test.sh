#!/bin/sh
# Actual local own-process /proc and files; no target/device admission.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
temporary=
out=${READER_CAPTURE_READBACK_OUTPUT:-}
if [ -z "$out" ]; then out=$(mktemp -d); temporary=$out; fi
trap 'if [ -n "$temporary" ]; then rm -rf "$temporary"; fi' EXIT
mkdir -p "$out"
READER_CAPTURE_READBACK_ASSETS=${READER_CAPTURE_READBACK_ASSETS:-$root/../tests/fixtures/identity-capture-pair/case-1}
READER_CAPTURE_READBACK_EXPORT=$out/frame-exports
export READER_CAPTURE_READBACK_ASSETS READER_CAPTURE_READBACK_EXPORT
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then . "$QT_PROBE_SDK_ENV"; fi
flags='-std=c++17 -Wall -Wextra -Werror -O1 -g -fPIC -pthread'
if [ "${READER_CAPTURE_READBACK_SANITIZERS:-0}" = 1 ]; then flags="$flags -fsanitize=address,undefined -fno-omit-frame-pointer"; fi
${CXX:-c++} --version > "$out/compiler.txt"
pkg-config --modversion Qt6Core >> "$out/compiler.txt"
${CXX:-c++} $flags -I"$root" "$root/reader_identity_capture_readback_fixture.cpp" $(pkg-config --cflags --libs Qt6Core) -o "$out/fixture"
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
 ${CXX:-c++} $flags -I"$root" "$root/reader_identity_capture_readback.cpp" $(pkg-config --cflags --libs Qt6Core) -o "$out/production-reader-compile-only"
 "$READELF" -h -A "$out/production-reader-compile-only" > "$out/production-elf.txt"
 timeout --kill-after=1s 90s "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" -E LANG=C.UTF-8 "$out/fixture"
 sha256sum "$out/production-reader-compile-only" > "$out/production-artifact-hash.txt"
else
 ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 timeout --kill-after=1s 90s "$out/fixture"
fi
sha256sum "$out/fixture" > "$out/artifact-hashes.txt"
