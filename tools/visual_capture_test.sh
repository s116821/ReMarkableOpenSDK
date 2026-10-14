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
$CXX -std=c++17 -Wall -Wextra -Werror -O2 -fPIC -shared -fvisibility=hidden -Wl,-z,defs \
 '-DVISUAL_CAPTURE_NONCE="aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"' -DVISUAL_CAPTURE_PASSIVE_FRAME_READY=1 \
 tools/visual_capture_startup.cpp $(pkg-config --cflags --libs Qt6Qml Qt6Gui Qt6Quick) -o "$work/test-only-passive.so"
if $CXX -std=c++17 -DVISUAL_CAPTURE_TESTING '-DVISUAL_CAPTURE_NONCE="aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"' \
 $(pkg-config --cflags Qt6Qml Qt6Gui Qt6Quick) -fsyntax-only tools/visual_capture_startup.cpp \
 > "$work/fence.stdout" 2> "$work/fence.stderr"; then exit 1; fi
grep -q 'Fixture-only clock/publication hooks must not enter' "$work/fence.stderr"
if $CXX -std=c++17 -DVISUAL_CAPTURE_PASSIVE_FRAME_READY=0 '-DVISUAL_CAPTURE_NONCE="aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"' \
 $(pkg-config --cflags Qt6Qml Qt6Gui Qt6Quick) -fsyntax-only tools/visual_capture_startup.cpp \
 > "$work/mode.stdout" 2> "$work/mode.stderr"; then exit 1; fi
grep -q 'Passive frame readiness must be explicitly enabled' "$work/mode.stderr"
if $CXX -std=c++17 -DVISUAL_CAPTURE_BINDING_WITNESS=1 '-DVISUAL_CAPTURE_NONCE="aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"' \
 $(pkg-config --cflags Qt6Qml Qt6Gui Qt6Quick) -fsyntax-only tools/visual_capture_startup.cpp \
 > "$work/witness.stdout" 2> "$work/witness.stderr"; then exit 1; fi
grep -q 'Binding witness requires explicit value 1' "$work/witness.stderr"
"$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" \
 -E LANG=C.UTF-8 -E QT_QPA_PLATFORM=offscreen -E QT_QUICK_BACKEND=software \
 -E QT_PLUGIN_PATH="$SDKTARGETSYSROOT/usr/lib/plugins" \
 -E QML_IMPORT_PATH="$SDKTARGETSYSROOT/usr/lib/qml" "$work/fixture"
"$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" \
 -E LANG=C.UTF-8 -E QT_QPA_PLATFORM=offscreen -E QT_QUICK_BACKEND=software \
 -E QT_PLUGIN_PATH="$SDKTARGETSYSROOT/usr/lib/plugins" \
 -E QML_IMPORT_PATH="$SDKTARGETSYSROOT/usr/lib/qml" "$work/fixture" frame-interleaving-only
"$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" \
 -E LANG=C.UTF-8 -E QT_QPA_PLATFORM=offscreen -E QT_QUICK_BACKEND=software \
 -E QT_PLUGIN_PATH="$SDKTARGETSYSROOT/usr/lib/plugins" \
 -E QML_IMPORT_PATH="$SDKTARGETSYSROOT/usr/lib/qml" "$work/fixture" binding-witness-only
"$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" \
 -E LANG=C.UTF-8 -E QT_QPA_PLATFORM=offscreen -E QT_QUICK_BACKEND=software \
 -E QT_PLUGIN_PATH="$SDKTARGETSYSROOT/usr/lib/plugins" \
 -E QML_IMPORT_PATH="$SDKTARGETSYSROOT/usr/lib/qml" \
 -E LD_PRELOAD="$work/test-only-startup.so" "$work/fixture" owned-startup
"$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" \
 -E LANG=C.UTF-8 -E QT_QPA_PLATFORM=offscreen -E QT_QUICK_BACKEND=software \
 -E QT_PLUGIN_PATH="$SDKTARGETSYSROOT/usr/lib/plugins" \
 -E QML_IMPORT_PATH="$SDKTARGETSYSROOT/usr/lib/qml" \
 -E LD_PRELOAD="$work/test-only-passive.so" "$work/fixture" owned-frame-startup
if [ "$#" -eq 1 ]; then
 cp "$work/test-only-startup.so" "$1"
 cp "$work/test-only-passive.so" "$1.passive.so"
 $READELF -h -d -V "$1" > "$1.elf.txt"
 $NM -D --demangle "$1" > "$1.symbols.txt"
 $READELF -h -d -V "$1.passive.so" > "$1.passive.so.elf.txt"
 $NM -D --demangle "$1.passive.so" > "$1.passive.so.symbols.txt"
 sha256sum "$1"
 sha256sum "$1.passive.so"
fi
