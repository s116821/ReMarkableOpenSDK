#!/bin/sh
set -eu
out=${RESIDENT_TEST_OUTPUT:?fresh output directory required}
vectors=${RESIDENT_WIRE_VECTORS:?Rust-generated vector directory required}
mkdir -p "$out"
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then . "$QT_PROBE_SDK_ENV"; fi
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -O1 -g -fPIC \
    "$root/wire_test.cpp" $(pkg-config --cflags --libs Qt6Core) -o "$out/wire-test"
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
    timeout --kill-after=1s 20s "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" "$out/wire-test" "$vectors"
else
    timeout --kill-after=1s 20s "$out/wire-test" "$vectors"
fi
