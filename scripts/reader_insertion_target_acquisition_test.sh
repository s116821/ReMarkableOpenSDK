#!/bin/sh
# Original Qt method/one-shot models only. No native implementation or device.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
temporary=
out=${INSERTION_TARGET_ACQUISITION_OUTPUT:-}
if [ -z "$out" ]; then out=$(mktemp -d); temporary=$out; fi
trap 'if [ -n "$temporary" ]; then rm -rf "$temporary"; fi' EXIT
mkdir -p "$out"
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
 . "$QT_PROBE_SDK_ENV"
 moc="$OECORE_NATIVE_SYSROOT/usr/libexec/moc"
else
 moc=${QT_HOST_MOC:-/usr/lib/qt6/libexec/moc}
fi
flags='-std=c++17 -Wall -Wextra -Werror -O1 -g -fPIC -pthread'
if [ "${TARGET_ACQUISITION_SANITIZERS:-0}" = 1 ]; then flags="$flags -fsanitize=address,undefined -fno-omit-frame-pointer"; fi
${CXX:-c++} --version > "$out/compiler.txt"
pkg-config --modversion Qt6Core >> "$out/compiler.txt"
for name in reader_selected_view_probe reader_joined_creation_witness; do
 "$moc" "$root/$name.h" -o "$out/moc_$name.cpp"
done
"$moc" "$root/qt_qml_creation_bridge.h" -o "$out/qt_qml_creation_bridge.moc"
"$moc" "$root/reader_insertion_target_acquisition_fixture.cpp" -o "$out/reader_insertion_target_acquisition_fixture.moc"
printf '#include "reader_insertion_target_acquisition.h"\nstatic_assert(!reader_insertion_target_acquisition::Result::nativeAuthority);\n' > "$out/header.cpp"
${CXX:-c++} $flags -I"$root" -I"$out" $(pkg-config --cflags Qt6Core Qt6Gui Qt6Qml Qt6Quick) -c "$out/header.cpp" -o "$out/header.o"
${CXX:-c++} $flags -I"$root" -I"$out" "$root/reader_insertion_target_acquisition_fixture.cpp" "$root/qt_qml_creation_bridge.cpp" \
 "$out/moc_reader_selected_view_probe.cpp" "$out/moc_reader_joined_creation_witness.cpp" \
 $(pkg-config --cflags --libs Qt6Core Qt6Gui Qt6Qml Qt6Quick) -o "$out/fixture"
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
 "$READELF" -h -A "$out/fixture" > "$out/elf.txt"
 timeout --kill-after=1s 45s "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" \
  -E LANG=C.UTF-8 -E QT_QPA_PLATFORM=offscreen -E QT_PLUGIN_PATH="$SDKTARGETSYSROOT/usr/lib/plugins" "$out/fixture"
else
 QT_QPA_PLATFORM=offscreen ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 timeout --kill-after=1s 45s "$out/fixture"
fi
sha256sum "$out/fixture" "$out/header.o" > "$out/artifact-hashes.txt"
