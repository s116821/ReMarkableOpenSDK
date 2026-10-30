#!/bin/sh
set -eu
ulimit -c 0
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then . "$QT_PROBE_SDK_ENV"; fi
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
out=${INSERTION_JOB_OUTPUT:?set INSERTION_JOB_OUTPUT}
mkdir -p "$out"
command -v timeout > "$out/timeout.txt"
timeout --version >> "$out/timeout.txt"
$CXX --version > "$out/compiler.txt"
$CXX -dM -E -x c++ /dev/null > "$out/predefines.txt"
$CXX -std=c++17 -Wall -Wextra -Werror -O1 -g -fPIC -pthread -marm -fexceptions -frtti -fno-lto \
 "$root/reader_insertion_job_snapshot_fixture.cpp" -Wl,-z,now -o "$out/job-fixture"
$READELF -h -A -d -s "$out/job-fixture" > "$out/elf.txt"
grep BIND_NOW "$out/elf.txt"
$OBJDUMP -d -C "$out/job-fixture" > "$out/assembly.txt"
sha256sum "$SDKTARGETSYSROOT/usr/lib/libc.so.6" "$SDKTARGETSYSROOT/usr/lib/libpthread.so.0" > "$out/providers.txt"
sha256sum "$out/job-fixture"
for mode in success busy zero range; do
 timeout --kill-after=1s 10s "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" "$out/job-fixture" "$mode" > "$out/$mode.stdout" 2> "$out/$mode.stderr"
 cat "$out/$mode.stdout"
done
for mode in try-error try-throw owner-dead throw-after-acquire unlock-error unlock-throw throw-after-unlock fatal-return fatal-throw; do
 status=0
 timeout --kill-after=1s 10s "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" "$out/job-fixture" "$mode" > "$out/$mode.stdout" 2> "$out/$mode.stderr" || status=$?
 test "$status" = 119
 test ! -s "$out/$mode.stdout"
 grep "OWNED_FAILURE_CONTROL $mode before snapshot" "$out/$mode.stderr"
 ! grep FORBIDDEN_FATAL_REPORTER "$out/$mode.stderr"
done
printf 'PASS one-shot job snapshot owned controls\n'
