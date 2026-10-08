#!/bin/sh
set -eu
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then . "$QT_PROBE_SDK_ENV"; fi
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then moc="$OECORE_NATIVE_SYSROOT/usr/libexec/moc";
else moc="$(pkg-config --variable=libexecdir Qt6Core)/moc"; fi
for file in qt_page_facts qt_page_facts_entry; do "$moc" "tools/$file.h" -o "$work/$file.moc"; done
"$moc" tools/qt_page_facts_fixture.cpp -o "$work/qt_page_facts_fixture.moc"
if [ -n "${FACTS_BUDDY_SOURCE:-}" ]; then set -- -DOWNED_CAPTURE_PUBLISHER -I"$FACTS_BUDDY_SOURCE/tools/native_page_facts_probe"; else set --; fi
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -O2 -fPIC -I"$work" \
 "$@" \
 tools/qt_capture_observation_fixture.cpp tools/qt_page_facts_entry.cpp tools/qt_page_facts.cpp \
 $(pkg-config --cflags --libs Qt6Qml Qt6Gui Qt6Quick) -o "$work/capture-fixture"
for case in good duplicate wrong-token early-facts cross-purpose empty grab-epoch grab-late grab-cancel visual-epoch token-replaced png-replaced setup-expiry setup-near external-buffer dimension-mismatch completion-late completion-epoch nested-early nested-purpose visual-double-click publication-alias foreign-temporary temporary-reappears temporary-unlink; do
 if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
  /opt/codex/rm2/5.8.203/sysroots/x86_64-codexsdk-linux/usr/bin/qemu-arm \
   -L "$SDKTARGETSYSROOT" -E LANG=C.UTF-8 -E QT_QPA_PLATFORM=offscreen \
   -E QT_PLUGIN_PATH="$SDKTARGETSYSROOT/usr/lib/plugins" \
   -E QML_IMPORT_PATH="$SDKTARGETSYSROOT/usr/lib/qml" "$work/capture-fixture" "$case"
 else QT_QPA_PLATFORM=offscreen "$work/capture-fixture" "$case"; fi
done
if [ -n "${FACTS_BUDDY_SOURCE:-}" ]; then
 root="$work/rmb-qt-probe-0123456789abcdef0123456789abcdef"
 mkdir -m700 "$root"
 printf 'Owned fixture data only; not an ELF extension.\n' > "$root/payload.so"
 chmod 600 "$root/payload.so"
 if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
  FACTS_OWNED_ROOT="$root" LD_PRELOAD="$root/payload.so" /opt/codex/rm2/5.8.203/sysroots/x86_64-codexsdk-linux/usr/bin/qemu-arm \
   -L "$SDKTARGETSYSROOT" -E LANG=C.UTF-8 -E QT_QPA_PLATFORM=offscreen \
   -E QT_PLUGIN_PATH="$SDKTARGETSYSROOT/usr/lib/plugins" -E QML_IMPORT_PATH="$SDKTARGETSYSROOT/usr/lib/qml" "$work/capture-fixture" publisher-interleave
 else FACTS_OWNED_ROOT="$root" LD_PRELOAD="$root/payload.so" QT_QPA_PLATFORM=offscreen "$work/capture-fixture" publisher-interleave; fi
fi
