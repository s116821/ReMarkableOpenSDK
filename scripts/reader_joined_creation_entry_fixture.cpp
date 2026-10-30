// Owned phase orchestration only; creation is an explicit test model.
#define READER_ENTRY_OWNED_HELPERS_ONLY
#include "reader_selected_view_entry_fixture.cpp"

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    qmlRegisterType<QObject>("OwnedProbe", 1, 0, "QtObject");
    qmlRegisterType<QQuickItem>("OwnedProbe", 1, 0, "Item");
    qmlRegisterType<FocusFixture>("OwnedProbe", 1, 0, "FocusScope");
    qmlRegisterType<SceneView>("OwnedProbe", 1, 0, "SceneFixture");
    qmlRegisterType<OwnershipOwner>("OwnedLegacyEntry", 1, 0, "Owner");
    registerCreationFixture();
    const QByteArray nonce(32, 'c'), identity = reader_selected::selfIdentity();
    for (const QByteArray &mode : {QByteArray("valid"), QByteArray("already-target"), QByteArray("before-cancel"),
            QByteArray("wrong-initial"), QByteArray("order-wrong"), QByteArray("source-moved"),
            QByteArray("cancel-create"), QByteArray("expire-create"), QByteArray("budget-create"),
            QByteArray("false-effect"), QByteArray("unknown-effect"), QByteArray("no-callback"),
            QByteArray("cancel-before-next"), QByteArray("deadline-before-next"), QByteArray("cancel-after-next"),
            QByteArray("missing-next"), QByteArray("no-next-effect"), QByteArray("same-engine-window"), QByteArray("postlink-cancel")}) {
        QByteArray extra = "property int forwardCalls: 0\n";
        if (mode != "missing-next") extra += QByteArray("function moveForward() { forwardCalls += 1;") +
            (mode == "no-next-effect" ? QByteArray("return;") : QByteArray("currentPage += 1; currentPageId = document.idForPage(currentPage); externalController.pageId = currentPageId;")) + "}\n";
        Fixture f(false, false, extra);
        f.document.pages = QStringList{QStringLiteral("20000000-0000-0000-0000-000000000001"), QStringLiteral("20000000-0000-0000-0000-000000000002")};
        f.reader->setProperty("currentPageId", f.document.pages[0]); f.controller.setPageId(f.document.pages[0]);
        Library library; DocumentController creation; creation.setLibrary(&library); library.result = &f.document;
        f.engine.rootContext()->setContextProperty("externalCreation", &creation);
        xofm::bindings::qt::WindowNavigatorWrapper navigator;
        xofm::bindings::qt::WindowLayout layout; layout.activeValue = QStringLiteral("legacydevice/window/main");
        f.engine.rootContext()->setContextProperty("entryNavigator", &navigator);
        f.engine.rootContext()->setContextProperty("entryLayout", &layout);
        QQmlComponent composition(&f.engine);
        composition.setData(R"QML(import OwnedLegacyEntry 1.0
Owner { id: root; width: 100; height: 100; windowNavigator: entryNavigator; windowLayout: entryLayout; contentItem: mainView
    Owner { id: mainView; width: 100; height: 100; windowNavigator: root.windowNavigator; windowLayout: root.windowLayout }
})QML", QUrl(QStringLiteral("qrc:/qt/qml/xofm/modules/legacydevice/qml/LegacyMainWindow.qml")));
        require(composition.isReady(), "joined creation owned composition compiles");
        std::unique_ptr<QObject> root(composition.create()); auto *visual = qobject_cast<QQuickItem *>(root.get());
        require(visual, "joined creation owned composition creates");
        visual->setParentItem(f.window.contentItem()); visual->setParent(f.window.contentItem());
        f.reader->setVisible(false);
        reader_joined_creation::Request request;
        request.documentId = f.document.key; request.expectedPages = f.document.pages; request.sourceIndex = 0;
        request.sourcePageId = f.document.pages[0]; request.targetPageId = QStringLiteral("30000000-0000-0000-0000-000000000001");
        require(request.valid(), "joined creation owned request valid");
        QTemporaryDir base; const QString path = base.path() + "/root";
        require(base.isValid() && QDir().mkdir(path) && ::chmod(path.toLocal8Bit().constData(), 0700) == 0, "joined creation private root");
        putEntry(path + "/owner", nonce); putEntry(path + "/attempt.identity", identity);
        reader_selected::Entry entry(&app, path, nonce, nullptr,
            reader_selected::Entry::Diagnostic::NavigateJoinedCreation, {request.documentId, 0, true}, request);
        std::unique_ptr<QQuickWindow> otherWindow;
        entry.creationWirePaddingForTest(QString(700, QLatin1Char('p'))); // Explicit owned-only cap regression; forbidden in startup builds.
        int submissions = 0;
        entry.creationSubmitForTest([&](QObject *controller, QObject *document, const QVariant &id,
                const reader_selected::Observation &before, const reader_joined_creation::Request &selected, const std::function<bool()> &guard) {
            reader_joined_creation::Result result; result.finished = true;
            require(entry.samplingForTest() && readEntry(path + "/selected-result").isEmpty(), "no joined creation result on sample stack");
            require(controller == &creation && document == &f.document && id.isValid() && guard(), "actual qualified owned objects and guard");
            if (before.pageId != selected.sourcePageId || before.pageIndex != selected.sourceIndex || f.document.pages != selected.expectedPages) {
                result.refusal = QStringLiteral("modeled-preflight-refused"); return result;
            }
            ++submissions;
            result.attempted = true; result.returnKnown = true; result.accepted = mode != "false-effect";
            result.unknown = mode == "unknown-effect"; result.callbackObserved = mode != "no-callback";
            result.callbackCount = result.callbackObserved ? 1 : 0;
            f.document.pages.insert(1, selected.targetPageId); emit f.document.pageCountChanged(); emit f.document.pageMapChanged();
            if (mode == "order-wrong") f.document.pages.swapItemsAt(1, 2);
            if (mode == "already-target" || mode == "source-moved") {
                const int index = mode == "already-target" ? 1 : 2;
                f.reader->setProperty("currentPage", index); f.reader->setProperty("currentPageId", f.document.pages[index]);
                f.controller.setPageId(f.document.pages[index]);
            }
            if (mode == "cancel-create") { putEntry(path + "/selected-cancel", "stop"); entry.checkForTest(); pumpEntry(); }
            if (mode == "expire-create") entry.expireForTest();
            if (mode == "budget-create") entry.advanceForTest(30001);
            require(readEntry(path + "/selected-result").isEmpty(), "creation unwind still precedes publication");
            return result;
        });
        navigator.onOpen = [&] {
            f.reader->setVisible(true);
            if (mode == "wrong-initial") { f.reader->setProperty("currentPage", 1); f.reader->setProperty("currentPageId", f.document.pages[1]); f.controller.setPageId(f.document.pages[1]); }
        };
        if (mode == "postlink-cancel") entry.postLinkForTest([&] { putEntry(path + "/selected-cancel", "stop"); });
        require(entry.start(), "joined creation Entry starts"); pumpEntry();
        require(submissions == 0 && navigator.calls == 0, "no mutation/navigation before request");
        if (mode == "before-cancel") putEntry(path + "/selected-cancel", "stop");
        putEntry(path + "/selected-request", nonce + ' ' + identity); entry.checkForTest(); pumpEntry();
        if (entry.navigationPendingForTest()) entry.afterNavigationForTest();
        if (entry.creationPendingForTest()) {
            if (mode == "same-engine-window") {
                otherWindow = std::make_unique<QQuickWindow>();
                QQmlEngine::setContextForObject(otherWindow.get(), f.engine.rootContext());
                otherWindow->show(); otherWindow->requestActivate(); pumpEntry();
                require(QGuiApplication::focusWindow() == otherWindow.get(), "owned same-engine focus replacement");
            }
            if (mode == "cancel-before-next") { putEntry(path + "/selected-cancel", "stop"); entry.checkForTest(); pumpEntry(); }
            if (mode == "deadline-before-next") entry.advanceForTest(30001);
            entry.afterCreationForTest();
        }
        if (entry.creationPendingForTest()) {
            if (mode == "cancel-after-next") { putEntry(path + "/selected-cancel", "stop"); entry.checkForTest(); pumpEntry(); }
            entry.afterCreationForTest();
        }
        pumpEntry(); require(entry.terminal(), "joined creation one-shot reaches terminal");
        const auto bytes = readEntry(path + "/selected-result");
        if (mode == "same-engine-window") {
            validReceipt(bytes, nonce, identity, "creation-refused", 4096);
            require(f.reader->property("forwardCalls").toInt() == 0, "replacement window always refuses with zero Next");
        }
        const bool success = mode == "valid" || mode == "already-target" || mode == "postlink-cancel";
        if (success) {
            if (QJsonDocument::fromJson(bytes).object()["stage"] != "creation-selected-observed")
                QTextStream(stderr) << "joined-mode=" << mode << " wire=" << bytes << '\n';
            validReceipt(bytes, nonce, identity, "creation-selected-observed", 4096);
            require(bytes.size() > 2048, "actual larger joined creation receipt publishes above old cap");
            const auto json = QJsonDocument::fromJson(bytes).object();
            require(json["observed"].toBool() && json["ready"].toBool() && json["creation_join_observed"].toBool() &&
                json["page_id"] == request.targetPageId && json["page_index"] == 1 && json["page_count"] == 3 &&
                json["creation_order_observed"].toBool() && json["effect_requires_reconciliation"].toBool(), "target facts and reconciliation stay explicit");
            const bool already = mode == "already-target";
            require(json["creation_target_already_selected"].toBool() == already &&
                json["creation_next_attempted"].toBool() == !already && json["creation_next_dispatched"].toBool() == !already &&
                f.reader->property("forwardCalls").toInt() == (already ? 0 : 1), "at most one adjacent Next, none if target selected");
            struct stat result{}; require(::stat((path + "/selected-result").toLocal8Bit().constData(), &result) == 0 &&
                result.st_nlink == (mode == "postlink-cancel" ? 2U : 1U), "uncertain post-link publication remains two-linked");
        } else if (!bytes.isEmpty()) {
            const auto json = QJsonDocument::fromJson(bytes).object();
            require(json["stage"] != "creation-selected-observed" && !json["observed"].toBool(), "failure never claims selected created target");
            if (mode == "same-engine-window") require(json["creation_refusal"] == "creation-context-changed" &&
                !json["creation_next_attempted"].toBool(), "same engine cannot replace pinned window across creation phases");
        }
        const int expected = mode == "before-cancel" || mode == "wrong-initial" ? 0 : 1;
        require(submissions == expected && f.reader->property("forwardCalls").toInt() <= 1, "one creation and at most one Next");
        const auto saved = bytes; entry.checkForTest(); entry.afterCreationForTest(); pumpEntry();
        require(readEntry(path + "/selected-result") == saved && submissions == expected && !entry.start(), "no replay or late publication");
    }
    QTextStream(stdout) << "PASS owned joined creation Entry: qualified source, ordinary task model, preserved order/adjacency, already-target skip, one Next, fresh target, original budget/cancel/uncertain publication; no native authority\n";
}
