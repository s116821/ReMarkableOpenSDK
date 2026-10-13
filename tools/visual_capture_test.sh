#!/bin/sh
# Existing vendor SDK only; owned ARM emulator, no tablet/dependency install.
set -eu
. "$QT_PROBE_SDK_ENV"
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
$CXX -std=c++17 -Wall -Wextra -Werror -O2 -fPIC tools/visual_capture_fixture.cpp \
 $(pkg-config --cflags --libs Qt6Qml Qt6Gui Qt6Quick) -o "$work/fixture"
$CXX -std=c++17 -Wall -Wextra -Werror -O2 -fPIC -shared -fvisibility=hidden -Wl,-z,defs \
 '-DVISUAL_CAPTURE_NONCE="aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"' tools/visual_capture_startup.cpp \
 $(pkg-config --cflags --libs Qt6Qml Qt6Gui Qt6Quick) -o "$work/test-only-startup.so"
if $CXX -std=c++17 -DVISUAL_CAPTURE_TESTING '-DVISUAL_CAPTURE_NONCE="aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"' \
 $(pkg-config --cflags Qt6Qml Qt6Gui Qt6Quick) -fsyntax-only tools/visual_capture_startup.cpp \
 > "$work/fence.stdout" 2> "$work/fence.stderr"; then exit 1; fi
grep -q 'Fixture-only clock/publication hooks must not enter' "$work/fence.stderr"
"$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" \
 -E LANG=C.UTF-8 -E QT_QPA_PLATFORM=offscreen -E QT_QUICK_BACKEND=software \
 -E QT_PLUGIN_PATH="$SDKTARGETSYSROOT/usr/lib/plugins" \
 -E QML_IMPORT_PATH="$SDKTARGETSYSROOT/usr/lib/qml" "$work/fixture"
"$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" \
 -E LANG=C.UTF-8 -E QT_QPA_PLATFORM=offscreen -E QT_QUICK_BACKEND=software \
 -E QT_PLUGIN_PATH="$SDKTARGETSYSROOT/usr/lib/plugins" \
 -E QML_IMPORT_PATH="$SDKTARGETSYSROOT/usr/lib/qml" \
 -E LD_PRELOAD="$work/test-only-startup.so" "$work/fixture" owned-startup
if [ "$#" -eq 1 ]; then
 cp "$work/test-only-startup.so" "$1"
 $READELF -h -d -V "$1" > "$1.elf.txt"
 $NM -D --demangle "$1" > "$1.symbols.txt"
 sha256sum "$1"
fi
