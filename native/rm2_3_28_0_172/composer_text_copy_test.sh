#!/bin/sh
# Owned document and typed request fakes; never a device payload.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
work=${COMPOSER_TEXT_OUTPUT:?fresh output directory required}
test ! -e "$work"
mkdir -p "$work"
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
    . "$QT_PROBE_SDK_ENV"
    moc="$OECORE_NATIVE_SYSROOT/usr/libexec/moc"
else
    moc=${QT_HOST_MOC:-/usr/lib/qt6/libexec/moc}
fi
version=$(pkg-config --modversion Qt6Gui)
includes=$(pkg-config --variable=includedir Qt6Gui)
if [ ! -d "$includes/QtGui/$version" ] && [ -n "${SDKTARGETSYSROOT:-}" ]; then
    includes="$SDKTARGETSYSROOT$includes"
fi
test -r "$includes/QtGui/$version/QtGui/private/qtextdocument_p.h"
private="-I$includes/QtCore/$version -I$includes/QtCore/$version/QtCore -I$includes/QtGui/$version -I$includes/QtGui/$version/QtGui"
sanitizers=
if [ "${COMPOSER_TEXT_SANITIZERS:-0}" = 1 ]; then sanitizers='-fsanitize=address,undefined -fno-omit-frame-pointer -no-pie'; fi
"$moc" "$root/composer_text_copy_test.cpp" -o "$work/composer_text_copy_test.moc"
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -O1 -g -fPIC -pthread $sanitizers \
    -DCOMPOSER_TEXT_PRIVATE_QT $private -I"$work" \
    "$root/composer_text_copy_test.cpp" $(pkg-config --cflags --libs Qt6Core Qt6Gui) -ldl -o "$work/text-test"
printf '%s\n' '#include "composer_scene_delivery.h"' '#include "composer_pre_enter.h"' > "$work/production.cpp"
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -fPIC -pthread -DCOMPOSER_TEXT_PRIVATE_QT \
    $private -I"$root" $(pkg-config --cflags Qt6Core Qt6Gui) \
    -c "$work/production.cpp" -o "$work/production-private.o"
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -fPIC -pthread -I"$root" \
    $(pkg-config --cflags Qt6Core Qt6Gui) -c "$work/production.cpp" -o "$work/production-ordinary.o"
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -fPIC -pthread -DCOMPOSER_TEXT_PRIVATE_QT \
    $private $(pkg-config --cflags Qt6Core Qt6Gui) \
    -c "$root/composer_pre_enter.cpp" -o "$work/pre-enter-private.o"
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -fPIC -pthread \
    $(pkg-config --cflags Qt6Core Qt6Gui) \
    -c "$root/composer_pre_enter.cpp" -o "$work/pre-enter-ordinary.o"
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
    timeout --kill-after=1s 40s "$OECORE_NATIVE_SYSROOT/usr/bin/qemu-arm" -L "$SDKTARGETSYSROOT" -E QT_QPA_PLATFORM=offscreen -E QT_PLUGIN_PATH="$SDKTARGETSYSROOT/usr/lib/plugins" "$work/text-test"
else
    QT_QPA_PLATFORM=offscreen ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 timeout --kill-after=1s 40s "$work/text-test"
fi
