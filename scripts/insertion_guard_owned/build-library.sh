#!/bin/sh
set -eu
. /opt/codex/rm2/5.8.203/environment-setup-cortexa7hf-neon-remarkable-linux-gnueabi
cmake --version
$CXX --version
pkg-config --modversion Qt6Core
"$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" --version
$CXX -dM -E -x c++ /dev/null > /out/target-predefines.txt
cmake -S /inputs -B /out/build \
  -DCMAKE_TOOLCHAIN_FILE="$OECORE_NATIVE_SYSROOT/usr/share/cmake/OEToolchainConfig.cmake" \
  -DCMAKE_BUILD_TYPE=Release -DDOBBY_DEBUG=OFF -DPlugin.SymbolResolver=OFF \
  -DDOBBY_BUILD_EXAMPLE=OFF -DDOBBY_BUILD_TEST=OFF -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build /out/build --target dobby_static -j4
$READELF -h -A /out/build/upstream/libdobby.a > /out/library-elf.txt
sha256sum /out/build/upstream/libdobby.a
