#!/bin/sh
set -eu
ulimit -c 0
. /opt/codex/rm2/5.8.203/environment-setup-cortexa7hf-neon-remarkable-linux-gnueabi
command -v timeout > /out/timeout.txt
timeout --version >> /out/timeout.txt
$CXX --version > /out/compiler.txt
pkg-config --modversion Qt6Core >> /out/compiler.txt
sha256sum "$SDKTARGETSYSROOT/usr/lib/libQt6Core.so.6" "$SDKTARGETSYSROOT/usr/lib/libstdc++.so.6" "$SDKTARGETSYSROOT/usr/lib/libgcc_s.so.1" "$SDKTARGETSYSROOT/usr/lib/libm.so.6" "$SDKTARGETSYSROOT/usr/lib/libpthread.so.0" "$SDKTARGETSYSROOT/usr/lib/libc.so.6" "$SDKTARGETSYSROOT/lib/ld-linux-armhf.so.3" "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" > /out/providers.txt
$CXX -dM -E -x c++ /dev/null > /out/predefines.txt
flags='-std=c++17 -Wall -Wextra -Werror -O1 -g -fPIC -pthread -marm -fexceptions -frtti -fno-lto -fno-optimize-sibling-calls -fno-omit-frame-pointer'
test "$(sha256sum /library/libdobby.a | cut -d ' ' -f1)" = 622b17dc65d0dcad717d83264f783ad12044565ed8ea86f9e5088d9abf5328ef
$CXX $flags -I/inputs -I/dobbyinclude $(pkg-config --cflags Qt6Core) -c /inputs/core.cpp -o /out/core.o
$CXX $flags -c /inputs/startup.cpp -o /out/startup.o
# Deliberately inspect the result; textual link order is not credited as proof.
$CXX -shared /out/core.o /library/libdobby.a /out/startup.o $(pkg-config --libs Qt6Core) -ldl -pthread -Wl,-z,now -Wl,-z,relro -Wl,-Map=/out/payload.map -o /out/loader.so
$CXX $flags -I/inputs /inputs/host.cpp $(pkg-config --cflags --libs Qt6Core) -ldl -Wl,--export-dynamic -o /out/loader-host
$CXX $flags -I/inputs /inputs/host.cpp $(pkg-config --cflags --libs Qt6Core) -ldl -o /out/loader-host-noexports
for file in loader.so loader-host loader-host-noexports; do
 $READELF -W -h -A -d -r -s /out/$file > /out/$file.elf.txt
 $READELF -W -x .init_array /out/$file > /out/$file.init_array.txt
 $READELF --unwind /out/$file > /out/$file.unwind.txt
 $NM -n -C /out/$file > /out/$file.symbols.txt
done
$OBJDUMP -d -C /out/loader.so > /out/loader.assembly.txt
$OBJDUMP -d --disassemble=owned_loader_outer /out/loader-host > /out/host-outer.assembly.txt
$OBJDUMP -d --disassemble=owned_loader_inner /out/loader-host > /out/host-inner.assembly.txt
sha256sum /out/loader.so /out/loader-host /out/loader-host-noexports
timeout --kill-after=1s 10s "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" -E LANG=C.UTF-8 -E LD_DEBUG=libs -E LD_PRELOAD=/out/loader.so /out/loader-host > /out/positive.stdout 2> /out/positive.stderr
cat /out/positive.stdout
for mode in bad-entry missing-exports; do
 status=0
 if [ "$mode" = bad-entry ]; then
  timeout --kill-after=1s 10s "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" -E LANG=C.UTF-8 -E LD_PRELOAD=/out/loader.so -E OWNED_LOADER_FAIL=bad-entry /out/loader-host > /out/$mode.stdout 2> /out/$mode.stderr || status=$?
 else
  timeout --kill-after=1s 10s "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" -E LANG=C.UTF-8 -E LD_PRELOAD=/out/loader.so /out/loader-host-noexports > /out/$mode.stdout 2> /out/$mode.stderr || status=$?
 fi
 test "$status" = 119
 test ! -s /out/$mode.stdout
 ! grep HOST_MAIN /out/$mode.stderr
 grep LOADER_FAILSTOP /out/$mode.stderr
done
printf 'PASS owned pre-main DSO install and forwarding\n'
