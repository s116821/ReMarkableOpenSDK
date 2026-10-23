#!/bin/sh
set -eu
ulimit -c 0
# Standalone ARM owned test. Caller supplies pinned SDK env, Dobby headers/archive,
# and writable output directory. Does not fetch/install or contact a device.
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then . "$QT_PROBE_SDK_ENV"; fi
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
out=${INSERTION_HOOKS_OUTPUT:?set INSERTION_HOOKS_OUTPUT}
: "${DOBBY_INCLUDE:?set DOBBY_INCLUDE}"
: "${DOBBY_LIBRARY:?set DOBBY_LIBRARY}"
mkdir -p "$out"
command -v timeout > "$out/timeout.txt"
timeout --version >> "$out/timeout.txt"
test "$(sha256sum "$DOBBY_LIBRARY" | cut -d ' ' -f1)" = 622b17dc65d0dcad717d83264f783ad12044565ed8ea86f9e5088d9abf5328ef
$CXX --version > "$out/compiler.txt"
pkg-config --modversion Qt6Core >> "$out/compiler.txt"
$CXX -std=c++17 -Wall -Wextra -Werror -O1 -g -fPIC -pthread -marm -fexceptions -frtti -fno-lto -fno-optimize-sibling-calls -fno-omit-frame-pointer \
 -I"$root" -I"$DOBBY_INCLUDE" "$root/reader_insertion_guard_hooks_fixture.cpp" \
 "$DOBBY_LIBRARY" $(pkg-config --cflags --libs Qt6Core) -ldl -o "$out/hooks-fixture"
$READELF -h -A "$out/hooks-fixture" > "$out/elf.txt"
$READELF --unwind "$out/hooks-fixture" > "$out/unwind.txt"
$OBJDUMP -d -C "$out/hooks-fixture" > "$out/assembly.txt"
sha256sum "$out/hooks-fixture"
for name in mismatch foreign binding cancel deadline cancel-inner cancel-predicate transformed-index transformed-target duplicate nested unrelated-outer unrelated-inner job-valid job-false job-zero job-cancel; do
 timeout --kill-after=1s 10s "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" "$out/hooks-fixture" "$name" > "$out/$name.stdout" 2> "$out/$name.stderr"
 cat "$out/$name.stdout"
done
for name in bad-original misaligned bad-protection second-install-fails null-original early-entry bad-entry bad-trampoline wrong-callsite duplicate-inner throw-installer throw-failstop overlap second-corrupts-entry second-corrupts-trampoline; do
 status=0
 timeout --kill-after=1s 10s "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" "$out/hooks-fixture" "$name" > "$out/$name.stdout" 2> "$out/$name.stderr" || status=$?
 test "$status" = 119
 test ! -s "$out/$name.stdout"
 grep 'FAILSTOP ' "$out/$name.stderr"
 case "$name" in
  wrong-callsite|duplicate-inner) grep 'submissions=1 outer=1 inner=0' "$out/$name.stderr" ;;
  *) grep 'submissions=0 outer=0 inner=0' "$out/$name.stderr" ;;
 esac
 case "$name" in
  overlap) grep 'hooks=0' "$out/$name.stderr" ;;
  second-corrupts-*) grep 'CORRUPTED ' "$out/$name.stderr"; grep 'hooks=2' "$out/$name.stderr" ;;
 esac
done
printf 'PASS owned hook core: normal and fail-stop cases completed\n'
