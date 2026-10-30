#!/bin/sh
# Owned QString diagnostic-state regression only; no application or device IO.
set -eu
mode=$1;src=$2;out=$3
mkdir -p "$out"
flags='-std=c++17 -O1 -g -Wall -Wextra -Werror -Wno-psabi -fPIC -fno-omit-frame-pointer'
case "$mode" in
 host) compiler=g++;;
 asan) compiler=g++;flags="$flags -fsanitize=address,undefined -no-pie";;
 arm) . /opt/codex/rm2/5.8.203/environment-setup-cortexa7hf-neon-remarkable-linux-gnueabi;compiler=$CXX;;
 *) exit 90;;
esac
sha256sum "$src"/reader_write_reopen_wait.h "$src"/reader_write_reopen_wait_fixture.cpp "$src"/reader_write_reopen_wait_test.sh > "$out/inputs.sha256"
$compiler $flags -I"$src" "$src/reader_write_reopen_wait_fixture.cpp" $(pkg-config --cflags --libs Qt6Core) -o "$out/fixture"
if test "$mode" = arm;then
 "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" "$out/fixture" > "$out/stdout" 2> "$out/stderr"
else
 ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 "$out/fixture" > "$out/stdout" 2> "$out/stderr"
fi
sha256sum -c "$out/inputs.sha256" > "$out/input-recheck.txt"

