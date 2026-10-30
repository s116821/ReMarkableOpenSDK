#!/bin/sh
# Owned opaque-address/QString models only; no native function invocation.
set -eu
ulimit -c 0
out=${INSERTION_CONDITION_OUTPUT:?set INSERTION_CONDITION_OUTPUT}
mkdir -p "$out"
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then . "$QT_PROBE_SDK_ENV"; fi
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
${CXX:-c++} --version > "$out/compiler.txt"
pkg-config --modversion Qt6Core >> "$out/compiler.txt"
sanitize=
if [ "${INSERTION_CONDITION_SANITIZERS:-0}" = 1 ]; then sanitize='-fsanitize=address,undefined -fno-omit-frame-pointer'; fi
printf '#include "reader_insertion_condition.h"\n' > "$out/header.cpp"
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -O1 -g -fPIC -I"$root" $(pkg-config --cflags Qt6Core) -c "$out/header.cpp" -o "$out/header.o"
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -O1 -g -fPIC $sanitize -I"$root" "$root/reader_insertion_condition_fixture.cpp" $(pkg-config --cflags --libs Qt6Core) -pthread -o "$out/fixture"
sha256sum "$out/fixture" "$out/header.o" > "$out/artifact-hashes.txt"
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
    "$READELF" -W -h -A "$out/fixture" > "$out/elf.txt"
    timeout --kill-after=1s 10s "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" -E LANG=C.UTF-8 "$out/fixture"
else
    ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 timeout --kill-after=1s 10s "$out/fixture"
fi
