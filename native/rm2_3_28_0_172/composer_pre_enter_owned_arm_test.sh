#!/bin/sh
# Owned ARM ELF route only. Does not load, contact or modify Xochitl.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
work=${COMPOSER_PRE_ENTER_OUTPUT:?fresh output directory required}
test ! -e "$work"
mkdir -p "$work"
. "${QT_PROBE_SDK_ENV:?matching SDK environment required}"
version=$(pkg-config --modversion Qt6Gui)
includes=$(pkg-config --variable=includedir Qt6Gui)
if [ ! -d "$includes/QtGui/$version" ]; then includes="$SDKTARGETSYSROOT$includes"; fi
private="-I$includes/QtCore/$version -I$includes/QtCore/$version/QtCore -I$includes/QtGui/$version -I$includes/QtGui/$version/QtGui"
flags="-std=c++17 -Wall -Wextra -Werror -O1 -g -marm -fexceptions -funwind-tables -fPIC -pthread -DCOMPOSER_TEXT_PRIVATE_QT"
# Macro-free production translation unit must also compile.
${CXX} $flags $private $(pkg-config --cflags Qt6Core Qt6Gui) \
    -c "$root/composer_pre_enter.cpp" -o "$work/production.o"
${CXX} $flags -UCOMPOSER_TEXT_PRIVATE_QT $(pkg-config --cflags Qt6Core Qt6Gui) \
    -c "$root/composer_pre_enter.cpp" -o "$work/production-ordinary.o"
${CXX} $flags -DCOMPOSER_PRE_ENTER_TESTING $private \
    "$root/composer_pre_enter.cpp" $(pkg-config --cflags --libs Qt6Core Qt6Gui) \
    -shared -Wl,-soname,libpre-enter-owned.so -Wl,-z,nodelete -ldl -o "$work/libpre-enter-owned.so"
${CXX} $flags -DCOMPOSER_PRE_ENTER_TESTING $private \
    "$root/composer_pre_enter_owned_arm.cpp" "$root/composer_pre_enter_owned_arm.S" \
    $(pkg-config --cflags --libs Qt6Core Qt6Gui) -no-pie \
    -Wl,--section-start=.stock_original=0xb492a0 \
    -Wl,--section-start=.stock_call=0xb4a9ac \
    -Wl,--section-start=.stock_veneer=0xb4a000 \
    -L"$work" -lpre-enter-owned -Wl,-rpath,'$ORIGIN' -ldl -o "$work/owned-arm"
${OBJDUMP} -d "$work/owned-arm" > "$work/caller-disassembly.txt"
${OBJDUMP} -d "$work/libpre-enter-owned.so" > "$work/shim-disassembly.txt"
${READELF} --unwind "$work/owned-arm" > "$work/caller-unwind.txt"
${READELF} --unwind "$work/libpre-enter-owned.so" > "$work/shim-unwind.txt"
${READELF} --dynamic "$work/libpre-enter-owned.so" > "$work/shim-dynamic.txt"
${NM} -D "$work/libpre-enter-owned.so" > "$work/shim-symbols.txt"
grep -q NODELETE "$work/shim-dynamic.txt"
if grep -E 'RouteD[012]|__cxa_guard_acquire|__aeabi_atexit' "$work/shim-symbols.txt"; then
    echo 'Unexpected lazy initialization or exit destruction in resident shim' >&2
    exit 1
fi
timeout --kill-after=1s 40s "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" \
    -L "$SDKTARGETSYSROOT" -E QT_QPA_PLATFORM=offscreen \
    -E QT_PLUGIN_PATH="$SDKTARGETSYSROOT/usr/lib/plugins" "$work/owned-arm"
