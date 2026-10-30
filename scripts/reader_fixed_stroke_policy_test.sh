#!/bin/sh
# Plain owned models + real pipe/child lifetime controls. Never input-device IO.
set -eu
src=${1:?frozen source folder}
out=${2:?fresh output folder}
mode=${READER_FIXED_STROKE_MODE:-host}
mkdir -p "$out"
flags='-std=c++17 -Wall -Wextra -Werror -O1 -g -fno-omit-frame-pointer -pthread'
case "$mode" in
 host) compiler=g++;;
 asan) compiler=g++;flags="$flags -fsanitize=address,undefined";;
 arm) . /opt/codex/rm2/5.8.203/environment-setup-cortexa7hf-neon-remarkable-linux-gnueabi;compiler=$CXX;;
 *) exit 90;;
esac
$compiler --version > "$out/compiler.txt"
sha256sum "$src/reader_fixed_stroke_policy.h" "$src/reader_fixed_stroke_policy_fixture.cpp" "$src/reader_fixed_stroke_policy_test.sh" > "$out/input-hashes.txt"
$compiler $flags "$src/reader_fixed_stroke_policy_fixture.cpp" -o "$out/fixture"
if [ "$mode" = arm ]; then
 $READELF -h -A "$out/fixture" > "$out/elf.txt"
 timeout --kill-after=1s 30s "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" "$out/fixture"
else
 ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 timeout --kill-after=1s 30s "$out/fixture"
fi
sha256sum "$out/fixture" > "$out/artifact.sha256"
sha256sum -c "$out/input-hashes.txt" > "$out/input-recheck.txt"
