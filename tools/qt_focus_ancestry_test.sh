#!/bin/sh
set -eu
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then . "$QT_PROBE_SDK_ENV"; fi
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then moc="$OECORE_NATIVE_SYSROOT/usr/libexec/moc";
else moc="$(pkg-config --variable=libexecdir Qt6Core)/moc"; fi
for file in qt_page_facts qt_page_facts_entry; do "$moc" "tools/$file.h" -o "$work/$file.moc"; done
"$moc" tools/qt_page_facts_fixture.cpp -o "$work/qt_page_facts_fixture.moc"
"$moc" tools/qt_focus_ancestry_fixture.cpp -o "$work/qt_focus_ancestry_fixture.moc"
"$moc" tools/qt_focus_scene_funnel_fixture.cpp -o "$work/qt_focus_scene_funnel_fixture.moc"
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -O2 -fPIC -Itools -I"$work" \
 tools/qt_focus_scene_funnel_fixture.cpp $(pkg-config --cflags --libs Qt6Qml Qt6Gui Qt6Quick) -o "$work/scene-funnel-fixture"
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
 "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" -E LANG=C.UTF-8 \
  -E QT_QPA_PLATFORM=offscreen -E QT_PLUGIN_PATH="$SDKTARGETSYSROOT/usr/lib/plugins" \
  -E QML_IMPORT_PATH="$SDKTARGETSYSROOT/usr/lib/qml" "$work/scene-funnel-fixture"
else QT_QPA_PLATFORM=offscreen "$work/scene-funnel-fixture"; fi
cat > "$work/key-positive.cpp" <<'CPP'
#include "qt_retained_owner_ticket.h"
#include <type_traits>
static_assert(!std::is_aggregate_v<qml_access::RetainedOwnerFactoryKey>);
namespace qml_access { class FactsEntry { void factory(){RetainedOwnerFactoryKey a{};RetainedOwnerFactoryKey b;} }; }
CPP
${CXX:-c++} -std=c++17 -fPIC -fsyntax-only -Itools $(pkg-config --cflags Qt6Qml Qt6Gui Qt6Quick) "$work/key-positive.cpp"
for case in free-brace free-paren session-brace session-paren; do
 printf '#include "qt_retained_owner_ticket.h"\n' > "$work/key-negative.cpp"
 case "$case" in
 free-brace) printf 'void bad(){qml_access::RetainedOwnerFactoryKey key{};}\n' >> "$work/key-negative.cpp" ;;
 free-paren) printf 'void bad(){auto key=qml_access::RetainedOwnerFactoryKey();}\n' >> "$work/key-negative.cpp" ;;
 session-brace) printf 'namespace qml_access {class PageFactsSession{void bad(){RetainedOwnerFactoryKey key{};}};}\n' >> "$work/key-negative.cpp" ;;
 session-paren) printf 'namespace qml_access {class PageFactsSession{void bad(){auto key=RetainedOwnerFactoryKey();}};}\n' >> "$work/key-negative.cpp" ;;
 esac
 if ${CXX:-c++} -std=c++17 -fPIC -fsyntax-only -Itools $(pkg-config --cflags Qt6Qml Qt6Gui Qt6Quick) "$work/key-negative.cpp" > "$work/key-negative.log" 2>&1; then
  printf 'FAIL unauthorized passkey %s\n' "$case"; exit 1
 fi
 grep -q private "$work/key-negative.log"
 printf 'PASS actual-header passkey refusal %s\n' "$case"
done
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -O2 -fPIC -I"$work" \
 tools/qt_focus_ancestry_fixture.cpp tools/qt_page_facts_entry.cpp tools/qt_page_facts.cpp \
 $(pkg-config --cflags --libs Qt6Qml Qt6Gui Qt6Quick) -o "$work/focus-fixture"
for case in receiver-document-type-invalid receiver-document-type-quuid receiver-document-type-other receiver-document-type-historical receiver-identity-complete receiver-identity-wrong-tuple identity-read-schedule receiver-unfocused-complete receiver-v7-unfocused-refusal receiver-unfocused-wrong-tuple receiver-unfocused-two receiver-depth32-complete receiver-depth32-overflow receiver-v6-depth16 receiver-4096-complete receiver-4096-overflow receiver-v5-item-overflow receiver-depth16-complete receiver-depth16-overflow receiver-v4-depth8 receiver-2048-complete receiver-2048-overflow receiver-1024-complete receiver-1024-overflow receiver-512-complete receiver-512-overflow receiver-good receiver-other-page receiver-two receiver-cap receiver-depth receiver-getter-loss receiver-facts-refusal funnel-serializer good large edge24 edge25 ambiguous focus-event initial-invalidated aba-out aba-in generation ticket-reentrant second-factory delivery-generation getter-counts metadata-only metadata-ticket-loss scene9-negative endpoint-capture endpoint-visual endpoint-bindings endpoint-final1 endpoint-final2; do
 if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
  "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" -E LANG=C.UTF-8 \
   -E QT_QPA_PLATFORM=offscreen -E QT_PLUGIN_PATH="$SDKTARGETSYSROOT/usr/lib/plugins" \
   -E QML_IMPORT_PATH="$SDKTARGETSYSROOT/usr/lib/qml" "$work/focus-fixture" "$case"
 else QT_QPA_PLATFORM=offscreen "$work/focus-fixture" "$case"; fi
done
