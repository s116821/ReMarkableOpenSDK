#!/bin/sh
# Real owned selected-page QML + capture core; separately labeled timing models.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
out=${READER_IDENTITY_CAPTURE_OUTPUT:?fresh output directory required}
test ! -e "$out" && test ! -L "$out"
mkdir -p "$out"
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
 . "$QT_PROBE_SDK_ENV"
 moc="$OECORE_NATIVE_SYSROOT/usr/libexec/moc"
else
 moc=${QT_HOST_MOC:-/usr/lib/qt6/libexec/moc}
fi
flags='-std=c++17 -Wall -Wextra -Werror -O1 -g -fPIC -pthread'
if [ "${READER_IDENTITY_CAPTURE_SANITIZERS:-0}" = 1 ]; then flags="$flags -fsanitize=address,undefined -fno-omit-frame-pointer"; fi
${CXX:-c++} --version > "$out/compiler.txt"
pkg-config --modversion Qt6Core >> "$out/compiler.txt"
"$moc" "$root/reader_selected_view_probe.h" -o "$out/moc_reader_selected_view_probe.cpp"
"$moc" "$root/reader_selected_view_fixture.cpp" -o "$out/reader_selected_view_fixture.moc"
printf '#include "reader_identity_capture.h"\nstatic_assert(!reader_identity_capture::Result::nativeAuthority);\n' > "$out/header.cpp"
${CXX:-c++} $flags -I"$root" -I"$out" $(pkg-config --cflags Qt6Core Qt6Gui Qt6Qml Qt6Quick) -c "$out/header.cpp" -o "$out/header.o"
${CXX:-c++} $flags -I"$root" -I"$out" "$root/reader_identity_capture_fixture.cpp" "$out/moc_reader_selected_view_probe.cpp" \
 $(pkg-config --cflags --libs Qt6Core Qt6Gui Qt6Qml Qt6Quick) -o "$out/fixture"
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
 "$READELF" -h -A "$out/fixture" > "$out/elf.txt"
 timeout --kill-after=1s 100s "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" \
  -E LANG=C.UTF-8 -E QT_QPA_PLATFORM=offscreen -E QT_QUICK_BACKEND=software \
  -E QT_PLUGIN_PATH="$SDKTARGETSYSROOT/usr/lib/plugins" "$out/fixture"
else
 QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
  timeout --kill-after=1s 100s "$out/fixture"
fi
sha256sum "$out/fixture" "$out/header.o" > "$out/artifact-hashes.txt"
