#!/bin/sh
# Actual owned file Entry; explicit modeled native/Core/identity boundaries.
set -eu
out=${RESIDENT_MUTATION_OUTPUT:?set RESIDENT_MUTATION_OUTPUT to a fresh directory}
mkdir -p "$out"
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
 . "$QT_PROBE_SDK_ENV"
 moc="$OECORE_NATIVE_SYSROOT/usr/libexec/moc"
else
 moc=${QT_HOST_MOC:-/usr/lib/qt6/libexec/moc}
fi
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
flags='-std=c++17 -Wall -Wextra -Werror -O1 -g -fPIC -pthread'
if [ "${RESIDENT_MUTATION_SANITIZERS:-0}" = 1 ]; then flags="$flags -fsanitize=address,undefined -fno-omit-frame-pointer"; fi
sha256sum "$root/reader_resident_mutation_transport_fixture.cpp" "$root/reader_resident_mutation_transport_test.sh" \
 "$root/reader_insertion_guard_transport.h" "$root/reader_insertion_guard_entry_fixture.cpp" \
 "$root/reader_request_scope.h" "$root/reader_retirement_scope.h" "$root/reader_request_retirement.h" > "$out/direct-input-hashes.txt"
for header in reader_request_retirement reader_insertion_guard_entry reader_selected_view_probe reader_worker_callback_witness reader_joined_creation_witness; do
 "$moc" "$root/$header.h" -o "$out/moc_$header.cpp"
done
"$moc" "$root/qt_qml_creation_bridge.h" -o "$out/qt_qml_creation_bridge.moc"
"$moc" "$root/reader_joined_creation_witness_fixture.cpp" -o "$out/reader_joined_creation_witness_fixture.moc"
"$moc" "$root/reader_insertion_guard_entry_fixture.cpp" -o "$out/reader_insertion_guard_entry_fixture.moc"
${CXX:-c++} --version > "$out/compiler.txt"
pkg-config --modversion Qt6Core Qt6Gui Qt6Quick Qt6Qml >> "$out/compiler.txt"
${CXX:-c++} $flags -I"$out" -I"$root" "$root/reader_resident_mutation_transport_fixture.cpp" \
 "$out/moc_reader_request_retirement.cpp" "$out/moc_reader_insertion_guard_entry.cpp" "$out/moc_reader_selected_view_probe.cpp" \
 "$out/moc_reader_worker_callback_witness.cpp" "$out/moc_reader_joined_creation_witness.cpp" "$root/qt_qml_creation_bridge.cpp" \
 $(pkg-config --cflags --libs Qt6Qml Qt6Gui Qt6Quick) -ldl -o "$out/fixture"
printf '#include "reader_insertion_guard_transport.h"\n' > "$out/production.cpp"
${CXX:-c++} $flags -I"$root" $(pkg-config --cflags Qt6Qml Qt6Gui Qt6Quick) -c "$out/production.cpp" -o "$out/production.o"
sha256sum "$out/fixture" "$out/production.o" > "$out/artifact-hashes.txt"
mkdir -p "$out/wire"
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
 timeout --kill-after=1s 60s "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" \
  -E LANG=C.UTF-8 -E QT_QPA_PLATFORM=offscreen -E QT_QUICK_BACKEND=software \
  -E QT_PLUGIN_PATH="$SDKTARGETSYSROOT/usr/lib/plugins" -E QML_IMPORT_PATH="$SDKTARGETSYSROOT/usr/lib/qml" \
  -E RESIDENT_MUTATION_WIRE_OUTPUT="$out/wire" "$out/fixture"
else
 ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
  QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software RESIDENT_MUTATION_WIRE_OUTPUT="$out/wire" timeout --kill-after=1s 60s "$out/fixture"
fi
sha256sum -c "$out/direct-input-hashes.txt" > "$out/input-recheck.txt"
