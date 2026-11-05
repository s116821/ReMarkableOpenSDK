#!/bin/sh
# Build a reproducible native bundle in the RM2 SDK container.
set -eu

: "${QT_PROBE_SDK_ENV:?exact SDK environment required}"
: "${RSDK_DOBBY_SOURCE_ARCHIVE:?pinned upstream Dobby tarball required}"
root=/src/native/rm2_3_28_0_172
test -f "$root/installed-reader-insert-v1.sha256"
test -z "$(ls -A /source)"
test -z "$(ls -A /out)"
test "$(sha256sum "$RSDK_DOBBY_SOURCE_ARCHIVE" | cut -d ' ' -f 1)" = b5dddb530ae3b1abe8b61dd3a690eaf0ffe74900b0313c3d1bee0597691cf2d3

tar -xzf "$RSDK_DOBBY_SOURCE_ARCHIVE" -C /source --strip-components=1
# Give the header-defined method inline linkage and initialize ARM/Thumb mode
# before InterceptEntry checks the target address.
sed -i 's/^Logger \*Logger::Shared()/inline Logger *Logger::Shared()/' \
    /source/external/logging/logging/logging.h
sed -i '/^  this->type = type;$/a\  this->thumb_mode = false;' /source/source/InterceptEntry.cpp
sha256sum --check <<'EOF'
60153cefa6fdf9617c5a4ecd317cfb1b1795eebf2974b2c711fca998c3206bcd  /source/external/logging/logging/logging.h
d9c4c98156e43e8ee67e2d08c48604fa0d0d04ff4443ec45b262582b214499e2  /source/source/InterceptEntry.cpp
EOF
. "$QT_PROBE_SDK_ENV"
# The pinned archive embeds its original UTC build day and debug build path.
SOURCE_DATE_EPOCH=1792713600 cmake -S /inputs -B /out/build \
    -DCMAKE_TOOLCHAIN_FILE="$OECORE_NATIVE_SYSROOT/usr/share/cmake/OEToolchainConfig.cmake" \
    -DCMAKE_BUILD_TYPE=Release -DDOBBY_DEBUG=OFF -DPlugin.SymbolResolver=OFF \
    -DDOBBY_BUILD_EXAMPLE=OFF -DDOBBY_BUILD_TEST=OFF -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build /out/build --target dobby_static -j4
mv /out/build /out/dobby

export RSDK_DOBBY_ARCHIVE=/out/dobby/upstream/libdobby.a
export RSDK_DOBBY_INCLUDE=/source/include
export RSDK_NATIVE_OUTPUT=/out/build
export RSDK_BUILD_VARIANT=installed-reader-insert-v1
sh "$root/build.sh"
(cd /out/build && sha256sum --check "$root/installed-reader-insert-v1.sha256")

bundle=/out/bundle
mkdir "$bundle"
cp /out/build/libremarkable-resident.so /out/build/profile.json \
    /out/build/LICENSE /out/build/Dobby-LICENSE "$bundle/"
(
    cd "$bundle"
    sha256sum libremarkable-resident.so profile.json LICENSE Dobby-LICENSE > SHA256SUMS
    tar --format=ustar --mtime=@0 --owner=0 --group=0 --numeric-owner \
        --mode=u=rw,go=r -cf /out/bundle.tar \
        Dobby-LICENSE LICENSE SHA256SUMS libremarkable-resident.so profile.json
)
gzip -n /out/bundle.tar
mv /out/bundle.tar.gz /out/remarkable-open-sdk-rm2-3.28.0.172-reader-insert.tar.gz
