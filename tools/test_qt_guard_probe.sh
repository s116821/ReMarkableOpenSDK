#!/bin/bash
# Run only in an owned disposable SDK container with its environment sourced.
set -eu
: "${SDKTARGETSYSROOT:?Source the vendor SDK environment first}"
: "${CXX:?}"
src=$(cd -- "$(dirname -- "$0")" && pwd)
out=$(mktemp -d)
h=$SDKTARGETSYSROOT/usr/include
flags="-I$h/QtCore/6.10.3 -I$h/QtCore/6.10.3/QtCore -I$h/QtQml/6.10.3 -I$h/QtQml/6.10.3/QtQml"
qemu=/opt/codex/rm2/5.8.203/sysroots/x86_64-codexsdk-linux/usr/bin/qemu-arm
dir=/run/rmb-qt-probe-f7629ec893af4149950ee96c27981d05
mkdir -m 700 "$dir"
cat > "$out/config.h" <<'EOF'
#define QT_PROBE_NONCE "f7629ec893af4149950ee96c27981d05"
#define QT_GUARD_MODULE "Owned.GuardProbe"
#define QT_GUARD_NAME "OwnedObject"
#define QT_GUARD_META_OBJECT (&QObject::staticMetaObject)
EOF
$CXX -std=c++17 -Wall -Wextra -Werror -O2 -fPIC -shared -fvisibility=hidden -Wl,-z,defs \
    -include "$out/config.h" $flags "$src/qt_guard_probe.cpp" $(pkg-config --cflags --libs Qt6Qml) -o "$out/probe.so"
for variant in normal no-rtti; do
    extra=""
    if [ "$variant" = no-rtti ]; then extra=-fno-rtti; fi
    $CXX -std=c++17 -Wall -Wextra -Werror -O2 -fvisibility=hidden $extra \
        -include "$out/config.h" $flags "$src/qt_guard_probe_fixture.cpp" \
        $(pkg-config --cflags --libs Qt6Qml) -o "$out/fixture-$variant"
done
for mode in live deleted missing factory wrong-version wrong-meta duplicate retention rtti-mismatch; do
    variant=normal
    if [ "$mode" = rtti-mismatch ]; then variant=no-rtti; fi
    "$qemu" -L "$SDKTARGETSYSROOT" -E LANG=C.UTF-8 -E LD_PRELOAD="$out/probe.so" "$out/fixture-$variant" "$mode"
    echo "$mode PASS"
    rm -f -- "$dir/callback.json"
done
# The disposable container owns the build directory and destroys it on exit.
rmdir -- "$dir"
