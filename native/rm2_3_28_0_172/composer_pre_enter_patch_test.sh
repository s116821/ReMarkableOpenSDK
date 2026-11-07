#!/bin/sh
# Raw Linux capsule in owned children; no native builder or activation.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
work=${COMPOSER_PATCH_OUTPUT:?fresh output required}
test ! -e "$work"
mkdir -p "$work"
arch=
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then . "$QT_PROBE_SDK_ENV"; arch=-marm; fi
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -O1 -g -pthread \
    $arch -DCOMPOSER_PATCH_OWNED_TESTING "$root/composer_pre_enter_patch_owned.cpp" -o "$work/capsule-test"
printf '%s\n' '#include "composer_pre_enter_patch_private.h"' > "$work/private.cpp"
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror $arch -I"$root" \
    -c "$work/private.cpp" -o "$work/private-production.o"
manifest="$root/development-pre-enter-diagnostic-v1.json"
digest=$(sha256sum "$manifest" | cut -d ' ' -f 1)
test "$(wc -c < "$manifest")" -eq 750
gate=-DRSDK_DEVELOPMENT_PRE_ENTER_DIAGNOSTIC_V1=1
binding="-DRSDK_PROFILE_SHA256=\"$digest\""
gate_compile() {
    ${CXX:-c++} -std=c++17 $arch -I"$root" "$@" -fsyntax-only "$work/gate.cpp"
}
printf '%s\n' '#include "composer_pre_enter_arm.h"' \
    'static_assert(remarkable::composer_pre_enter::detail::StartupSelected == (EXPECTED != 0));' > "$work/gate.cpp"
gate_compile -DEXPECTED=0 -DCOMPOSER_TEXT_PRIVATE_QT
gate_compile -DEXPECTED=1 "$gate" "$binding" -DCOMPOSER_TEXT_PRIVATE_QT
refuse_gate() {
    if gate_compile -DEXPECTED=1 "$@" > "$work/gate-refusal.log" 2>&1; then
        printf '%s\n' 'Unexpected pre-Enter gate admission' >&2
        exit 1
    fi
    grep -q 'Pre-Enter diagnostic\|pre-Enter diagnostic' "$work/gate-refusal.log"
}
refuse_gate "$gate" -DCOMPOSER_TEXT_PRIVATE_QT
refuse_gate "$gate" "$binding"
refuse_gate "$gate" '-DRSDK_PROFILE_SHA256="wrong"' -DCOMPOSER_TEXT_PRIVATE_QT
for mixed in RSDK_INSTALLED_READER_INSERT_V1 RSDK_DEVELOPMENT_EFFECT_PROBE_V1 \
             RSDK_DEVELOPMENT_REPEAT_INSERT_V1 RSDK_DEVELOPMENT_ORDINARY_CLOSE_V1 \
             RSDK_DEVELOPMENT_FIXTURE_REOPEN_V1 COMPOSER_PRE_ENTER_TESTING \
             COMPOSER_PATCH_OWNED_TESTING RSDK_PRE_ENTER_MARKER_TESTING COMPOSER_TEXT_TESTING \
             COMPOSER_SCENE_TESTING COMPOSER_OBSERVER_TESTING COMPOSER_DELIVERY_TESTING; do
    refuse_gate "$gate" "$binding" -DCOMPOSER_TEXT_PRIVATE_QT "-D$mixed"
done
printf '%s\n' 'PASS: ordinary, exact manifest, missing/wrong and mixed diagnostic build gates'
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
    ${OBJDUMP} -d "$work/capsule-test" > "$work/capsule-disassembly.txt"
    timeout --kill-after=1s 40s "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" "$work/capsule-test"
else
    objdump -d "$work/capsule-test" > "$work/capsule-disassembly.txt"
    timeout --kill-after=1s 40s "$work/capsule-test"
fi
