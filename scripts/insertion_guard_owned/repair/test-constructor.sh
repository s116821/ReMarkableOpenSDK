#!/bin/sh
set -eu
. /opt/codex/rm2/5.8.203/environment-setup-cortexa7hf-neon-remarkable-linux-gnueabi
flags='-std=c++17 -Wall -Wextra -Werror -O1 -g -fPIC -pthread -marm -fexceptions -frtti'
includes='-I/source -I/source/source -I/source/source/dobby -I/source/include -I/source/external -I/source/external/logging'
$CXX $flags $includes -include /inputs/compat-gcc-armv7.h -DEXPECT_REPAIRED=0 \
 /inputs/constructor.cpp /oldlib/libdobby.a -ldl -o /out/constructor-old
$CXX $flags $includes -include /inputs/compat-gcc-armv7.h -DEXPECT_REPAIRED=1 \
 /inputs/constructor.cpp /newlib/libdobby.a -ldl -o /out/constructor-fixed
sha256sum /out/constructor-old /out/constructor-fixed
"$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" /out/constructor-old
"$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" /out/constructor-fixed
