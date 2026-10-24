#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
work=${ACTIVATION_TEST_OUTPUT:?fresh output directory required}
mkdir -p "$work"
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then . "$QT_PROBE_SDK_ENV"; fi
sanitizers=
if [ "${ACTIVATION_TEST_SANITIZERS:-0}" = 1 ]; then sanitizers='-fsanitize=address,undefined -fno-omit-frame-pointer -no-pie'; fi
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -O1 -g -fPIC $sanitizers \
    "$root/activation_test.cpp" $(pkg-config --cflags --libs Qt6Core Qt6Network) -o "$work/activation-test"
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
    timeout --kill-after=1s 20s "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" -E LANG=C.UTF-8 "$work/activation-test"
else
    timeout --kill-after=1s 20s "$work/activation-test"
fi
