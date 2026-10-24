#!/bin/sh
# New cleanup controls plus unchanged original capture-file regression on same header.
set -eu
src=${1:?frozen source folder};out=${2:?fresh output folder}
mode=${READER_CAPTURE_RETIRE_MODE:-host}
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
sha256sum "$src"/reader_capture_files.h "$src"/reader_capture_files_fixture.cpp "$src"/reader_capture_files_test.sh "$src"/reader_capture_retire_fixture.cpp "$src"/reader_capture_retire_test.sh > "$out/input-hashes.txt"
mkdir "$out/legacy"
if [ "$mode" = arm ];then
 READER_CAPTURE_FILES_OUTPUT="$out/legacy" QT_PROBE_SDK_ENV=/opt/codex/rm2/5.8.203/environment-setup-cortexa7hf-neon-remarkable-linux-gnueabi /bin/sh "$src/reader_capture_files_test.sh"
elif [ "$mode" = asan ];then
 READER_CAPTURE_FILES_OUTPUT="$out/legacy" READER_CAPTURE_FILES_SANITIZERS=1 /bin/sh "$src/reader_capture_files_test.sh"
else
 READER_CAPTURE_FILES_OUTPUT="$out/legacy" /bin/sh "$src/reader_capture_files_test.sh"
fi
$compiler $flags -I"$src" "$src/reader_capture_retire_fixture.cpp" $(pkg-config --cflags --libs Qt6Core) -o "$out/fixture"
if [ "$mode" = arm ];then
 $READELF -h -A "$out/fixture" > "$out/elf.txt"
 timeout --kill-after=1s 30s "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" "$out/fixture"
else
 ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 timeout --kill-after=1s 30s "$out/fixture"
fi
sha256sum "$out/fixture" > "$out/artifact.sha256"
sha256sum -c "$out/input-hashes.txt" > "$out/input-recheck.txt"
