#!/bin/sh
set -eu
out=${RESIDENT_PIPELINE_OUTPUT:?fresh output directory required}
test ! -e "$out"
mkdir -p "$out"
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
    . "$QT_PROBE_SDK_ENV"
    moc="$OECORE_NATIVE_SYSROOT/usr/libexec/moc"
else
    moc=${QT_HOST_MOC:-/usr/lib/qt6/libexec/moc}
fi
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
for header in reader_request_retirement reader_insertion_guard_entry reader_selected_view_probe reader_worker_callback_witness reader_joined_creation_witness; do
    "$moc" "$root/$header.h" -o "$out/moc_$header.cpp"
done
"$moc" "$root/qt_qml_creation_bridge.h" -o "$out/qt_qml_creation_bridge.moc"
"$moc" "$root/reader_joined_creation_witness_fixture.cpp" -o "$out/reader_joined_creation_witness_fixture.moc"
"$moc" "$root/resident_mutation_pipeline_test.cpp" -o "$out/resident_mutation_pipeline_test.moc"
sanitize=
if [ "${RESIDENT_PIPELINE_SANITIZERS:-0}" = 1 ]; then sanitize='-fsanitize=address,undefined -fno-omit-frame-pointer -no-pie'; fi
${CXX:-c++} --version > "$out/compiler.txt"
pkg-config --modversion Qt6Core >> "$out/compiler.txt"
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -O1 -g -fPIC -pthread $sanitize \
    -DRESIDENT_MUTATION_TESTING -DREADER_INSERTION_GUARD_ENTRY_TESTING -DRESIDENT_WORKER_SLOT_TESTING \
    -I"$out" -I"$root" "$root/resident_mutation_pipeline_test.cpp" "$root/resident_mutation.cpp" \
    "$out"/moc_*.cpp "$root/qt_qml_creation_bridge.cpp" \
    $(pkg-config --cflags --libs Qt6Qml Qt6Gui Qt6Quick) -ldl -o "$out/fixture"
sha256sum "$root/resident_mutation"* "$root/reader_joined_creation_witness_fixture.cpp" "$root"/*.h "$out"/moc_*.cpp > "$out/inputs.sha256"
sha256sum "$out/fixture" > "$out/artifacts.sha256"
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
    timeout --kill-after=1s 60s "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" \
        -E LANG=C.UTF-8 -E QT_QPA_PLATFORM=offscreen -E QT_QUICK_BACKEND=software \
        -E QT_PLUGIN_PATH="$SDKTARGETSYSROOT/usr/lib/plugins" -E QML_IMPORT_PATH="$SDKTARGETSYSROOT/usr/lib/qml" "$out/fixture"
else
    ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
        QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software timeout --kill-after=1s 60s "$out/fixture"
fi
