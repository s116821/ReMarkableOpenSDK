#define main selected_view_baseline_main
#include "reader_selected_view_fixture.cpp"
#undef main

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    qmlRegisterType<QObject>("OwnedProbe", 1, 0, "QtObject");
    qmlRegisterType<QQuickItem>("OwnedProbe", 1, 0, "Item");
    qmlRegisterType<FocusFixture>("OwnedProbe", 1, 0, "FocusScope");
    qmlRegisterType<SceneView>("OwnedProbe", 1, 0, "SceneFixture");

    using rem61_focus::Leaf;
    using reader_selected::focus_diagnostic::observeJoinedLeaf;
    const auto allowed = [] { return true; };
    {
        Fixture f;
        const auto sample = [&] { return observeJoinedLeaf(&f.window, allowed, 1000); };
        QQuickItem child(f.current);
        QQuickItem sibling(f.device);
        QQuickItem outside(f.reader);
        f.current->forceActiveFocus();
        int finalPermit = 0;
        require(observeJoinedLeaf(&f.window, [&] { ++finalPermit; return true; }, 1000) == Leaf::ExactScene &&
                finalPermit > 1, "joined exact selected scene");
        child.forceActiveFocus();
        require(sample() == Leaf::InsideScene, "joined leaf inside selected scene");
        sibling.forceActiveFocus();
        require(sample() == Leaf::ScopeOnly, "joined leaf inside scope only");
        outside.forceActiveFocus();
        require(sample() == Leaf::OutsideScope, "joined leaf outside scope");
        f.current->forceActiveFocus();
        f.tiles.setPendingTiles(true);
        require(sample() == Leaf::Refused, "unready join refuses");
        f.tiles.setPendingTiles(false);

        const auto refuseAtFinalPermit = [&](const std::function<bool()> &change, const char *message) {
            int calls = 0;
            const auto receipt = observeJoinedLeaf(&f.window, [&] {
                if (++calls == finalPermit) return change();
                return true;
            }, 1000);
            require(calls == finalPermit && receipt == Leaf::Refused, message);
        };
        refuseAtFinalPermit([&] {
            sibling.forceActiveFocus();
            f.current->forceActiveFocus();
            return true;
        }, "focus away-and-back at final original permit refuses");
        refuseAtFinalPermit([&] {
            const QString page = f.current->pageId();
            f.current->setPageId(QStringLiteral("temporary"));
            f.current->setPageId(page);
            return true;
        }, "selected page away-and-back at final original permit refuses");
        refuseAtFinalPermit([] { return false; }, "cancel at final original permit refuses");
        refuseAtFinalPermit([&] {
            require(f.sample().refusal == QStringLiteral("sample-reentered"), "nested observation refuses");
            return true;
        }, "nested observation invalidates outer receipt");
        refuseAtFinalPermit([] {
            std::this_thread::sleep_for(std::chrono::milliseconds(1050));
            return true;
        }, "deadline through final original permit refuses");
        require(observeJoinedLeaf(&f.window, allowed, 0) == Leaf::Refused, "zero budget refuses");
        require(f.sample().ready && sample() == Leaf::ExactScene, "ordinary observation and subsequent receipt recover");
    }
    {
        Fixture f(true);
        f.current->forceActiveFocus();
        require(observeJoinedLeaf(&f.window, allowed, 1000) == Leaf::Refused, "wrong Reader lexical identity refuses");
    }
    QTextStream(stdout) << "PASS 13 joined Reader focus checks\n";
    return 0;
}
