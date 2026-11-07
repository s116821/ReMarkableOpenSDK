#!/bin/sh
# Owned Qt host fixtures for the focus leaf and the selected Reader join.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
out=${READER_FOCUS_TEST_OUTPUT:?fresh output directory required}
test ! -e "$out" && test ! -L "$out"
mkdir -p "$out"
moc=${QT_HOST_MOC:-/usr/lib/qt6/libexec/moc}
qt_modules='Qt6Core Qt6Gui Qt6Qml Qt6Quick'

"$moc" $(pkg-config --cflags $qt_modules) "$root/reader_selected_view_probe.h" -o "$out/moc_reader_selected_view_probe.cpp"
"$moc" $(pkg-config --cflags $qt_modules) "$root/reader_selected_view_fixture.cpp" -o "$out/reader_selected_view_fixture.moc"

${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -fPIC -no-pie -pthread \
    "$root/focus_leaf_test.cpp" $(pkg-config --cflags --libs $qt_modules) -o "$out/focus-leaf-test"
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -fPIC -no-pie -pthread -I"$out" \
    "$root/joined_focus_test.cpp" "$out/moc_reader_selected_view_probe.cpp" \
    $(pkg-config --cflags --libs $qt_modules) -o "$out/joined-focus-test"

QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software timeout --kill-after=1s 15s "$out/focus-leaf-test"
QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software timeout --kill-after=1s 15s "$out/joined-focus-test"
