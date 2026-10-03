#!/bin/sh
set -eu
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then . "$QT_PROBE_SDK_ENV"; fi
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then moc="$OECORE_NATIVE_SYSROOT/usr/libexec/moc";
else moc="$(pkg-config --variable=libexecdir Qt6Core)/moc"; fi
"$moc" tools/qt_page_facts.h -o "$work/qt_page_facts.moc"
"$moc" tools/qt_page_facts_fixture.cpp -o "$work/qt_page_facts_fixture.moc"
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -O2 -fPIC -I"$work" \
  tools/qt_page_facts_fixture.cpp tools/qt_page_facts.cpp \
  $(pkg-config --cflags --libs Qt6Qml Qt6Gui Qt6Quick) -o "$work/fixture"
for case in good-one good-six good-cap cap-zero cap-large cap-small budget-zero canonical nil duplicate empty hidden disabled no-window wrong-window wrong-engine focus document count order reverse alias page index metadata signal-shape incompatible ambiguous cancel dirty away-back redirection worker signal-count signal-added signal-added-list signal-moved signal-moved-list signal-removed signal-metadata signal-orientation signal-viewport signal-document signal-page getter-focus getter-context getter-cancel deadline nested nested-cancel nested-deadline nested-destroy destroy scene-destroy delivery-cancel delivery-dirty delivery-deadline engine-destroy window-destroy delivery-reentry delivery-progress-destroy release silent-aba; do
  if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
    /opt/codex/rm2/5.8.203/sysroots/x86_64-codexsdk-linux/usr/bin/qemu-arm \
      -L "$SDKTARGETSYSROOT" -E LANG=C.UTF-8 -E QT_QPA_PLATFORM=offscreen \
      -E QT_PLUGIN_PATH="$SDKTARGETSYSROOT/usr/lib/plugins" \
      -E QML_IMPORT_PATH="$SDKTARGETSYSROOT/usr/lib/qml" "$work/fixture" "$case"
  else QT_QPA_PLATFORM=offscreen "$work/fixture" "$case"; fi
done
