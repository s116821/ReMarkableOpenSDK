#!/bin/sh
set -eu
ulimit -c 0
out=${INSERTION_MUTEX_OUTPUT:?set INSERTION_MUTEX_OUTPUT}
mkdir -p "$out"
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then . "$QT_PROBE_SDK_ENV"; fi
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
${CXX:-c++} --version > "$out/compiler.txt"
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -O1 -g -pthread -I"$root" "$root/reader_insertion_mutex_probe_fixture.cpp" -o "$out/fixture"
sha256sum "$out/fixture" > "$out/artifact-hashes.txt"
run() {
    if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
        timeout --kill-after=1s 10s "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" "$out/fixture" "$1"
    else timeout --kill-after=1s 10s "$out/fixture" "$1"; fi
}
for mode in valid invalid; do
    run "$mode" > "$out/$mode.stdout" 2> "$out/$mode.stderr"
    cat "$out/$mode.stdout"
done
for mode in busy reuse unlock-failure try-throws unlock-throws; do
    status=0;run "$mode" > "$out/$mode.stdout" 2> "$out/$mode.stderr" || status=$?
    test "$status" = 119
    test ! -s "$out/$mode.stdout" && test ! -s "$out/$mode.stderr"
    printf 'PASS direct fail-stop mode=%s exit119\n' "$mode"
done
