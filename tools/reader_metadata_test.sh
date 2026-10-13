#!/bin/sh
# Existing vendor ARM SDK only. No dependency installation or device access.
set -eu
. "$QT_PROBE_SDK_ENV"
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
"$OECORE_NATIVE_SYSROOT/usr/libexec/moc" tools/reader_metadata_fixture.cpp -o "$work/reader_metadata_fixture.moc"
$CXX -std=c++17 -Wall -Wextra -Werror -O2 -fPIC -I"$work" \
  tools/reader_metadata_fixture.cpp $(pkg-config --cflags --libs Qt6Qml Qt6Gui Qt6Quick) -o "$work/fixture"
# Explicitly test-only startup artifact. Never use this identity on a tablet.
$CXX -std=c++17 -Wall -Wextra -Werror -O2 -fPIC -shared -fvisibility=hidden -Wl,-z,defs \
  '-DREADER_CENSUS_NONCE="aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"' tools/reader_metadata_startup.cpp \
  $(pkg-config --cflags --libs Qt6Qml Qt6Gui Qt6Quick) -o "$work/test-only-startup.so"
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
  $NM -D "$1" > "$1.symbols.txt"
  sha256sum "$1"
fi
