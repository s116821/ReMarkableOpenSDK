#!/bin/sh
set -eu
set -x # Exact compiler and MOC arguments retained in the private test log.
# Pinned SDK container, /src read-only, /out writable, ephemeral /run and /tmp.
# Synthetic all-zero compile-only artifact is NEVER a selected native payload.
. /opt/codex/rm2/5.8.203/environment-setup-cortexa7hf-neon-remarkable-linux-gnueabi
cd /src
mkdir -p /out/pretoken-moc /out/ordinary-moc
moc="$OECORE_NATIVE_SYSROOT/usr/libexec/moc"
for name in qt_page_facts qt_page_facts_entry; do
 "$moc" -DQT_FACTS_PRETOKEN_DIAGNOSTIC=1 "tools/$name.h" -o "/out/pretoken-moc/$name.moc"
 "$moc" "tools/$name.h" -o "/out/ordinary-moc/$name.moc"
done
"$moc" tools/qt_page_facts_fixture.cpp -o /out/pretoken-moc/qt_page_facts_fixture.moc
"$moc" tools/qt_page_facts_fixture.cpp -o /out/ordinary-moc/qt_page_facts_fixture.moc
set -- $CXX -std=c++17 -Wall -Wextra -Werror -O2 -fPIC -shared -Wl,-z,defs \
 -DQT_FACTS_PRETOKEN_DIAGNOSTIC=1 '-DQT_SHUTDOWN_NONCE="00000000000000000000000000000000"' \
 -I/out/pretoken-moc tools/qt_shutdown_trace_startup.cpp tools/qt_page_facts_entry.cpp tools/qt_page_facts.cpp \
 $(pkg-config --cflags --libs Qt6Quick Qt6Gui Qt6Core Qt6Qml) -o /out/pretoken-compile-only.so
printf '%s\0' "$@" > /out/pretoken-compile-argv.nul
"$@"
$CXX -std=c++17 -Wall -Wextra -Werror -O2 -fPIC -DQT_FACTS_PRETOKEN_DIAGNOSTIC=1 -I/out/pretoken-moc \
 tools/qt_pretoken_shutdown_fixture.cpp tools/qt_page_facts_entry.cpp tools/qt_page_facts.cpp \
 $(pkg-config --cflags --libs Qt6Quick Qt6Gui Qt6Core Qt6Qml) -o /out/pretoken-owned-fixture
$CXX -std=c++17 -Wall -Wextra -Werror -O2 -fPIC -I/out/ordinary-moc \
 tools/qt_page_facts_entry_fixture.cpp tools/qt_page_facts_entry.cpp tools/qt_page_facts.cpp \
 $(pkg-config --cflags --libs Qt6Quick Qt6Gui Qt6Core Qt6Qml) -o /out/pretoken-ordinary-control
run() {
 "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" \
  -E QT_QPA_PLATFORM=offscreen -E QT_QUICK_BACKEND=software \
  -E QT_PLUGIN_PATH="$SDKTARGETSYSROOT/usr/lib/plugins" \
  -E QML_IMPORT_PATH="$SDKTARGETSYSROOT/usr/lib/qml" "$@"
}
for mode in source-valid capture-valid malformed normal-fallback input-fallback nonzero-frame identity-refused config-refused stale-refused; do
 run /out/pretoken-owned-fixture "$mode"
done
run /out/pretoken-ordinary-control good
$READELF -h /out/pretoken-compile-only.so > /out/pretoken-elf-header.txt
$READELF -d /out/pretoken-compile-only.so > /out/pretoken-elf-dynamic.txt
$NM -C /out/pretoken-compile-only.so > /out/pretoken-symbols.txt
sha256sum /out/pretoken-compile-only.so /out/pretoken-owned-fixture /out/pretoken-ordinary-control
