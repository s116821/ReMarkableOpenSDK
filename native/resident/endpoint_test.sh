#!/bin/sh
set -eu
out=${RESIDENT_TEST_OUTPUT:?fresh output directory required}
vectors=${RESIDENT_WIRE_VECTORS:?Rust-generated vector directory required}
mkdir -p "$out"
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then . "$QT_PROBE_SDK_ENV"; fi
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -O1 -g -fPIC -pthread \
    "$root/endpoint_test.cpp" $(pkg-config --cflags --libs Qt6Core Qt6Network) -o "$out/endpoint-test"
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
    export RESIDENT_QEMU="$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm"
    export RESIDENT_QEMU_SYSROOT="$SDKTARGETSYSROOT"
    timeout --kill-after=1s 30s "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" "$out/endpoint-test" "$vectors"
else
    timeout --kill-after=1s 30s "$out/endpoint-test" "$vectors"
fi
