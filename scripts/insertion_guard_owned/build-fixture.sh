#!/bin/sh
set -eu
. /opt/codex/rm2/5.8.203/environment-setup-cortexa7hf-neon-remarkable-linux-gnueabi
"$OECORE_NATIVE_SYSROOT/usr/libexec/moc" /inputs/owned_fixture.cpp -o /out/owned_fixture.moc
$CXX -std=c++17 -Wall -Wextra -Werror -O1 -g -fPIC -pthread -marm -fexceptions -frtti \
  -fno-lto -fno-optimize-sibling-calls -fno-omit-frame-pointer \
  -I/out -I/source/include /inputs/owned_fixture.cpp /library/libdobby.a \
  $(pkg-config --cflags --libs Qt6Core) -ldl -o /out/owned-fixture
$READELF -h -A /out/owned-fixture > /out/fixture-elf.txt
$READELF --unwind /out/owned-fixture > /out/fixture-unwind.txt
$OBJDUMP -d --disassemble=owned_handler /out/owned-fixture > /out/handler-assembly.txt
$OBJDUMP -d --disassemble=owned_insert /out/owned-fixture > /out/insert-assembly.txt
sha256sum /out/owned-fixture
"$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" \
  -E LANG=C.UTF-8 /out/owned-fixture
