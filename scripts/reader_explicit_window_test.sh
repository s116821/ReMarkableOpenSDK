#!/bin/sh
# Only new explicit-window owned controls. No startup artifact or device access.
set -eu
. "$QT_PROBE_SDK_ENV"
if [ -n "${READER_EXPLICIT_EVIDENCE_DIR:-}" ]; then
 work="$READER_EXPLICIT_EVIDENCE_DIR/work"
 mkdir "$work"
else
 work=$(mktemp -d)
 trap 'rm -rf "$work"' EXIT
fi
$CXX --version > "$work/compiler-version.txt"
pkg-config --cflags --libs Qt6Qml Qt6Gui Qt6Quick > "$work/qt-flags.txt"
"$OECORE_NATIVE_SYSROOT/usr/libexec/moc" scripts/reader_explicit_window_fixture.cpp -o "$work/reader_explicit_window_fixture.moc" \
 > "$work/moc.stdout" 2> "$work/moc.stderr"
$CXX -std=c++17 -Wall -Wextra -Werror -O2 -fPIC -I"$work" \
 scripts/reader_explicit_window_fixture.cpp $(pkg-config --cflags --libs Qt6Qml Qt6Gui Qt6Quick) -o "$work/fixture" \
 > "$work/compile.stdout" 2> "$work/compile.stderr"
$READELF -h -d -V "$work/fixture" > "$work/fixture.elf.txt"
sha256sum "$work/fixture" > "$work/fixture.sha256.txt"
"$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" \
 -E LANG=C.UTF-8 -E QT_QPA_PLATFORM=offscreen -E QT_QUICK_BACKEND=software \
 -E QT_PLUGIN_PATH="$SDKTARGETSYSROOT/usr/lib/plugins" \
 -E QML_IMPORT_PATH="$SDKTARGETSYSROOT/usr/lib/qml" "$work/fixture" "$@" \
 > "$work/control.stdout" 2> "$work/control.stderr"
cat "$work/control.stdout"
