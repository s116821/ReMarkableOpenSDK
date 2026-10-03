#!/bin/sh
set -eu
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then . "$QT_PROBE_SDK_ENV"; fi
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then moc="$OECORE_NATIVE_SYSROOT/usr/libexec/moc";
else moc="$(pkg-config --variable=libexecdir Qt6Core)/moc"; fi
for file in qt_page_facts qt_page_facts_entry; do "$moc" "tools/$file.h" -o "$work/$file.moc"; done
"$moc" tools/qt_page_facts_fixture.cpp -o "$work/qt_page_facts_fixture.moc"
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -O2 -fPIC -I"$work" \
 tools/qt_page_facts_entry_fixture.cpp tools/qt_page_facts_entry.cpp tools/qt_page_facts.cpp \
 $(pkg-config --cflags --libs Qt6Qml Qt6Gui Qt6Quick) -o "$work/fixture"
cat > "$work/owned-facts-entry-config.h" <<'CONFIG'
inline qml_access::FactsEntryConfig pageFactsEntryConfig() {
    qml_access::FactsEntryConfig config;
    config.nonce=QStringLiteral("0123456789abcdef0123456789abcdef");
    config.directory=QStringLiteral("/run/rmb-qt-probe-")+config.nonce;
    config.facts.documentId=QStringLiteral("00000000-0000-4000-8000-000000000001");
    config.facts.pageCap=6; config.facts.budgetMs=5000;
    for (int i=2;i<=7;++i) config.facts.expectedOrder.append(QStringLiteral("00000000-0000-4000-8000-%1").arg(i,12,16,QLatin1Char('0')));
    return config;
}
CONFIG
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -fPIC -fsyntax-only -I"$work" \
 '-DQT_FACTS_ENTRY_CONFIG="owned-facts-entry-config.h"' tools/qt_page_facts_startup.cpp \
 $(pkg-config --cflags Qt6Qml Qt6Gui Qt6Quick)
for case in good myfiles no-owner-then-ready wrong-document wrong-token oversize symlink-token fifo-token token-mode directory-mode owner-mode stale setup-deadline delayed-dispatch nested-cancel nested-deadline restoring-before restoring-getter directory-replaced late-delivery duplicate-after release; do
 if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
  /opt/codex/rm2/5.8.203/sysroots/x86_64-codexsdk-linux/usr/bin/qemu-arm \
   -L "$SDKTARGETSYSROOT" -E LANG=C.UTF-8 -E QT_QPA_PLATFORM=offscreen \
   -E QT_PLUGIN_PATH="$SDKTARGETSYSROOT/usr/lib/plugins" \
   -E QML_IMPORT_PATH="$SDKTARGETSYSROOT/usr/lib/qml" "$work/fixture" "$case"
 else QT_QPA_PLATFORM=offscreen "$work/fixture" "$case"; fi
done
if [ -n "${FACTS_BUDDY_SOURCE:-}" ]; then
 ${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -O2 -fPIC -I"$work" \
  -DOWNED_FACTS_PUBLISHER -I"$FACTS_BUDDY_SOURCE/tools/native_page_facts_probe" \
  tools/qt_page_facts_entry_fixture.cpp tools/qt_page_facts_entry.cpp tools/qt_page_facts.cpp \
  $(pkg-config --cflags --libs Qt6Qml Qt6Gui Qt6Quick) -o "$work/integration"
 root="$work/rmb-qt-probe-0123456789abcdef0123456789abcdef"
 mkdir -m 700 "$root"
 printf 'OWNED FIXTURE DATA; not an ELF or executable extension.\n' > "$root/payload.so"
 chmod 600 "$root/payload.so"
 if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
  FACTS_OWNED_ROOT="$root" LD_PRELOAD="$root/payload.so" /opt/codex/rm2/5.8.203/sysroots/x86_64-codexsdk-linux/usr/bin/qemu-arm \
   -L "$SDKTARGETSYSROOT" -E LANG=C.UTF-8 -E QT_QPA_PLATFORM=offscreen \
   -E QT_PLUGIN_PATH="$SDKTARGETSYSROOT/usr/lib/plugins" \
   -E QML_IMPORT_PATH="$SDKTARGETSYSROOT/usr/lib/qml" "$work/integration" integration
 else FACTS_OWNED_ROOT="$root" LD_PRELOAD="$root/payload.so" QT_QPA_PLATFORM=offscreen "$work/integration" integration; fi
fi
