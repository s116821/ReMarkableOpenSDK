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
variant=${RSDK_BUILD_VARIANT:-observation}
if [ "$variant" = development-pre-enter-diagnostic-v1 ]; then
    test "$(sha256sum "$QT_PROBE_SDK_ENV" | cut -d ' ' -f 1)" = d77b2e8bed5d8ce4bd953b9ae2a5397e3bd8693fc065600cf536bad5dac53606
fi
. "$QT_PROBE_SDK_ENV"
test "$(pkg-config --modversion Qt6Core)" = 6.10.3
test "$(sha256sum "$RSDK_DOBBY_ARCHIVE" | cut -d ' ' -f 1)" = 622b17dc65d0dcad717d83264f783ad12044565ed8ea86f9e5088d9abf5328ef
test "$(sha256sum "$RSDK_DOBBY_INCLUDE/dobby.h" | cut -d ' ' -f 1)" = ef532c5dba1ea4f6bd620037c70c3bc2f86b66fad0d22ab121ef76a478def09a
set --
case "$variant" in
    observation) manifest="$root/profile.json"; probe_flag= ;;
    observation-endpoint-diagnostics-v1) manifest="$root/profile.json"; probe_flag=-DRSDK_ENDPOINT_DIAGNOSTICS_V1=1 ;;
    installed-reader-insert-v1) manifest="$root/installed-reader-insert-v1.json"; probe_flag=-DRSDK_INSTALLED_READER_INSERT_V1=1 ;;
    development-effect-repeat-insert-v1) manifest="$root/development-effect-repeat-insert-v1.json"; probe_flag="-DRSDK_DEVELOPMENT_EFFECT_PROBE_V1=1 -DRSDK_DEVELOPMENT_REPEAT_INSERT_V1=1 -DRSDK_ENDPOINT_DIAGNOSTICS_V1=1" ;;
    development-effect-write-reopen-v1) manifest="$root/development-effect-write-reopen-v1.json"; probe_flag="-DRSDK_DEVELOPMENT_EFFECT_PROBE_V1=1 -DRSDK_DEVELOPMENT_ORDINARY_CLOSE_V1=1 -DRSDK_DEVELOPMENT_FIXTURE_REOPEN_V1=1 -DRSDK_ENDPOINT_DIAGNOSTICS_V1=1" ;;
    development-effect-close-probe-v1) manifest="$root/development-effect-close-probe-v1.json"; probe_flag="-DRSDK_DEVELOPMENT_EFFECT_PROBE_V1=1 -DRSDK_DEVELOPMENT_ORDINARY_CLOSE_V1=1 -DRSDK_ENDPOINT_DIAGNOSTICS_V1=1" ;;
    development-effect-probe-v1) manifest="$root/development-effect-probe-v1.json"; probe_flag=-DRSDK_DEVELOPMENT_EFFECT_PROBE_V1=1 ;;
    development-pre-enter-diagnostic-v1)
        manifest="$root/development-pre-enter-diagnostic-v1.json"
        probe_flag="-DRSDK_DEVELOPMENT_PRE_ENTER_DIAGNOSTIC_V1=1 -DCOMPOSER_TEXT_PRIVATE_QT -funwind-tables"
        test "$(${CXX:?} -dumpfullversion)" = 13.4.0
        version=$(pkg-config --modversion Qt6Gui)
        test "$version" = 6.10.3
        includes=$(pkg-config --variable=includedir Qt6Gui)
        if [ ! -d "$includes/QtGui/$version" ]; then includes="$SDKTARGETSYSROOT$includes"; fi
        for path in "$includes/QtCore/$version" "$includes/QtCore/$version/QtCore" \
                    "$includes/QtGui/$version" "$includes/QtGui/$version/QtGui"; do
            test -d "$path"
            set -- "$@" "-I$path"
        done
        private_header="$includes/QtGui/$version/QtGui/private/qtextdocument_p.h"
        test "$(sha256sum "$private_header" | cut -d ' ' -f 1)" = 8f79c3a295f83eaa674c7c77d55fb269df33631a97492d4be1a180d7be452c42
        ;;
    *) printf '%s\n' 'unsupported build variant' >&2; exit 1 ;;
esac
profile=$(sha256sum "$manifest" | cut -d ' ' -f 1)
for name in reader_selected_view_probe reader_joined_creation_witness reader_worker_callback_witness reader_insertion_guard_entry reader_request_retirement; do
    "$OECORE_NATIVE_SYSROOT/usr/libexec/moc" "$root/$name.h" -o "$work/moc_$name.cpp"
done
"$OECORE_NATIVE_SYSROOT/usr/libexec/moc" "$root/qt_qml_creation_bridge.h" -o "$work/qt_qml_creation_bridge.moc"
${CXX:?} -std=c++17 -marm -Wall -Wextra -Werror -O2 -fPIC -fexceptions -frtti \
    -fno-lto -fno-gnu-unique -fvisibility=hidden -ftls-model=initial-exec \
    $probe_flag "$@" -DRSDK_PROFILE_SHA256="\"$profile\"" -I"$RSDK_DOBBY_INCLUDE" -I"$work" \
    "$root/bootstrap.cpp" "$root/composer_pre_enter.cpp" "$root/reader_insertion_guard_native_profile.cpp" "$root/qt_qml_creation_bridge.cpp" "$root/resident_mutation.cpp" \
    "$work"/moc_*.cpp \
    $(pkg-config --cflags --libs Qt6Core Qt6Network Qt6Qml Qt6Quick) "$RSDK_DOBBY_ARCHIVE" -ldl -pthread \
    -shared -Wl,-z,now,-z,relro,-z,nodelete,--no-undefined,--exclude-libs,ALL -o "$work/libremarkable-resident.so"
cp "$manifest" "$work/profile.json"
printf '%s\n' "$variant" > "$work/build-variant.txt"
cp "$root/Dobby-LICENSE" "$work/Dobby-LICENSE"
cp "$root/../../LICENSE" "$work/LICENSE"
${READELF:-readelf} -h -d "$work/libremarkable-resident.so" > "$work/elf.txt"
sha256sum "$work/libremarkable-resident.so" "$work/profile.json" "$RSDK_DOBBY_ARCHIVE" > "$work/artifacts.sha256"
sha256sum "$root/bootstrap.cpp" "$root/composer_pre_enter.cpp" "$root/reader_insertion_guard_native_profile.cpp" "$root/qt_qml_creation_bridge.cpp" \
    "$root/resident_mutation.cpp" "$root"/*.h "$root/../resident"/*.h "$manifest" "$root/build.sh" "$RSDK_DOBBY_INCLUDE/dobby.h" \
    "$work"/moc_*.cpp "$work/qt_qml_creation_bridge.moc" > "$work/inputs.sha256"
${CXX:?} --version > "$work/compiler.txt"
if [ "$variant" = development-pre-enter-diagnostic-v1 ]; then
    ${READELF:-readelf} --unwind "$work/libremarkable-resident.so" > "$work/unwind.txt"
    sha256sum "$QT_PROBE_SDK_ENV" "$private_header" >> "$work/inputs.sha256"
fi
printf '%s\n' "Built unqualified $variant native artifact. No installation or activation performed."
