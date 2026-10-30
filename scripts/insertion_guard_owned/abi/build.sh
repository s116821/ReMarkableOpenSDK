#!/bin/sh
set -eu
. /opt/codex/rm2/5.8.203/environment-setup-cortexa7hf-neon-remarkable-linux-gnueabi
$CXX --version
$CXX -dM -E -x c++ /dev/null > /out/predefines.txt
pkg-config --modversion Qt6Core
"$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" --version
flags='-std=c++17 -Wall -Wextra -Werror -O1 -g -fPIC -pthread -marm -fexceptions -frtti -fno-lto -fno-optimize-sibling-calls -fno-omit-frame-pointer'
$CXX $flags -I/inputs $(pkg-config --cflags Qt6Core) -S -fverbose-asm \
  /inputs/compile_carriers.cpp -o /out/gcc-carriers.s
$CXX $flags -I/inputs $(pkg-config --cflags Qt6Core) -c \
  /inputs/compile_carriers.cpp -o /out/carriers.o
$OBJDUMP -dr /out/carriers.o > /out/carriers-assembly.txt
$CXX $flags -I/inputs -I/source/include /inputs/runtime.cpp /inputs/owned_entries.S \
  /library/libdobby.a $(pkg-config --cflags --libs Qt6Core) -ldl -o /out/abi-fixture
$READELF -h -A /out/abi-fixture > /out/elf.txt
$READELF --unwind /out/abi-fixture > /out/unwind.txt
$OBJDUMP -d --disassemble=owned_outer_entry /out/abi-fixture > /out/outer-entry.txt
$OBJDUMP -d --disassemble=owned_inner_entry /out/abi-fixture > /out/inner-entry.txt
$OBJDUMP -d --disassemble=owned_probe_registers /out/abi-fixture > /out/probe.txt
sha256sum /out/abi-fixture
"$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" -E LANG=C.UTF-8 /out/abi-fixture
