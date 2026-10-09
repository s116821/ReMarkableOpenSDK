#!/bin/sh
set -eu
if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then . "$QT_PROBE_SDK_ENV"; fi
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -O2 -fPIC tools/qt_page_open_arm_fixture.cpp \
  $(pkg-config --cflags --libs Qt6Core) -o "$work/fixture"
for case in good immediate duplicate closed-before-release none unrelated wrong wrong-process partial oversized token-mode symlink stale stale-marker stale-temp directory-mode closed restoring closed-after restore-after cancel cutoff late replacement creation-good creation-duplicate creation-wrong-process creation-stale creation-restoring creation-stopping creation-closed-before-release creation-cutoff creation-late; do
  if [ -n "${QT_PROBE_SDK_ENV:-}" ]; then
    /opt/codex/rm2/5.8.203/sysroots/x86_64-codexsdk-linux/usr/bin/qemu-arm \
      -L "$SDKTARGETSYSROOT" -E LANG=C.UTF-8 "$work/fixture" "$case"
  else "$work/fixture" "$case"; fi
done
