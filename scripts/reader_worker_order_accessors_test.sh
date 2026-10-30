#!/bin/sh
# Owned typed function-pointer models; no proprietary target calls.
set -eu
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then . "$QT_PROBE_SDK_ENV"; fi
printf '%s\n' '#include "reader_worker_order_accessors.h"' > "$work/production-header.cpp"
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -O2 -fPIC ${READER_ORDER_SANITIZE_FLAGS:-} -Iscripts \
    $(pkg-config --cflags Qt6Core) -c "$work/production-header.cpp" -o "$work/production-header.o"
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -O2 -fPIC ${READER_ORDER_SANITIZE_FLAGS:-} -Iscripts \
    scripts/reader_worker_order_accessors_fixture.cpp $(pkg-config --cflags --libs Qt6Core) -ldl -pthread -o "$work/fixture"
if [ -n "${QT_PROBE_BINARY_OUT:-}" ]; then cp "$work/fixture" "$QT_PROBE_BINARY_OUT"; fi
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
    "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" -E LANG=C.UTF-8 "$work/fixture"
else
    "$work/fixture"
fi
