#!/bin/sh
# Owned optional-scope and real Qt/JS bridge controls; no device/native backend.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
out=${READER_REQUEST_SCOPE_OUTPUT:?fresh output directory required}
test ! -e "$out"
mkdir -p "$out"
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
 . "$QT_PROBE_SDK_ENV"
 moc="$OECORE_NATIVE_SYSROOT/usr/libexec/moc"
else
 moc=${QT_HOST_MOC:-/usr/lib/qt6/libexec/moc}
fi
flags='-std=c++17 -Wall -Wextra -Werror -O1 -g -fPIC -pthread'
if [ "${READER_REQUEST_SCOPE_SANITIZERS:-0}" = 1 ]; then flags="$flags -fsanitize=address,undefined -fno-omit-frame-pointer"; fi
${CXX:-c++} --version > "$out/compiler.txt"
pkg-config --modversion Qt6Core Qt6Gui Qt6Qml >> "$out/compiler.txt"
sha256sum "$root/reader_request_scope.h" "$root/reader_retirement_scope.h" \
 "$root/reader_request_retirement.h" "$root/qt_qml_creation_bridge.h" \
 "$root/qt_qml_creation_bridge.cpp" "$root/reader_request_scope_fixture.cpp" \
 "$root/reader_request_scope_test.sh" > "$out/input-hashes.txt"
printf '#include "reader_request_scope.h"\n#include <type_traits>\nstatic_assert(!std::is_copy_constructible<reader_request_scope::Scope>::value);\nstatic_assert(!std::is_move_constructible<reader_request_scope::Scope>::value);\n' > "$out/no-qt-header.cpp"
${CXX:-c++} $flags -I"$root" -c "$out/no-qt-header.cpp" -o "$out/no-qt-header.o"
"$moc" "$root/reader_request_retirement.h" -o "$out/moc_reader_request_retirement.cpp"
"$moc" "$root/qt_qml_creation_bridge.h" -o "$out/qt_qml_creation_bridge.moc"
${CXX:-c++} $flags -I"$root" -I"$out" "$root/reader_request_scope_fixture.cpp" \
 "$root/qt_qml_creation_bridge.cpp" "$out/moc_reader_request_retirement.cpp" \
 $(pkg-config --cflags --libs Qt6Core Qt6Gui Qt6Qml) -o "$out/fixture"
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
 "$READELF" -h -A "$out/fixture" > "$out/elf.txt"
 timeout --kill-after=1s 60s "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" \
  -E LANG=C.UTF-8 -E QT_QPA_PLATFORM=offscreen \
  -E QT_PLUGIN_PATH="$SDKTARGETSYSROOT/usr/lib/plugins" "$out/fixture"
else
 QT_QPA_PLATFORM=offscreen ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
  timeout --kill-after=1s 60s "$out/fixture"
fi
sha256sum "$out/fixture" "$out/no-qt-header.o" > "$out/artifact-hashes.txt"
sha256sum -c "$out/input-hashes.txt" > "$out/input-recheck.txt"
