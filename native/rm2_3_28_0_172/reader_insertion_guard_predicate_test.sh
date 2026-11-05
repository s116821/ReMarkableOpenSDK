#!/bin/sh
# Original owned address/QString models only; no private/native function calls.
set -eu
work=${INSERTION_PREDICATE_OUTPUT:?fresh output directory required}
mkdir -p "$work"
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then . "$QT_PROBE_SDK_ENV"; fi
printf '%s\n' '#include "reader_insertion_guard_predicate.h"' > "$work/header.cpp"
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -O2 -fPIC -I"$root" \
    $(pkg-config --cflags Qt6Core) -c "$work/header.cpp" -o "$work/header.o"
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -O2 -fPIC -I"$root" \
    "$root/reader_insertion_guard_predicate_fixture.cpp" $(pkg-config --cflags --libs Qt6Core) -pthread -o "$work/fixture"
if [ -n "${QT_PROBE_BINARY_OUT:-}" ]; then cp "$work/fixture" "$QT_PROBE_BINARY_OUT"; fi
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
    timeout --kill-after=1s 10s "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" -E LANG=C.UTF-8 "$work/fixture"
else
    timeout --kill-after=1s 10s "$work/fixture"
fi
