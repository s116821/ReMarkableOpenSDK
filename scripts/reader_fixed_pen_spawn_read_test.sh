#!/bin/sh
# Consume frozen original bytes only; never launches a writer.
set -eu
src=${1:?frozen tools};assets=${2:?owned original assets};out=${3:?fresh output}
mode=${READER_FIXED_PEN_SPAWN_READ_MODE:-host}
mkdir -p "$out"
flags='-std=c++17 -O1 -g -Wall -Wextra -Werror -fPIC -pthread -fno-omit-frame-pointer'
case "$mode" in
 host) compiler=g++;;
 asan) compiler=g++;flags="$flags -fsanitize=address,undefined -no-pie";;
 arm) . /opt/codex/rm2/5.8.203/environment-setup-cortexa7hf-neon-remarkable-linux-gnueabi;compiler=$CXX;;
 *) exit 90;;
esac
sha256sum "$src"/reader_fixed_stroke_policy.h "$src"/reader_fixed_pen_stream.h "$src"/reader_fixed_pen_result.h "$src"/reader_fixed_pen_holder_read_fixture.cpp "$src"/reader_fixed_pen_spawn_read_fixture.cpp "$src"/reader_fixed_pen_spawn_read_test.sh > "$out/input-hashes.txt"
(cd "$assets";sha256sum -c asset-hashes.txt) > "$out/assets-before.txt"
$compiler $flags -Wno-psabi -I"$src" "$src/reader_fixed_pen_spawn_read_fixture.cpp" $(pkg-config --cflags --libs Qt6Core) -o "$out/fixture"
if test "$mode" = arm;then
 "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" "$out/fixture" "$assets" > "$out/stdout" 2> "$out/stderr"
else
 ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 "$out/fixture" "$assets" > "$out/stdout" 2> "$out/stderr"
fi
(cd "$assets";sha256sum -c asset-hashes.txt) > "$out/assets-after.txt"
sha256sum -c "$out/input-hashes.txt" > "$out/input-recheck.txt"

