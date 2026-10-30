#!/bin/sh
# Owned pipes and host signal delivery only; ARM-QEMU is not native kernel proof.
set -eu
mode=${1:?mode};src=${2:?source};out=${3:?fresh output}
mkdir -p "$out"
flags='-std=c++17 -Wall -Wextra -Werror -Wno-psabi -O1 -g -fPIC -fno-omit-frame-pointer'
case "$mode" in
 host) compiler=c++;;
 asan) compiler=c++;flags="$flags -fsanitize=address,undefined";;
 arm) . /opt/codex/rm2/5.8.203/environment-setup-cortexa7hf-neon-remarkable-linux-gnueabi;compiler=$CXX;;
 *) exit 90;;
esac
$compiler $flags -I"$src" "$src/reader_capture_frame_write_fixture.cpp" $(pkg-config --cflags --libs Qt6Core) -o "$out/fixture"
if test "$mode" = arm;then
 timeout --kill-after=1s 30s "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" "$out/fixture" --skip-signal-test > "$out/stdout" 2> "$out/stderr"
else
 ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 timeout --kill-after=1s 30s "$out/fixture" > "$out/stdout" 2> "$out/stderr"
fi
test ! -s "$out/stderr"

