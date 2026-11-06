#!/bin/sh
# Explicit non-installing observation build. No approved profile exists yet.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
scratch=$(mktemp -d)
trap 'rm -f "$scratch/profile-check"; rmdir "$scratch"' EXIT HUP INT TERM
${RSDK_HOST_CXX:-c++} -std=c++17 -Wall -Wextra -Werror "$root/profile_check.cpp" -o "$scratch/profile-check"
checked=$("$scratch/profile-check") || exit "$?"
profile_hash=$(printf '%s\n' "$checked" | sed -n '1p')
qt_version=$(printf '%s\n' "$checked" | sed -n '2p')
if [ "${1:-}" = --check-inputs ]; then printf '%s\n' "$checked"; exit 0; fi
test "$#" = 0
: "${QT_PROBE_SDK_ENV:?reviewed RM1 ARM SDK environment required}"
: "${RSDK_RM1_PROFILE_MANIFEST:?reviewed exact RM1 profile manifest required}"
: "${RSDK_NATIVE_OUTPUT:?fresh output directory required}"
test "$(sha256sum "$RSDK_RM1_PROFILE_MANIFEST" | cut -d ' ' -f 1)" = "$profile_hash"
test ! -e "$RSDK_NATIVE_OUTPUT"
. "$QT_PROBE_SDK_ENV"
test "$(pkg-config --modversion Qt6Core)" = "$qt_version"
: "${CXX:?RM1 target C++ compiler required}"
: "${READELF:?RM1 target readelf required}"
: "${OECORE_NATIVE_SYSROOT:?matching RM1 moc required}"
out=$RSDK_NATIVE_OUTPUT
mkdir "$out"
"$OECORE_NATIVE_SYSROOT/usr/libexec/moc" "$root/../rm2_3_28_0_172/reader_selected_view_probe.h" -o "$out/moc_reader_selected_view_probe.cpp"
$CXX -std=c++17 -marm -Wall -Wextra -Werror -O2 -fPIC -fexceptions -frtti \
    -fno-lto -fno-gnu-unique -fvisibility=hidden -ftls-model=initial-exec \
    "$root/bootstrap.cpp" "$root/admission.cpp" "$out/moc_reader_selected_view_probe.cpp" \
    $(pkg-config --cflags --libs Qt6Core Qt6Gui Qt6Network Qt6Qml Qt6Quick) -ldl -pthread \
    -shared -Wl,-z,now,-z,relro,-z,nodelete,--no-undefined \
    -o "$out/libremarkable-rm1-observation.so"
"$READELF" -W -h -A -d -s "$out/libremarkable-rm1-observation.so" > "$out/elf.txt"
python3 "$root/audit_elf.py" "$out/elf.txt"
cp "$RSDK_RM1_PROFILE_MANIFEST" "$out/profile.json"
cp "$root/../../LICENSE" "$out/LICENSE"
sha256sum "$out/libremarkable-rm1-observation.so" "$out/profile.json" > "$out/artifacts.sha256"
sha256sum "$root"/*.h "$root"/*.cpp "$root/build.sh" "$root/audit_elf.py" \
    "$root/../resident"/*.h "$root/../rm2_3_28_0_172"/*.h > "$out/source.sha256"
