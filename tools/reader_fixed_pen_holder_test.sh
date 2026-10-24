#!/bin/sh
# Owned FIFO/process composition only. Production ARM CLI compiles/refuses args.
set -eu
src=${1:?frozen source folder};out=${2:?fresh output folder}
mode=${READER_FIXED_PEN_HOLDER_MODE:-host}
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
sha256sum "$src"/reader_fixed_stroke_policy.h "$src"/reader_fixed_pen_stream.h "$src"/reader_fixed_pen_holder.h "$src"/reader_fixed_pen_holder_wire.h "$src"/reader_fixed_pen_holder.cpp "$src"/reader_fixed_pen_holder_fixture.cpp "$src"/reader_fixed_pen_holder_test.sh > "$out/input-hashes.txt"
cat > "$out/reverse-include.cpp" <<'CPP'
#include <QJsonDocument>
#include "reader_fixed_pen_holder.h"
#include "reader_fixed_pen_holder_wire.h"
#ifndef emit
#error caller Qt emit macro lost
#endif
static_assert(!reader_fixed_pen_stream::Report::nativeAuthority);
CPP
$compiler $flags -I"$src" $(pkg-config --cflags Qt6Core) -c "$out/reverse-include.cpp" -o "$out/reverse-include.o"
$compiler $flags -I"$src" "$src/reader_fixed_pen_holder_fixture.cpp" $(pkg-config --cflags --libs Qt6Core) -o "$out/fixture"
if [ "$mode" = arm ];then
 $READELF -h -A "$out/fixture" > "$out/elf.txt"
 timeout --kill-after=1s 45s "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" -E READER_HOLDER_EXPORT="$out/wire" "$out/fixture"
 $compiler $flags -I"$src" "$src/reader_fixed_pen_holder.cpp" $(pkg-config --cflags --libs Qt6Core) -o "$out/production-holder"
 $READELF -h -A -d "$out/production-holder" > "$out/production-elf.txt"
 $READELF -Ws "$out/production-holder" > "$out/production-symbols.txt"
 set +e
 timeout --kill-after=1s 10s "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" "$out/production-holder" > "$out/production-refusal.stdout" 2> "$out/production-refusal.stderr"
 status=$?
 set -e
 test "$status" = 2
 test ! -s "$out/production-refusal.stdout"
else
 READER_HOLDER_EXPORT="$out/wire" ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 timeout --kill-after=1s 45s "$out/fixture"
fi
sha256sum "$out/fixture" > "$out/artifact.sha256"
sha256sum -c "$out/input-hashes.txt" > "$out/input-recheck.txt"
