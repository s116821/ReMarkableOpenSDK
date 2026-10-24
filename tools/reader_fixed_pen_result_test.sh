#!/bin/sh
# Closed consumer and original producer; process and device evidence modeled.
set -eu
src=${1:?frozen source folder}; out=${2:?fresh output folder}
mkdir -p "$out"
mode=${READER_FIXED_PEN_RESULT_MODE:-host}
flags='-std=c++17 -Wall -Wextra -Werror -Wno-psabi -O1 -g -fPIC -fno-omit-frame-pointer'
case "$mode" in
 host) compiler=g++;;
 asan) compiler=g++;flags="$flags -fsanitize=address,undefined";;
 arm) . /opt/codex/rm2/5.8.203/environment-setup-cortexa7hf-neon-remarkable-linux-gnueabi;compiler=$CXX;;
 *) exit 90;;
esac
$compiler --version > "$out/compiler.txt"
pkg-config --modversion Qt6Core >> "$out/compiler.txt"
$compiler $flags -I"$src" "$src/reader_fixed_pen_result_fixture.cpp" $(pkg-config --cflags --libs Qt6Core) -o "$out/fixture"
if [ "$mode" = arm ]; then
 $READELF -h -A "$out/fixture" > "$out/elf.txt"
 timeout --kill-after=1s 60s "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" "$out/fixture"
else
 ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 timeout --kill-after=1s 60s "$out/fixture"
fi
sha256sum "$out/fixture" > "$out/artifact.sha256"
