#!/bin/sh
# Build only. Never install, load or execute the native artifact.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
work=${RSDK_NATIVE_OUTPUT:?fresh output directory required}
test ! -e "$work"
mkdir -p "$work"
: "${QT_PROBE_SDK_ENV:?exact SDK environment required}"
: "${RSDK_DOBBY_ARCHIVE:?reviewed Dobby archive required}"
: "${RSDK_DOBBY_INCLUDE:?matching Dobby headers required}"
. "$QT_PROBE_SDK_ENV"
test "$(pkg-config --modversion Qt6Core)" = 6.10.3
test "$(sha256sum "$RSDK_DOBBY_ARCHIVE" | cut -d ' ' -f 1)" = 622b17dc65d0dcad717d83264f783ad12044565ed8ea86f9e5088d9abf5328ef
test "$(sha256sum "$RSDK_DOBBY_INCLUDE/dobby.h" | cut -d ' ' -f 1)" = ef532c5dba1ea4f6bd620037c70c3bc2f86b66fad0d22ab121ef76a478def09a
profile=$(sha256sum "$root/profile.json" | cut -d ' ' -f 1)
for name in reader_selected_view_probe reader_joined_creation_witness reader_worker_callback_witness reader_insertion_guard_entry reader_request_retirement; do
    "$OECORE_NATIVE_SYSROOT/usr/libexec/moc" "$root/$name.h" -o "$work/moc_$name.cpp"
done
"$OECORE_NATIVE_SYSROOT/usr/libexec/moc" "$root/qt_qml_creation_bridge.h" -o "$work/qt_qml_creation_bridge.moc"
${CXX:?} -std=c++17 -marm -Wall -Wextra -Werror -O2 -fPIC -fexceptions -frtti \
    -fno-lto -fno-gnu-unique -fvisibility=hidden -ftls-model=initial-exec \
    -DRSDK_PROFILE_SHA256="\"$profile\"" -I"$RSDK_DOBBY_INCLUDE" -I"$work" \
    "$root/bootstrap.cpp" "$root/reader_insertion_guard_native_profile.cpp" "$root/qt_qml_creation_bridge.cpp" "$root/resident_mutation.cpp" \
    "$work"/moc_*.cpp \
    $(pkg-config --cflags --libs Qt6Core Qt6Network Qt6Qml Qt6Quick) "$RSDK_DOBBY_ARCHIVE" -ldl -pthread \
    -shared -Wl,-z,now,-z,relro,-z,nodelete,--no-undefined,--exclude-libs,ALL -o "$work/libremarkable-resident.so"
cp "$root/profile.json" "$work/profile.json"
cp "$root/Dobby-LICENSE" "$work/Dobby-LICENSE"
cp "$root/../../LICENSE" "$work/LICENSE"
${READELF:-readelf} -h -d "$work/libremarkable-resident.so" > "$work/elf.txt"
sha256sum "$work/libremarkable-resident.so" "$work/profile.json" "$RSDK_DOBBY_ARCHIVE" > "$work/artifacts.sha256"
sha256sum "$root/bootstrap.cpp" "$root/reader_insertion_guard_native_profile.cpp" "$root/qt_qml_creation_bridge.cpp" \
    "$root/resident_mutation.cpp" "$root"/*.h "$root/../resident"/*.h "$root/profile.json" "$root/build.sh" "$RSDK_DOBBY_INCLUDE/dobby.h" \
    "$work"/moc_*.cpp "$work/qt_qml_creation_bridge.moc" > "$work/inputs.sha256"
${CXX:?} --version > "$work/compiler.txt"
printf '%s\n' 'Built unqualified read-only native artifact. No installation or activation performed.'
