#!/bin/sh
# Actual owned selected-view Sample + fixed composition. No device/native backend.
set -eu
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then . "$QT_PROBE_SDK_ENV"; moc="$OECORE_NATIVE_SYSROOT/usr/libexec/moc"; else moc=${QT_HOST_MOC:-/usr/lib/qt6/libexec/moc}; fi
for name in reader_selected_view_probe reader_worker_callback_witness reader_joined_creation_witness reader_request_retirement; do
    "$moc" "scripts/$name.h" -o "$work/moc_$name.cpp"
done
"$moc" scripts/qt_qml_creation_bridge.h -o "$work/qt_qml_creation_bridge.moc"
for source in reader_selected_view_fixture reader_fixture_navigation_fixture reader_navigation_ownership_fixture; do
    "$moc" "scripts/$source.cpp" -o "$work/$source.moc"
done
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -O2 -fPIC -pthread -I"$work" -Iscripts \
    scripts/reader_resident_join_pair_fixture.cpp "$work/moc_reader_selected_view_probe.cpp" "$work/moc_reader_worker_callback_witness.cpp" \
    "$work/moc_reader_joined_creation_witness.cpp" "$work/moc_reader_request_retirement.cpp" scripts/qt_qml_creation_bridge.cpp \
    $(pkg-config --cflags --libs Qt6Core Qt6Gui Qt6Qml Qt6Quick) -ldl -o "$work/fixture"
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
    "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" -E LANG=C.UTF-8 -E QT_QPA_PLATFORM=offscreen -E QT_QUICK_BACKEND=software \
        -E QT_PLUGIN_PATH="$SDKTARGETSYSROOT/usr/lib/plugins" -E QML_IMPORT_PATH="$SDKTARGETSYSROOT/usr/lib/qml" "$work/fixture"
else
    QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software "$work/fixture"
fi
# Compile the separate startup with original owned UUIDs, never load its SO.
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -O2 -fPIC -pthread -shared -Wl,-z,defs -I"$work" -Iscripts \
 '-DREADER_RESIDENT_ACTIVATION_NONCE="aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"' '-DREADER_RESIDENT_FIRST_NONCE="bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"' '-DREADER_RESIDENT_SECOND_NONCE="cccccccccccccccccccccccccccccccc"' \
 '-DREADER_RESIDENT_DOCUMENT_ID="10000000-0000-0000-0000-000000000001"' '-DREADER_RESIDENT_PAGE_ID="20000000-0000-0000-0000-000000000001"' -DREADER_RESIDENT_PAGE_INDEX=0 -DREADER_RESIDENT_PAGE_COUNT=2 \
 scripts/reader_resident_join_startup.cpp "$work/moc_reader_selected_view_probe.cpp" "$work/moc_reader_worker_callback_witness.cpp" \
 "$work/moc_reader_joined_creation_witness.cpp" "$work/moc_reader_request_retirement.cpp" scripts/qt_qml_creation_bridge.cpp \
 $(pkg-config --cflags --libs Qt6Core Qt6Gui Qt6Qml Qt6Quick) -ldl -o "$work/pair-owned.so"
printf 'PASS compile-only resident startup; SO never loaded\n'
