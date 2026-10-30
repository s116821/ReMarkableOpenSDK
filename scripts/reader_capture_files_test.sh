#!/bin/sh
# Actual local owned files/FDs; no transport, target application or device.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
temporary=
out=${READER_CAPTURE_FILES_OUTPUT:-}
if [ -z "$out" ]; then out=$(mktemp -d); temporary=$out; fi
trap 'if [ -n "$temporary" ]; then rm -rf "$temporary"; fi' EXIT
mkdir -p "$out"
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then . "$QT_PROBE_SDK_ENV"; fi
flags='-std=c++17 -Wall -Wextra -Werror -O1 -g -fPIC -pthread'
if [ "${READER_CAPTURE_FILES_SANITIZERS:-0}" = 1 ]; then flags="$flags -fsanitize=address,undefined -fno-omit-frame-pointer"; fi
${CXX:-c++} --version > "$out/compiler.txt"
pkg-config --modversion Qt6Core >> "$out/compiler.txt"
printf '#include "reader_capture_files.h"\nstatic_assert(reader_capture_files::Files::ImageCap==1048576);\n' > "$out/header.cpp"
${CXX:-c++} $flags -I"$root" $(pkg-config --cflags Qt6Core) -c "$out/header.cpp" -o "$out/header.o"
${CXX:-c++} $flags -I"$root" "$root/reader_capture_files_fixture.cpp" $(pkg-config --cflags --libs Qt6Core) -o "$out/fixture"
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
 "$READELF" -h -A "$out/fixture" > "$out/elf.txt"
 timeout --kill-after=1s 60s "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" -E LANG=C.UTF-8 "$out/fixture"
else
 ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 timeout --kill-after=1s 60s "$out/fixture"
fi
sha256sum "$out/fixture" "$out/header.o" > "$out/artifact-hashes.txt"
