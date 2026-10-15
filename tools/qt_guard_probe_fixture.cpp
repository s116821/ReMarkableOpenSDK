/* Owned host objects only. Run with the separate host-built payload preloaded. */
#include "qt_guard_probe_core.h"
#include <QtQml/qqml.h>
#include <QCoreApplication>
#include <QTimer>
#include <cassert>
#include <fstream>
#include <memory>
#include <string>
#include <unistd.h>

struct OwnedObject : QObject {
    const QPointer<QObject> *witness = nullptr;
    bool *seenDuringDerivedDestruction = nullptr;
    ~OwnedObject() override {
        if (witness) *seenDuringDerivedDestruction = !witness->isNull();
    }
};

int main(int argc, char **argv) {
    assert(argc == 2);
    const std::string mode = argv[1];
    const char *path = "/run/rmb-qt-probe-" QT_PROBE_NONCE "/callback.json";
    assert(access(path, F_OK) != 0);
    QCoreApplication app(argc, argv);
    auto first = std::make_unique<OwnedObject>();
    auto second = std::make_unique<QObject>();
    int factoryCalls = 0;
    int id = -1;
    int duplicate = -1;
    const char *module = QT_GUARD_MODULE;
    const char *name = QT_GUARD_NAME;
    if (mode == "wrong-meta") {
        id = qmlRegisterSingletonInstance(module, 1, 0, name, &app);
    } else if (mode == "factory") {
        id = qmlRegisterSingletonType<QObject>(module, 1, 0, name,
            [&factoryCalls](QQmlEngine *, QJSEngine *) -> QObject * { ++factoryCalls; return nullptr; });
    } else if (mode != "missing") {
        id = qmlRegisterSingletonInstance(module, 1, mode == "wrong-version" ? 1 : 0, name, first.get());
        if (mode == "duplicate")
            duplicate = qmlRegisterSingletonInstance(module, 1, 0, name, second.get());
        if (mode == "deleted") first.reset();
    }
    if (mode == "retention") {
        auto candidate = guard_probe::lookup(QString::fromLatin1(module), QString::fromLatin1(name),
            QTypeRevision::fromVersion(1, 0), &QObject::staticMetaObject);
        assert(candidate.registrationMatch && candidate.typedTargetMatch && !candidate.guard.isNull());
        QQmlMetaType::unregisterType(id);
        assert(!QQmlMetaType::qmlTypeById(id).isValid());
        candidate.type = QQmlType(); // Info alone must still own the capture.
        const auto *target = candidate.info->qobjectCallback.target<QQmlPrivate::SingletonInstanceFunctor>();
        assert(target && !target->m_object.isNull());
        bool seenDuringDerivedDestruction = false;
        first->witness = &candidate.guard;
        first->seenDuringDerivedDestruction = &seenDuringDerivedDestruction;
        first.reset();
        assert(seenDuringDerivedDestruction); // Noncleared is NOT a lifetime pin.
        assert(target->m_object.isNull() && candidate.guard.isNull());
        candidate.info = {}; // Copied guard must survive its original capture.
        assert(candidate.guard.isNull());
        return 0;
    }
    if (mode == "duplicate") {
        const auto candidate = guard_probe::lookup(QString::fromLatin1(module), QString::fromLatin1(name),
            QTypeRevision::fromVersion(1, 0), &QObject::staticMetaObject);
        assert(candidate.type.index() == id || candidate.type.index() == duplicate);
        assert(id != duplicate); // Valid candidate does NOT establish uniqueness.
    }
    assert(access(path, F_OK) != 0); // Queued, never a synchronous startup receipt.
    QTimer::singleShot(20, &app, &QCoreApplication::quit);
    assert(app.exec() == 0);
    assert(factoryCalls == 0);
    std::ifstream input(path);
    const std::string output((std::istreambuf_iterator<char>(input)), {});
    const bool registration = mode != "missing" && mode != "wrong-version" && mode != "wrong-meta";
    const bool target = registration && mode != "factory" && mode != "rtti-mismatch";
    const bool guard = target && mode != "deleted";
    const std::string expected = "{\"nonce\":\"" QT_PROBE_NONCE "\",\"application_thread\":true,\"registration_match\":" +
        std::string(registration ? "true" : "false") + ",\"typed_target_match\":" +
        (target ? "true" : "false") + ",\"guard_not_cleared\":" +
        (guard ? "true" : "false") + ",\"uniqueness\":\"unproven\"}\n";
    assert(output == expected && output.size() < 256);
    if (id >= 0) QQmlMetaType::unregisterType(id);
    if (duplicate >= 0) QQmlMetaType::unregisterType(duplicate);
}
