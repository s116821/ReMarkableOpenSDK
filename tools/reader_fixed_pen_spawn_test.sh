#!/bin/sh
set -eu
src=$1;out=$2;mode=${READER_FIXED_PEN_SPAWN_MODE:-host}
case "$mode" in host|arm|asan) :;;*) exit 90;;esac
mkdir -p "$out"
export LANG=C.UTF-8
sha256sum "$src"/reader_fixed_stroke_policy.h "$src"/reader_fixed_pen_stream.h "$src"/reader_fixed_pen_holder.h "$src"/reader_fixed_pen_holder_wire.h "$src"/reader_fixed_pen_holder.cpp "$src"/reader_fixed_pen_holder_fixture.cpp "$src"/reader_fixed_pen_spawn.h "$src"/reader_fixed_pen_writer.cpp "$src"/reader_fixed_pen_spawn_fixture.cpp "$src"/reader_fixed_pen_spawn_test.sh > "$out/input-hashes.txt"
# Same plain fixed-writer source; explicit host model substitutes a FIFO for input.
g++ -std=c++17 -O1 -g -Wall -Wextra -Werror -DREADER_FIXED_PEN_WRITER_OWNED_TESTING -I"$src" "$src/reader_fixed_pen_writer.cpp" -o "$out/owned-host-writer"
"$out/owned-host-writer" --profile > "$out/owned-writer-profile.json"
flags='-std=c++17 -O1 -g -Wall -Wextra -Werror -Wno-psabi -fPIC -pthread -fno-omit-frame-pointer'
if test "$mode" = arm;then
 . /opt/codex/rm2/5.8.203/environment-setup-cortexa7hf-neon-remarkable-linux-gnueabi
 compiler=$CXX
else compiler=g++;fi
if test "$mode" = asan;then flags="$flags -fsanitize=address,undefined -no-pie";fi
$compiler $flags -I"$src" "$src/reader_fixed_pen_spawn_fixture.cpp" $(pkg-config --cflags --libs Qt6Core) -o "$out/fixture"
if test "$mode" = arm;then
 # QEMU user mode does not qualify exec of a fresh ARM child. The prior mixed
 # ARM-parent/HOST-worker experiment was inconsistent and remains failed evidence.
 # Compile the ARM fixture for a real ARM kernel; do not label a cross-image
 # workaround as an executed ARM writer composition.
 printf 'ARM fixture compiled; fresh-child execution requires native ARM kernel.\n' > "$out/stdout"
 : > "$out/stderr"
 $compiler -std=c++17 -O2 -Wall -Wextra -Werror -Wno-psabi -I"$src" "$src/reader_fixed_pen_writer.cpp" -o "$out/production-writer"
 $compiler -std=c++17 -O2 -Wall -Wextra -Werror -Wno-psabi -DREADER_FIXED_PEN_WRITER_OWNED_TESTING -I"$src" "$src/reader_fixed_pen_writer.cpp" -o "$out/owned-arm-writer"
 $READELF -h -A -d -Ws "$out/production-writer" > "$out/production-writer-elf.txt"
 "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" "$out/production-writer" --profile > "$out/production-writer-profile-qemu.json"
 $compiler $flags -I"$src" "$src/reader_fixed_pen_holder.cpp" $(pkg-config --cflags --libs Qt6Core) -o "$out/production-holder"
 $READELF -h -A -d -Ws "$out/production-holder" > "$out/production-holder-elf.txt"
else
 READER_HOLDER_EXPORT="$out/wire" ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 "$out/fixture" "$out/owned-host-writer" > "$out/stdout" 2> "$out/stderr"
fi
sha256sum -c "$out/input-hashes.txt" > "$out/input-recheck.txt"
