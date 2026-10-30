#!/bin/sh
# Offline composition: never runs the producer/native writer or changes asset bytes.
set -eu
src=${1:?frozen source folder};assets=${2:?frozen original asset folder};out=${3:?fresh output folder}
mode=${READER_FIXED_PEN_HOLDER_READ_MODE:-host}
mkdir -p "$out"
flags='-std=c++17 -O1 -g -Wall -Wextra -Werror -fPIC -pthread -fno-omit-frame-pointer'
case "$mode" in
 host) compiler=g++;;
 asan) compiler=g++;flags="$flags -fsanitize=address,undefined";;
 arm) . /opt/codex/rm2/5.8.203/environment-setup-cortexa7hf-neon-remarkable-linux-gnueabi;compiler=$CXX;;
 *) exit 90;;
esac
$compiler --version > "$out/compiler.txt"
pkg-config --modversion Qt6Core >> "$out/compiler.txt"
sha256sum "$src"/reader_fixed_stroke_policy.h "$src"/reader_fixed_pen_stream.h "$src"/reader_fixed_pen_result.h "$src"/reader_fixed_pen_holder_read_fixture.cpp "$src"/reader_fixed_pen_holder_read_test.sh > "$out/input-hashes.txt"
(cd "$assets";sha256sum -c asset-hashes.txt) > "$out/asset-before.txt"
$compiler $flags -I"$src" "$src/reader_fixed_pen_holder_read_fixture.cpp" $(pkg-config --cflags --libs Qt6Core) -o "$out/fixture"
if [ "$mode" = arm ];then
 $READELF -h -A "$out/fixture" > "$out/elf.txt"
 timeout --kill-after=1s 20s "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" "$out/fixture" "$assets"
else
 ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 timeout --kill-after=1s 20s "$out/fixture" "$assets"
fi
(cd "$assets";sha256sum -c asset-hashes.txt) > "$out/asset-after.txt"
sha256sum "$out/fixture" > "$out/artifact.sha256"
sha256sum -c "$out/input-hashes.txt" > "$out/input-recheck.txt"
