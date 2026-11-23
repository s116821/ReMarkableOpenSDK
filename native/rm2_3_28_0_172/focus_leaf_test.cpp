#include "focus_leaf.h"

#include <QCoreApplication>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <thread>

using rem61_focus::Leaf;
using rem61_focus::sample;

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    QQuickWindow window;
    QQuickItem scope(window.contentItem());
    scope.setFlag(QQuickItem::ItemIsFocusScope);
    QQuickItem scene(&scope), inner(&scene), sibling(&scope), outside(window.contentItem());
    window.resize(120, 120);
    window.show();
    window.requestActivate();
    QCoreApplication::processEvents();
    const auto keep = [] { return true; };
    const auto read = [&] { return sample(&window, &scope, &scene, keep); };
    int count = 0;
    auto check = [&](bool ok, const char *label) {
        if (!ok) {
            std::fprintf(stderr, "FAIL: %s\n", label);
            std::exit(1);
        }
        ++count;
    };

    scene.forceActiveFocus();
    check(read() == Leaf::ExactScene, "exact current SceneView");
    inner.forceActiveFocus();
    check(read() == Leaf::InsideScene, "active leaf within current scene");
    sibling.forceActiveFocus();
    check(scope.hasActiveFocus() && read() == Leaf::ScopeOnly, "outer activeFocus with sibling leaf");
    outside.forceActiveFocus();
    check(read() == Leaf::OutsideScope, "active leaf outside owned scope");
    check(sample(&window, &scope, &outside, keep) == Leaf::Refused, "unjoined scene");
    scene.forceActiveFocus();
    check(sample(&window, &scope, &scene, [] { return false; }) == Leaf::Refused, "visit guard failed");
    check(sample(&window, &scope, &scene, [&] {
        sibling.forceActiveFocus();
        scene.forceActiveFocus();
        return true;
    }) == Leaf::Refused && read() == Leaf::ExactScene, "leave-return is visible during one sample");
    check(sample(&window, &scope, &scene, [&] {
        scene.setParentItem(window.contentItem());
        scene.setParentItem(&scope);
        return true;
    }) == Leaf::Refused, "reparent-return is visible during one sample");
    auto transient = std::make_unique<QQuickItem>(&scope);
    transient->forceActiveFocus();
    check(sample(&window, &scope, &scene, [&] {
        transient.reset();
        return true;
    }) == Leaf::Refused, "focus leaf destroyed during sample");
    Leaf worker = Leaf::NoLeaf;
    std::thread thread([&] { worker = read(); });
    thread.join();
    check(worker == Leaf::Refused, "wrong thread");
    auto deep = std::make_unique<QQuickItem>(&scene);
    auto *last = deep.get();
    for (qsizetype i = 0; i < rem61_focus::MaxParents; ++i)
        last = new QQuickItem(last);
    last->forceActiveFocus();
    check(read() == Leaf::Refused, "bounded ancestry");
    QQuickWindow other;
    other.resize(120, 120);
    other.show();
    other.requestActivate();
    QCoreApplication::processEvents();
    check(read() == Leaf::OtherWindow, "different active window");
    std::printf("%d focus leaf checks passed on Qt %s\n", count, qVersion());
}
