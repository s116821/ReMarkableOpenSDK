#!/bin/sh
# Owned-file regression and ARM compile/profile only; no tablet transport/input.
set -eu
src=$1;out=$2;mode=${READER_FIXED_PEN_CONTACT_FAULT_MODE:-host}
case "$mode" in host|asan|arm) :;;*) exit 90;;esac
mkdir -p "$out"
export LANG=C.UTF-8
sha256sum "$src"/fixed_pen_contact_fault_writer.cpp "$src"/fixed_pen_contact_fault_fixture.cpp "$src"/fixed_pen_contact_fault_trace_replay.cpp "$src"/reader_fixed_pen_writer.cpp "$src"/reader_fixed_pen_holder_fixture.cpp "$src"/reader_fixed_pen_holder.h "$src"/reader_fixed_pen_holder_wire.h "$src"/reader_fixed_pen_spawn.h "$src"/reader_fixed_pen_stream.h "$src"/reader_fixed_stroke_policy.h > "$out/input-hashes.txt"
flags='-std=c++17 -O1 -g -Wall -Wextra -Werror -Wno-psabi -fPIC -pthread -fno-omit-frame-pointer'
if test "$mode" = arm;then
 . /opt/codex/rm2/5.8.203/environment-setup-cortexa7hf-neon-remarkable-linux-gnueabi
 compiler=$CXX
else compiler=g++;fi
if test "$mode" = asan;then flags="$flags -fsanitize=address,undefined -no-pie";fi
$compiler -std=c++17 -O2 -Wall -Wextra -Werror -Wno-psabi -DREADER_FIXED_PEN_WRITER_OWNED_TESTING -I"$src" "$src/fixed_pen_contact_fault_writer.cpp" -o "$out/owned-fault-writer"
$compiler $flags -I"$src" "$src/fixed_pen_contact_fault_fixture.cpp" $(pkg-config --cflags --libs Qt6Core) -o "$out/fixture"
$compiler -std=c++17 -O2 -Wall -Wextra -Werror -Wno-psabi -I"$src" "$src/fixed_pen_contact_fault_trace_replay.cpp" -o "$out/trace-replay"
if test "$mode" = arm;then
 $compiler -std=c++17 -O2 -Wall -Wextra -Werror -Wno-psabi -I"$src" "$src/fixed_pen_contact_fault_writer.cpp" -o "$out/native-fault-writer"
 $compiler -std=c++17 -O2 -Wall -Wextra -Werror -Wno-psabi -I"$src" "$src/reader_fixed_pen_writer.cpp" -o "$out/normal-writer"
 $READELF -h -A -d -l -r -Ws "$out/native-fault-writer" > "$out/native-fault-writer.elf.txt"
 "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" "$out/native-fault-writer" --profile > "$out/profile-qemu.json"
 printf 'ARM compiled/profiled only; native exec and device IO are separate. QEMU thread count is not native qualification.\n' > "$out/stdout"
 : > "$out/stderr"
else
 "$out/owned-fault-writer" --profile > "$out/profile.json"
 READER_HOLDER_EXPORT="$out/wire" ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 "$out/fixture" "$out/owned-fault-writer" > "$out/stdout" 2> "$out/stderr"
fi
sha256sum -c "$out/input-hashes.txt" > "$out/input-recheck.txt"
