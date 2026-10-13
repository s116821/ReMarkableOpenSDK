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
 tools/qt_capture_observation_fixture.cpp tools/qt_page_facts_entry.cpp tools/qt_page_facts.cpp \
 $(pkg-config --cflags --libs Qt6Qml Qt6Gui Qt6Quick) -o "$work/fixture"
for case in ${CAPTURE_ORDER_CASES:-order-good order-real-clock order-owned-pixels order-window-grab order-stale order-cancel order-binding-drift order-write-deadline order-reverse order-drift order-queued-drift order-invalidate order-deadline order-queued-deadline order-cross-capture order-cross-source order-wrong-token order-legacy-reject order-source-reject good completion-diagnostic-success source-facts-good source-facts-reverse}; do
 case_backend="${QT_QUICK_BACKEND:-}"
 if [ "$case" = order-window-grab ]; then case_backend=software; fi
 if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
  QT_QUICK_BACKEND="$case_backend" /opt/codex/rm2/5.8.203/sysroots/x86_64-codexsdk-linux/usr/bin/qemu-arm \
   -L "$SDKTARGETSYSROOT" -E LANG=C.UTF-8 -E QT_QPA_PLATFORM=offscreen \
   -E QT_PLUGIN_PATH="$SDKTARGETSYSROOT/usr/lib/plugins" \
   -E QML_IMPORT_PATH="$SDKTARGETSYSROOT/usr/lib/qml" "$work/fixture" "$case"
 else QT_QUICK_BACKEND="$case_backend" QT_QPA_PLATFORM=offscreen "$work/fixture" "$case"; fi
done
