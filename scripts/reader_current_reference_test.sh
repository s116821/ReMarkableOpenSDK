#!/bin/sh
set -eu
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
  . "$QT_PROBE_SDK_ENV"
fi
compiler=${CXX:-c++}
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
$compiler -std=c++17 -Wall -Wextra -Werror -O2 -fPIC \
  scripts/reader_current_reference_fixture.cpp $(pkg-config --cflags --libs Qt6Qml Qt6Gui Qt6Quick) -o "$work/fixture"
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
  "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" \
    -E LANG=C.UTF-8 -E QT_QPA_PLATFORM=offscreen -E QT_QUICK_BACKEND=software \
    -E QT_PLUGIN_PATH="$SDKTARGETSYSROOT/usr/lib/plugins" \
    -E QML_IMPORT_PATH="$SDKTARGETSYSROOT/usr/lib/qml" "$work/fixture"
else
  QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software "$work/fixture"
fi
