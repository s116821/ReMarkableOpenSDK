#!/bin/sh
# Owned QML/worker integration and separate compile-only startup; no device.
set -eu
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then . "$QT_PROBE_SDK_ENV"; moc="$OECORE_NATIVE_SYSROOT/usr/libexec/moc"; else moc=${QT_HOST_MOC:-/usr/lib/qt6/libexec/moc}; fi
for name in reader_selected_view_probe reader_worker_callback_witness reader_joined_creation_witness reader_worker_owner_quarantine; do
    "$moc" "tools/$name.h" -o "$work/moc_$name.cpp"
done
"$moc" tools/qt_qml_creation_bridge.h -o "$work/qt_qml_creation_bridge.moc"
for source in reader_selected_view_fixture reader_fixture_navigation_fixture reader_navigation_ownership_fixture reader_worker_order_entry_fixture; do
    "$moc" "tools/$source.cpp" -o "$work/$source.moc"
done
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -O2 -fPIC -pthread -I"$work" -Itools \
    tools/reader_worker_order_entry_fixture.cpp "$work/moc_reader_selected_view_probe.cpp" "$work/moc_reader_worker_callback_witness.cpp" \
    "$work/moc_reader_joined_creation_witness.cpp" "$work/moc_reader_worker_owner_quarantine.cpp" tools/qt_qml_creation_bridge.cpp \
    $(pkg-config --cflags --libs Qt6Core Qt6Gui Qt6Qml Qt6Quick) -ldl -o "$work/fixture"
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
    "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" -E LANG=C.UTF-8 -E QT_QPA_PLATFORM=offscreen -E QT_QUICK_BACKEND=software \
        -E QT_PLUGIN_PATH="$SDKTARGETSYSROOT/usr/lib/plugins" -E QML_IMPORT_PATH="$SDKTARGETSYSROOT/usr/lib/qml" "$work/fixture"
else
    QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software "$work/fixture"
fi
for variant in tuple cancel-running; do
if [ "$variant" = cancel-running ]; then scenario_define=-DREADER_WORKER_ORDER_CANCEL_RUNNING=1; else scenario_define=; fi
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -O2 -fPIC -pthread -shared -Wl,-z,defs -I"$work" -Itools $scenario_define \
 '-DREADER_WORKER_ORDER_NONCE="aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"' \
 '-DREADER_WORKER_ORDER_DOCUMENT_ID="10000000-0000-0000-0000-000000000001"' -DREADER_WORKER_ORDER_SOURCE_INDEX=1 -DREADER_WORKER_ORDER_PAGE_COUNT=3 \
 '-DREADER_WORKER_ORDER_LEFT_ID="20000000-0000-0000-0000-000000000001"' '-DREADER_WORKER_ORDER_SOURCE_ID="20000000-0000-0000-0000-000000000002"' '-DREADER_WORKER_ORDER_RIGHT_ID="20000000-0000-0000-0000-000000000003"' \
 tools/reader_worker_order_startup.cpp "$work/moc_reader_selected_view_probe.cpp" "$work/moc_reader_worker_callback_witness.cpp" \
 "$work/moc_reader_joined_creation_witness.cpp" "$work/moc_reader_worker_owner_quarantine.cpp" tools/qt_qml_creation_bridge.cpp \
 $(pkg-config --cflags --libs Qt6Core Qt6Gui Qt6Qml Qt6Quick) -ldl -o "$work/worker-order-$variant-owned.so"
done
printf 'PASS compile-only default and cancellation worker order startup; SOs never loaded\n'
