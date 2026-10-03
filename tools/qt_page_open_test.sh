#!/bin/sh
set -eu
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then . "$QT_PROBE_SDK_ENV"; fi
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then moc="$OECORE_NATIVE_SYSROOT/usr/libexec/moc";
else moc="$(pkg-config --variable=libexecdir Qt6Core)/moc"; fi
"$moc" tools/qt_page_open.h -o "$work/qt_page_open.moc"
"$moc" tools/qt_page_open_fixture.cpp -o "$work/qt_page_open_fixture.moc"
"$moc" tools/qt_qml_creation_bridge.h -o "$work/qt_qml_creation_bridge.moc"
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -O2 -fPIC -I"$work" \
  tools/qt_page_open_fixture.cpp tools/qt_page_open.cpp tools/qt_qml_creation_bridge.cpp \
  $(pkg-config --cflags --libs Qt6Qml Qt6Gui Qt6Quick) -o "$work/fixture"
cat > "$work/page_open_config.h" <<'CONFIG'
inline qml_access::PageOpenConfig pageOpenConfig() {
  return {QStringLiteral("00000000-0000-4000-8000-000000000001"),
    QStringLiteral("00000000-0000-4000-8000-000000000002"), QStringLiteral("00000000-0000-4000-8000-000000000003"),
    {QStringLiteral("00000000-0000-4000-8000-000000000002"), QStringLiteral("00000000-0000-4000-8000-000000000003"),
     QStringLiteral("00000000-0000-4000-8000-000000000004"), QStringLiteral("00000000-0000-4000-8000-000000000005"),
     QStringLiteral("00000000-0000-4000-8000-000000000006"), QStringLiteral("00000000-0000-4000-8000-000000000007")}, true};
}
CONFIG
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -O2 -fPIC -I"$work" -shared -fvisibility=hidden -Wl,-z,defs \
  '-DQT_PROBE_NONCE="0123456789abcdef0123456789abcdef"' '-DQT_PROBE_PAGE_OPEN_CONFIG="page_open_config.h"' \
  tools/qt_qml_access_probe.cpp tools/qt_qml_creation_bridge.cpp tools/qt_page_open.cpp \
  $(pkg-config --cflags --libs Qt6Qml Qt6Gui Qt6Quick) -o "$work/open-probe.so"
for case in good already hidden disabled alias mapping missing duplicate config stale-source node-bound depth-bound cancel timeout wrong-page destroy cancel-queued cancel-after remap-queued page-change-queued notifications ambiguous no-window document-mismatch focus-loss document-change window-destroy engine-destroy document-destroy probe-good probe-already probe-alias probe-mapping probe-hidden probe-ambiguous probe-timeout probe-cancel-after probe-nested-cancel probe-nested-deadline probe-getter-expiry probe-getter-sticky probe-getter-document probe-getter-target-expiry probe-getter-target-sticky; do
  if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
    /opt/codex/rm2/5.8.203/sysroots/x86_64-codexsdk-linux/usr/bin/qemu-arm \
      -L "$SDKTARGETSYSROOT" -E LANG=C.UTF-8 -E QT_QPA_PLATFORM=offscreen \
      -E QT_PLUGIN_PATH="$SDKTARGETSYSROOT/usr/lib/plugins" \
      -E QML_IMPORT_PATH="$SDKTARGETSYSROOT/usr/lib/qml" "$work/fixture" "$case"
  else QT_QPA_PLATFORM=offscreen "$work/fixture" "$case"; fi
done
