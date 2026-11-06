#!/bin/sh
# Explicit owned ELF generation only, no execution/loading. Requires selected SDK.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
: "${QT_PROBE_SDK_ENV:?selected RM1 SDK required}"
: "${RSDK_BINDING_FIXTURE_OUTPUT:?fresh owned output directory required}"
test ! -e "$RSDK_BINDING_FIXTURE_OUTPUT"
. "$QT_PROBE_SDK_ENV"
out=$RSDK_BINDING_FIXTURE_OUTPUT
mkdir "$out"
$CC -marm -O2 -fPIC -c "$root/fixtures/arm.c" -o "$out/arm.o"
$CC -mthumb -O2 -fPIC -c "$root/fixtures/thumb.c" -o "$out/thumb.o"
$CC -shared -nostdlib -Wl,-z,now,-z,relro,--version-script,"$root/fixtures/versions.map",-soname,libbinding-fixture.so "$out/arm.o" "$out/thumb.o" -o "$out/libbinding-fixture.so"
$CC -marm -O2 -fPIC -shared -nostdlib -Wl,-z,now,-z,relro,--no-undefined "$root/fixtures/consumer.c" -L"$out" -lbinding-fixture -o "$out/consumer.so"
$CC -marm -O2 -fno-pie -no-pie "$root/fixtures/main.c" -L"$out" -lbinding-fixture -o "$out/main"
for file in libbinding-fixture.so consumer.so main; do
    $READELF -W -h -A -l -d -s -r -V "$out/$file" > "$out/$file.elf.txt"
done
sha256sum "$out"/*.o "$out/libbinding-fixture.so" "$out/consumer.so" "$out/main" > "$out/hashes.txt"
