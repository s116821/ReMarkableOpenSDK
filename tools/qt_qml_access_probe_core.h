/* Development experiment: metadata mode or explicitly configured one-call trial.
 * QML singleton resolution may invoke registration and change engine ownership.
 * Supported GUI lifecycle is assumed; weak guards do not pin native lifetimes. */
#pragma once
#include <QGuiApplication>
#include <QWindow>
#include <QQmlEngine>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQmlError>
#include <QQuickWindow>
#include <QQuickItem>
#include <QMetaProperty>
#include <QMetaMethod>
#include <QByteArrayView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QPointer>
#include <QTimer>
#include <QElapsedTimer>
#include <QThread>
#include "qt_qml_creation_bridge.h"
#include <functional>
#include <array>
#include <cstring>

namespace qml_access {
inline constexpr char helper[] =
    "import QtQml\nimport xofm.libs.library\n"
    "QtObject { property QtObject observedController: DocumentController; "
    "readonly property bool controllerAvailable: observedController !== null }";


struct CreationConfig {
    QString documentId;
    QStringList pageIds;
    bool enabled = false, developmentExplicitFixture = false;
    bool valid() const {
        static const QRegularExpression uuid(QStringLiteral("^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$"));
        if (!enabled || !uuid.match(documentId).hasMatch() || pageIds.size() != 5) return false;
        for (int i = 0; i < 5; ++i) {
            if (!uuid.match(pageIds[i]).hasMatch()) return false;
            for (int j = 0; j < i; ++j) if (pageIds[i] == pageIds[j]) return false;
        }
        return true;
    }
};
// Native QJSValue callbacks must be delivered on the originating engine thread.
// The C++ bridge guard cannot make foreign-thread QML/JS execution safe.
inline QByteArray creationHelper(const CreationConfig &config) {
    QJsonArray ids;
    for (const auto &id : config.pageIds) ids.append(id);
    const QByteArray document = QJsonDocument(QJsonArray{config.documentId}).toJson(QJsonDocument::Compact);
    const QByteArray pages = QJsonDocument(ids).toJson(QJsonDocument::Compact);
    return QByteArray("import QtQml\n") + (config.developmentExplicitFixture ? QByteArray() : QByteArray("import com.remarkable\n")) + QByteArray(R"QML(import xofm.libs.library
QtObject {
    id: ownedHelper
    property QtObject observedController: DocumentController
    readonly property bool controllerAvailable: observedController !== null
    property QtObject bridge: null
    property bool armed: true
    property bool entered: false
    property QtObject observedLibrary: null
    property bool waitingForLibrary: false
    property bool creationStarted: false
    property Connections libraryConnection: Connections {
        target: ownedHelper.waitingForLibrary && ownedHelper.armed ? ownedHelper.observedLibrary : null
        function onReadyChanged(ready) {
            if (!ownedHelper.armed || !ownedHelper.waitingForLibrary || !ownedHelper.bridge || !ownedHelper.bridge.callbackAllowed()) return;
            ownedHelper.bridge.observeProgress("ready_signal", "unobserved");
            ownedHelper.waitingForLibrary = false;
            ownedHelper.bridge.queueLibraryContinuation();
        }
    }
    readonly property bool developmentExplicitFixture: )QML" + QByteArray(config.developmentExplicitFixture ? "true" : "false") + R"QML(
    property int callbackCount: 0
    property bool callbackDuplicate: false
    function boundedError(text) {
        let value = text.slice(0,256);
        const last = value.charCodeAt(value.length-1);
        if (last >= 0xD800 && last <= 0xDBFF) value = value.slice(0,-1);
        return value;
    }
    function captureException(operation, e) {
        if (!armed || !bridge) return;
        if (!developmentExplicitFixture) return bridge.observeException(operation);
        let name = "", message = "", category = "unknown";
        try { const n = e.name; if (typeof n === "string") name = boundedError(n); } catch (ignored) {}
        try { const m = e.message; if (typeof m === "string") message = boundedError(m); } catch (ignored) {}
        if (name === "TypeError" || name === "ReferenceError" || name === "RangeError" || name === "SyntaxError" || name === "Error") category = name;
        bridge.observePrivateException(operation, category, name, message);
    }
    function createOnce() {
        if (entered) return;
        entered = true;
        if (!developmentExplicitFixture) return performCreation();
        if (armed && bridge) bridge.observeProgress("create_once_enter", "unobserved");
        let operation = "library-resolve";
        try {
            if (!armed || !bridge || !bridge.preflightAllowed()) return;
            bridge.observeProgress("library_resolve_enter", "unobserved");
            const lib = Library;
            bridge.observeProgress("library_resolve_return", "unobserved");
            observedLibrary = lib;
            if (!observedLibrary) return bridge.refuse("library-unavailable");
            operation = "library-method";
            bridge.observeProgress("library_method_enter", "unobserved");
            const method = observedLibrary.entryForId;
            bridge.observeProgress("library_method_return", "unobserved");
            if (typeof method !== "function") return bridge.refuse("library-method-unavailable");
            operation = "library-ready";
            bridge.observeProgress("library_ready_initial_enter", "unobserved");
            const ready = observedLibrary.isReady;
            bridge.observeProgress("library_ready_initial_return", typeof ready !== "boolean" ? "nonboolean" : ready ? "true" : "false");
            if (typeof ready !== "boolean") return bridge.refuse("library-readiness-invalid");
            if (!ready) { waitingForLibrary = true; return; }
            if (!bridge.admitLibraryReady()) return;
            performCreation();
        } catch (e) { captureException(operation, e); }
    }
    function resumeLibrary() {
        let operation = "library-ready";
        try {
            if (!armed || !bridge || !bridge.preflightAllowed() || creationStarted) return;
            bridge.observeProgress("library_ready_resume_enter", "unobserved");
            const ready = observedLibrary ? observedLibrary.isReady : undefined;
            bridge.observeProgress("library_ready_resume_return", typeof ready !== "boolean" ? "nonboolean" : ready ? "true" : "false");
            if (typeof ready !== "boolean") return bridge.refuse("library-readiness-invalid");
            if (!ready) return bridge.refuse("library-not-ready");
            if (!bridge.admitLibraryReady()) return;
            performCreation();
        } catch (e) { captureException(operation, e); }
    }
    function performCreation() {
        if (creationStarted) return;
        creationStarted = true;
        let operation = "preflight-check";
        try {
            if (!armed || !bridge || !bridge.preflightAllowed()) return;
            let docEnum, exportingEnum;
            if (!developmentExplicitFixture) {
                operation = "enum-document";
                docEnum = Entry.Document;
                operation = "enum-exporting";
                exportingEnum = Entry.Exporting;
                if (typeof docEnum !== "number" || typeof exportingEnum !== "number")
                    return bridge.refuse("enum-unavailable");
            }
            const expectedDocument = )QML") + document + R"QML([0];
            const expectedPages = )QML" + pages + R"QML(;
            operation = "library-lookup";
            if (developmentExplicitFixture) bridge.observeProgress("library_lookup_enter", "unobserved");
            const d = developmentExplicitFixture ? observedLibrary.entryForId(expectedDocument) : Library.entryForId(expectedDocument);
            if (developmentExplicitFixture) bridge.observeProgress("library_lookup_return", "unobserved");
            if (!d) return bridge.refuse("missing-document");
            operation = "document-id-read";
            const nativeId = d.id;
            operation = "document-id-string";
            if (nativeId === undefined || nativeId === null || String(nativeId) !== expectedDocument)
                return bridge.refuse("identity-mismatch");
            if (developmentExplicitFixture) {
                operation = "document-exporting-read";
                const exporting = d.isExporting;
                if (typeof exporting !== "boolean" || exporting !== false)
                    return bridge.refuse("exporting-refusal");
            } else {
                operation = "document-type-read";
                if (d.type !== docEnum) return bridge.refuse("not-document");
                operation = "document-status-read";
                const status = d.status;
                if (typeof status !== "number" || status === exportingEnum)
                    return bridge.refuse("status-refusal");
            }
            operation = "document-count-read";
            if (d.pageCount !== 5) return bridge.refuse("count-mismatch");
            for (let i = 0; i < 5; ++i) {
                operation = "page-id-read";
                const key = d.idForPage(i);
                if (typeof key !== "string" || key !== expectedPages[i])
                    return bridge.refuse("page-map-mismatch");
                operation = "page-index-read";
                if (d.pageForId(key) !== i) return bridge.refuse("page-map-mismatch");
            }
            operation = "template-read";
            const template = d.templateForPage(0);
            if (typeof template !== "string") return bridge.refuse("template-unavailable");
            operation = "paper-size";
            const paper = Qt.size(1404,1872);
            const self = ownedHelper;
            const callback = function() {
                if (!self || !self.armed || !self.bridge || !self.bridge.callbackAllowed()) return;
                if (self.callbackCount < 2) self.callbackCount += 1;
                if (self.callbackCount > 1) self.callbackDuplicate = true;
                self.bridge.observeCallback();
            };
            operation = "method-read";
            if (typeof observedController.addPageWithTemplateAndPageSize !== "function")
                return bridge.refuse("method-unavailable");
            operation = "mutation-claim";
            if (developmentExplicitFixture) bridge.observeProgress("mutation_claim_enter", "unobserved");
            if (!armed || !bridge || !bridge.claimMutation()) return;
            operation = "native-call";
            const result = observedController.addPageWithTemplateAndPageSize(nativeId,1,template,paper,callback);
            operation = "return-observe";
            if (armed && bridge) bridge.observeReturn(typeof result === "boolean",result === true);
        } catch(e) {
            captureException(operation, e);
        }
    }
}
)QML";
}
inline const char *creationExceptionOperation(const QString &operation) {
    static constexpr std::array<const char *, 21> allowed{{"library-resolve", "library-method", "library-ready", "preflight-check", "enum-document", "enum-exporting",
        "library-lookup", "document-id-read", "document-id-string", "document-type-read", "document-status-read",
        "document-count-read", "document-exporting-read", "page-id-read", "page-index-read", "template-read", "paper-size", "method-read",
        "mutation-claim", "native-call", "return-observe"}};
    for (const char *fixed : allowed) if (operation == QString::fromLatin1(fixed)) return fixed;
    return "exception-stage-unknown";
}
struct CreationObservation {
    bool enabled = false, attempted = false, returned = false, returnedBoolKnown = false, returnedBool = false;
    bool exception = false, duplicateCallback = false, developmentExplicitFixture = false;
    int callbackCount = 0;
    const char *phase = "not-started", *guard = "not-checked", *exceptionOperation = nullptr;
    QString privateErrorName, privateErrorMessage;
    const char *exceptionCategory = "unknown";
    std::array<qint64, 20> progressAtMs{{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}};
    const char *initialReadiness = "unobserved", *resumeReadiness = "unobserved", *currentReadiness = "unobserved";
    const char *lastEnteredStage = "unobserved", *lastCompletedStage = "unobserved";
    qint64 attemptedAtMs = -1, returnedAtMs = -1, firstCallbackAtMs = -1;
    qint64 libraryReadyAcceptedAtMs = -1;
};
inline constexpr std::array<const char *, 20> creationProgressStages{{"create_once_enter", "library_resolve_enter", "library_resolve_return",
    "library_method_enter", "library_method_return", "library_ready_initial_enter", "library_ready_initial_return", "ready_signal",
    "continuation_queued", "continuation_enter", "library_ready_resume_enter", "library_ready_resume_return", "library_lookup_enter", "library_lookup_return",
    "helper_create_enter", "helper_create_return", "mutation_claim_enter", "deadline_observed", "cleanup_enter", "cleanup_return"}};
inline QJsonObject creationJson(const CreationObservation &c) {
    const auto stamp = [](qint64 n) { return n < 0 ? QJsonValue(QJsonValue::Null) : QJsonValue(n); };
    QJsonObject result{{"enabled", c.enabled}, {"com_remarkable_import_selected", !c.developmentExplicitFixture}, {"development_explicit_fixture", c.developmentExplicitFixture}, {"mutation_attempted", c.attempted}, {"returned", c.returned},
        {"returned_bool_known", c.returnedBoolKnown}, {"returned_bool", c.returnedBool},
        {"exception", c.exception}, {"exception_operation", c.exceptionOperation ? QJsonValue(c.exceptionOperation) : QJsonValue(QJsonValue::Null)},
        {"exception_category", c.exception ? QJsonValue(c.exceptionCategory) : QJsonValue(QJsonValue::Null)},
        {"private_error_name", c.privateErrorName}, {"private_error_message", c.privateErrorMessage}, {"callback_count", c.callbackCount}, {"duplicate_callback", c.duplicateCallback},
        {"phase", c.phase}, {"guard_stage", c.guard}, {"attempted_at_ms", stamp(c.attemptedAtMs)},
        {"returned_at_ms", stamp(c.returnedAtMs)}, {"first_callback_at_ms", stamp(c.firstCallbackAtMs)},
        {"durable_success", false}, {"effect_requires_reconciliation", c.attempted}};
    if (c.developmentExplicitFixture) {
        result.insert("library_ready_accepted_at_ms", stamp(c.libraryReadyAcceptedAtMs));
        for (size_t i = 0; i < creationProgressStages.size(); ++i)
            result.insert(QString::fromLatin1(creationProgressStages[i]) + QStringLiteral("_ms"), stamp(c.progressAtMs[i]));
        result.insert("initial_readiness_observation", c.initialReadiness);
        result.insert("resume_readiness_observation", c.resumeReadiness);
        result.insert("current_readiness_observation", c.currentReadiness);
        result.insert("last_entered_stage", c.lastEnteredStage);
        result.insert("last_completed_stage", c.lastCompletedStage);
    }
    return result;
}

// English vendor-Qt diagnostics are hints, not a general parser or root cause.
// Qt owns/materializes the error list; these limits bound our inspection only.
inline const char *errorStage(const QList<QQmlError> &errors) {
    if (errors.isEmpty() || errors.size() > 8) return "component-error";
    const char *category = nullptr;
    for (const auto &error : errors) {
        const QString description = error.description();
        if (description.size() > 256) return "component-error";
        const char *current = nullptr;
        if (description.startsWith(QStringLiteral("module \"")) &&
            description.contains(QStringLiteral("\" version ")) &&
            description.endsWith(QStringLiteral(" is not installed"))) current = "module-version";
        else if (description.startsWith(QStringLiteral("module \"")) &&
                 description.endsWith(QStringLiteral("\" is not installed"))) {
            if (description == QStringLiteral("module \"QtQml\" is not installed")) current = "missing-qtqml";
            else if (description == QStringLiteral("module \"QML\" is not installed")) current = "missing-qml";
            else if (description == QStringLiteral("module \"QtQml.Models\" is not installed")) current = "missing-models";
            else if (description == QStringLiteral("module \"QtQml.WorkerScript\" is not installed")) current = "missing-worker";
            else if (description == QStringLiteral("module \"xofm.libs.library\" is not installed")) current = "missing-library";
            else current = "module-missing";
        }
        else if (description.size() > 14 && description.endsWith(QStringLiteral(" is not a type"))) current = "type-missing";
        else if (description == QStringLiteral("Invalid property assignment: unsupported type \"QObject*\"")) current = "property-type";
        else return "component-error";
        if (category && QByteArrayView(category) != QByteArrayView(current)) return "component-error";
        category = current;
    }
    return category;
}

// Private fixed-helper diagnostics only. Never serialize URLs or source text.
inline QByteArray diagnosticSnapshot(const QList<QQmlError> &errors, int attempt = 0) {
    QJsonArray retained;
    const qsizetype count = qMin<qsizetype>(errors.size(), 8);
    for (qsizetype i = 0; i < count; ++i) {
        const auto &error = errors.at(i);
        const QString description = error.description();
        QString retainedDescription = description.left(256);
        // Fixed helper has no user/document input. Keep system plugin paths
        // private, but suppress whole descriptions with personal path prefixes.
        static const QRegularExpression drive(QStringLiteral(R"([A-Za-z]:[\\/])"));
        const bool redacted = retainedDescription.contains(QStringLiteral("/home/"), Qt::CaseInsensitive) ||
            retainedDescription.contains(QStringLiteral("/root/"), Qt::CaseInsensitive) ||
            retainedDescription.contains(QStringLiteral("/Users/"), Qt::CaseInsensitive) ||
            retainedDescription.contains(QStringLiteral("\\Users\\"), Qt::CaseInsensitive) ||
            drive.match(retainedDescription).hasMatch();
        if (redacted) retainedDescription = QStringLiteral("[redacted personal path]");
        const bool truncated = description.size() > 256;
        retained.append(QJsonObject{
            {QStringLiteral("description"), retainedDescription.left(256)},
            {QStringLiteral("description_redacted"), redacted},
            {QStringLiteral("description_truncated"), truncated},
            {QStringLiteral("line"), error.line()}, {QStringLiteral("column"), error.column()}});
    }
    QJsonObject snapshot{
        {QStringLiteral("context"), QStringLiteral("last-compile-failure")},
        {QStringLiteral("failed_attempt"), attempt},
        {QStringLiteral("reported_error_count"), static_cast<qint64>(errors.size())},
        {QStringLiteral("retained_error_count"), static_cast<qint64>(count)},
        {QStringLiteral("count_truncated"), errors.size() > 8},
        {QStringLiteral("output_overflow"), false},
        {QStringLiteral("errors"), retained}};
    QByteArray bytes = QJsonDocument(snapshot).toJson(QJsonDocument::Compact);
    bytes.append('\n');
    if (bytes.size() > 8192) {
        // Reject the oversized serialization; emit a small explicit refusal.
        snapshot.insert(QStringLiteral("errors"), QJsonArray{});
        snapshot.insert(QStringLiteral("retained_error_count"), 0);
        snapshot.insert(QStringLiteral("count_truncated"), true);
        snapshot.insert(QStringLiteral("output_overflow"), true);
        bytes = QJsonDocument(snapshot).toJson(QJsonDocument::Compact) + '\n';
    }
    return bytes;
}

struct PropertyMetadata {
    int present = 0, typeMatch = 0, typeUnknown = 0, readable = 0, notify = 0;
};
struct MetadataBucket {
    int candidates = 0, complete = 0;
    std::array<PropertyMetadata, 3> properties{};
};
struct NavigationMetadata {
    int lookups = 0, candidates = 0, focusScope = 0, signalKind = 0, arityTwo = 0, variantPair = 0, complete = 0;
    int present = 0, readable = 0, notify = 0, pointer = 0, typeUnknown = 0, typeIncompatible = 0;
    int nameCompatible = 0, nameUnknown = 0;
    const char *location = "not-checked";
    bool located = false;
};
struct MetadataSummary {
    const char *result = "not-checked";
    int windows = 0, quickWindows = 0, items = 0;
    MetadataBucket scene, document, selection;
    NavigationMetadata navigation;
    bool signatureChecked = false, signaturePresent = false, signatureReturnsBool = false;
};
inline QJsonObject propertyMetadataJson(const PropertyMetadata &property, bool exactTypeKnown) {
    return {{"present", property.present}, {"type_match", property.typeMatch}, {"type_unknown", property.typeUnknown},
            {"readable", property.readable}, {"notify", property.notify}, {"exact_type_known", exactTypeKnown}};
}
inline QJsonObject metadataJson(const MetadataSummary &metadata) {
    const auto bucket = [](const MetadataBucket &b, const std::array<const char *, 3> &keys,
                           const std::array<bool, 3> &exact, int count) {
        QJsonObject properties;
        for (int i = 0; i < count; ++i) properties.insert(QString::fromLatin1(keys[i]), propertyMetadataJson(b.properties[i], exact[i]));
        return QJsonObject{{"candidates", b.candidates}, {"all_required_metadata", b.complete}, {"properties", properties}};
    };
    const auto &n = metadata.navigation;
    const QJsonObject navigation{{"lookups", n.lookups}, {"candidates", n.candidates}, {"focus_scope", n.focusScope},
        {"signal_kind", n.signalKind}, {"arity_two", n.arityTwo}, {"variant_pair", n.variantPair},
        {"all_required_metadata", n.complete}, {"location_result", QString::fromLatin1(n.location)}, {"located", n.located},
        {"window_navigator", QJsonObject{{"present", n.present}, {"readable", n.readable}, {"notify", n.notify},
            {"pointer_to_qobject", n.pointer}, {"type_unknown", n.typeUnknown}, {"type_incompatible", n.typeIncompatible},
            {"type_name_compatible", n.nameCompatible}, {"type_name_unknown", n.nameUnknown}, {"exact_type_known", false}}}};
    return {{"metadata_result", QString::fromLatin1(metadata.result)}, {"source_authority", false},
        {"receiver_relation", "unproven"}, {"navigation_candidate", navigation},
        {"owner_relation", "unproven"}, {"windows", metadata.windows}, {"quick_windows", metadata.quickWindows}, {"items", metadata.items},
        {"scene_view", bucket(metadata.scene, {"page_id", "document", "controller"}, {true, true, true}, 3)},
        {"document_view", bucket(metadata.document, {"scene_controller", "page_selection", "unused"}, {false, false, false}, 2)},
        {"selection_handler", bucket(metadata.selection, {"controller", "view_rect", "scene_rect"}, {false, true, true}, 3)},
        {"creation_signature", QJsonObject{{"checked", metadata.signatureChecked}, {"present", metadata.signaturePresent},
                                           {"returns_bool", metadata.signatureReturnsBool}}}};
}

// Existing metadata only; never read a native property or resolve/register a type.
inline bool observeProperty(const QMetaObject *meta, const char *name, const char *exactType, PropertyMetadata &counts) {
    const int index = meta->indexOfProperty(name);
    if (index < 0) return false;
    const QMetaProperty property = meta->property(index);
    ++counts.present;
    bool matches = false;
    if (exactType) {
        const char *type = property.typeName();
        if (!type) ++counts.typeUnknown;
        else matches = std::strcmp(type, exactType) == 0;
    } else {
        const auto type = property.metaType();
        if (!type.isValid()) ++counts.typeUnknown;
        else matches = type.flags().testFlag(QMetaType::PointerToQObject);
    }
    if (matches) ++counts.typeMatch;
    const bool readable = property.isReadable();
    if (readable) ++counts.readable;
    if (property.hasNotifySignal()) ++counts.notify;
    return matches && readable;
}
inline bool boundedMetaChain(const QMetaObject *meta) {
    for (int count = 0; meta; meta = meta->superClass()) if (++count > 32) return false;
    return true;
}

// Bounded canonical ASCII identifier chain only. This is a lexical diagnostic,
// not C++ type proof; a readable QObject pointer remains compatible independently.
inline bool navigatorTypeNameCompatible(const char *name) {
    if (!name) return false;
    int length = 0;
    while (length <= 128 && name[length]) ++length;
    if (length == 0 || length > 128 || name[length - 1] != '*') return false;
    const auto first = [](char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; };
    const auto next = [&](char c) { return first(c) || (c >= '0' && c <= '9'); };
    int position = 0, finalStart = 0;
    while (position < length - 1) {
        finalStart = position;
        if (!first(name[position++])) return false;
        while (position < length - 1 && next(name[position])) ++position;
        if (position == length - 1) break;
        if (position + 2 >= length - 1 || name[position] != ':' || name[position + 1] != ':') return false;
        position += 2;
    }
    constexpr char expected[] = "WindowNavigator";
    return length - 1 - finalStart == static_cast<int>(sizeof(expected) - 1) &&
        std::memcmp(name + finalStart, expected, sizeof(expected) - 1) == 0;
}

inline const char *observeNavigation(const QMetaObject *meta, bool focusScope, NavigationMetadata &n) {
    if (n.lookups >= 256) return "metadata-node-cap";
    ++n.lookups;
    const int index = meta->indexOfSignal("requestOpenDocumentOnPage(QVariant,QVariant)");
    if (index < 0) return nullptr;
    if (n.candidates >= 16) return "metadata-navigation-candidate-cap";
    ++n.candidates;
    if (focusScope) ++n.focusScope;
    const QMetaMethod signal = meta->method(index);
    const bool kind = signal.isValid() && signal.methodType() == QMetaMethod::Signal;
    const bool arity = signal.isValid() && signal.parameterCount() == 2;
    if (kind) ++n.signalKind;
    if (arity) ++n.arityTwo;
    bool variants = false;
    if (arity) {
        const QByteArray first = signal.parameterTypeName(0), second = signal.parameterTypeName(1);
        variants = first == "QVariant" && second == "QVariant";
    }
    if (variants) ++n.variantPair;
    bool propertyCompatible = false;
    const int propertyIndex = meta->indexOfProperty("windowNavigator");
    if (propertyIndex >= 0) {
        ++n.present;
        const QMetaProperty property = meta->property(propertyIndex);
        const bool readable = property.isReadable();
        if (readable) ++n.readable;
        if (property.hasNotifySignal()) ++n.notify;
        const auto type = property.metaType();
        bool pointer = false;
        if (!type.isValid()) ++n.typeUnknown;
        else {
            pointer = type.flags().testFlag(QMetaType::PointerToQObject);
            if (pointer) ++n.pointer; else ++n.typeIncompatible;
        }
        if (navigatorTypeNameCompatible(property.typeName())) ++n.nameCompatible; else ++n.nameUnknown;
        propertyCompatible = readable && pointer;
    }
    if (kind && arity && variants && focusScope && propertyCompatible) ++n.complete;
    return nullptr;
}

// One synchronous bounded scan, called only from an app-context queued turn.
// Public topology accessors may allocate; weak guards are not lifetime pins.
inline const char *observeMetadata(QGuiApplication *application, QQmlEngine *selectedEngine,
                                  QObject *resolvedController, const std::function<bool()> &withinBudget,
                                  MetadataSummary &summary) {
    const QPointer<QGuiApplication> app = application;
    const QPointer<QQmlEngine> engine = selectedEngine;
    const QPointer<QObject> controller = resolvedController;
    const auto context = [&]() -> const char * {
        if (!app || !engine) return "metadata-engine-lost";
        if (QThread::currentThread() != app->thread() || engine->thread() != app->thread()) return "metadata-thread";
        if (!withinBudget()) return "metadata-deadline";
        if (!controller) return "metadata-controller-lost";
        if (controller->thread() != app->thread()) return "metadata-thread";
        return nullptr;
    };
    struct Node { QPointer<QQuickItem> item; int depth = 0; };
    std::array<Node, 256> nodes{};
    std::array<QPointer<QWindow>, 16> trackedWindows{};
    int total = 0, processed = 0, windowCount = 0;
    const auto tracked = [&]() -> const char * {
        if (const char *failure = context()) return failure;
        for (int i = 0; i < windowCount; ++i) {
            if (!trackedWindows[i]) return "metadata-window-lost";
            if (trackedWindows[i]->thread() != app->thread()) return "metadata-thread";
        }
        for (int i = 0; i < total; ++i) {
            if (!nodes[i].item) return "metadata-item-lost";
            if (nodes[i].item->thread() != app->thread()) return "metadata-thread";
        }
        return nullptr;
    };
    const auto add = [&](QQuickItem *item, int depth) -> const char * {
        if (!item) return "metadata-item-lost";
        const QPointer<QQuickItem> weak = item;
        if (item->thread() != app->thread()) return "metadata-thread";
        for (int i = 0; i < total; ++i) if (nodes[i].item == weak) return "metadata-duplicate";
        if (total >= 256) return "metadata-node-cap";
        nodes[total++] = {weak, depth};
        summary.items = total;
        return nullptr;
    };
    if (const char *failure = context()) return failure;
    const auto windows = QGuiApplication::allWindows();
    if (windows.size() > 16) return "metadata-window-cap";
    // Capture all weak references before any virtual metadata accessor runs.
    windowCount = static_cast<int>(windows.size());
    summary.windows = windowCount;
    for (int i = 0; i < windowCount; ++i) trackedWindows[i] = windows[i];
    if (const char *failure = tracked()) return failure;
    for (int i = 0; i < windowCount; ++i) {
        if (const char *failure = tracked()) return failure;
        QWindow *window = trackedWindows[i];
        const QPointer<QQuickWindow> quick = qobject_cast<QQuickWindow *>(window);
        if (const char *failure = tracked()) return failure;
        QQmlEngine *owner = qmlEngine(window);
        if (const char *failure = tracked()) return failure;
        if (owner && owner != engine) return "metadata-engine-changed";
        if (!quick) continue;
        if (!owner || owner != engine) return "metadata-window-engine";
        ++summary.quickWindows;
        const QPointer<QQuickItem> root = quick->contentItem();
        if (const char *failure = tracked()) return failure;
        if (const char *failure = add(root, 0)) return failure;
    }
    while (processed < total) {
        if (const char *failure = tracked()) return failure;
        const Node node = nodes[processed++];
        const QMetaObject *meta = node.item->metaObject();
        if (const char *failure = tracked()) return failure;
        if (!meta) return "metadata-metaobject-missing";
        if (!boundedMetaChain(meta)) return "metadata-superclass-cap";
        bool scene = false, focusScope = false;
        for (const QMetaObject *base = meta; base; base = base->superClass())
        {
            if (std::strcmp(base->className(), "SceneView") == 0) scene = true;
            if (std::strcmp(base->className(), "QQuickFocusScope") == 0) focusScope = true;
        }
        const char *name = meta->className();
        const bool document = std::strstr(name, "DocumentView") && !std::strstr(name, "Shortcuts");
        const bool selection = std::strstr(name, "SceneSelectionHandler");
        if (const char *failure = tracked()) return failure;
        if (const char *failure = observeNavigation(meta, focusScope, summary.navigation)) return failure;
        if (const char *failure = tracked()) return failure;
        const auto inspect = [&](MetadataBucket &bucket, const std::array<const char *, 3> &names,
                                 const std::array<const char *, 3> &types, int count) -> const char * {
            if (bucket.candidates >= 16) return "metadata-candidate-cap";
            ++bucket.candidates;
            bool complete = true;
            for (int property = 0; property < count; ++property) {
                // Do not short-circuit: retain mismatches from this candidate.
                const bool matches = observeProperty(meta, names[property], types[property], bucket.properties[property]);
                complete = matches && complete;
            }
            if (complete) ++bucket.complete;
            return tracked();
        };
        if (scene) if (const char *failure = inspect(summary.scene, {"pageId", "document", "controller"},
                                                     {"QString", "QmlDocumentWrapper*", "SceneController*"}, 3)) return failure;
        if (document) if (const char *failure = inspect(summary.document, {"sceneController", "pageSelection", "unused"},
                                                        {nullptr, nullptr, nullptr}, 2)) return failure;
        if (selection) if (const char *failure = inspect(summary.selection, {"controller", "viewSelectionRect", "sceneSelectionRect"},
                                                         {nullptr, "QRectF", "QRectF"}, 3)) return failure;
        if (const char *failure = tracked()) return failure;
        const auto children = node.item->childItems();
        if (const char *failure = tracked()) return failure;
        if (node.depth >= 16 && !children.isEmpty()) return "metadata-depth-cap";
        if (children.size() > 256 - total) return "metadata-node-cap";
        // Convert the complete snapshot before inspecting any child metadata.
        std::array<QPointer<QQuickItem>, 256> weakChildren{};
        for (qsizetype i = 0; i < children.size(); ++i) weakChildren[i] = children[i];
        for (qsizetype i = 0; i < children.size(); ++i)
            if (const char *failure = add(weakChildren[i], node.depth + 1)) return failure;
    }
    if (const char *failure = tracked()) return failure;
    const QMetaObject *controllerMeta = controller->metaObject();
    if (const char *failure = tracked()) return failure;
    if (!controllerMeta) return "metadata-metaobject-missing";
    if (!boundedMetaChain(controllerMeta)) return "metadata-superclass-cap";
    summary.signatureChecked = true;
    const int method = controllerMeta->indexOfMethod("addPageWithTemplateAndPageSize(entry::Id,int,QString,QSizeF,QJSValue,QString)");
    summary.signaturePresent = method >= 0;
    if (method >= 0) {
        const char *type = controllerMeta->method(method).typeName();
        summary.signatureReturnsBool = type && std::strcmp(type, "bool") == 0;
    }
    if (const char *failure = tracked()) return failure;
    // Fresh final snapshot: no atomic topology or current-page claim.
    const auto finalWindows = QGuiApplication::allWindows();
    if (const char *failure = tracked()) return failure;
    if (finalWindows.size() > 16) return "metadata-window-cap";
    QPointer<QQmlEngine> finalEngine;
    std::array<QPointer<QWindow>, 16> finalWeak{};
    for (qsizetype i = 0; i < finalWindows.size(); ++i) finalWeak[i] = finalWindows[i];
    for (qsizetype i = 0; i < finalWindows.size(); ++i) {
        const QPointer<QWindow> weak = finalWeak[i];
        if (!weak) return "metadata-window-lost";
        if (weak->thread() != app->thread()) return "metadata-thread";
        QQmlEngine *candidate = qmlEngine(weak);
        if (candidate && finalEngine && candidate != finalEngine) return "metadata-multiple-engines";
        if (candidate) finalEngine = candidate;
        const QPointer<QQuickWindow> quick = qobject_cast<QQuickWindow *>(weak.data());
        if (const char *failure = tracked()) return failure;
        if (!weak) return "metadata-window-lost";
        if (quick && candidate != engine) return "metadata-window-engine";
    }
    if (finalEngine != engine) return "metadata-engine-changed";
    auto &navigation = summary.navigation;
    navigation.located = navigation.candidates == 1 && navigation.complete == 1;
    navigation.location = navigation.candidates == 0 ? "absent" : navigation.candidates > 1 ? "ambiguous" :
        navigation.located ? "located" : "incompatible";
    return "metadata-observed";
}

struct RootSummary {
    int readinessBudgetMs = 20000, accessBudgetMs = 5000, rootCount = 0;
    int retryIntervalMs = 200, retryRechecks = 0;
    const char *engineKind = "unselected", *gate = "waiting", *witnessKind = "none";
    qint64 witnessAtMs = -1, firstSetDataBeginMs = -1, firstSetDataReturnMs = -1, firstErrorHandledMs = -1;
    qint64 componentReadyAtMs = -1;
    qint64 lastSetDataBeginMs = -1, lastSetDataReturnMs = -1;
    MetadataSummary metadata;
    CreationObservation creation;
};

inline QByteArray runtimeSnapshot(const QByteArray &priorFailure, int attempts, int compiles,
                                  int events, const char *status, const char *stage, qint64 elapsedMs,
                                  const RootSummary &root = {}) {
    QJsonObject summary;
    if (priorFailure.isEmpty()) {
        summary = QJsonDocument::fromJson(diagnosticSnapshot({})).object();
        summary.insert(QStringLiteral("context"), QStringLiteral("runtime-only"));
    } else summary = QJsonDocument::fromJson(priorFailure).object();
    summary.insert(QStringLiteral("attempts"), qMin(attempts, 8));
    summary.insert(QStringLiteral("compile_attempts"), compiles);
    summary.insert(QStringLiteral("admitted_post_failure_events"), events);
    summary.insert(QStringLiteral("component_status"), QString::fromLatin1(status));
    summary.insert(QStringLiteral("terminal_stage"), QString::fromLatin1(stage));
    summary.insert(QStringLiteral("elapsed_ms"), elapsedMs);
    summary.insert(QStringLiteral("readiness_budget_ms"), root.readinessBudgetMs);
    summary.insert(QStringLiteral("access_budget_ms"), root.accessBudgetMs);
    summary.insert(QStringLiteral("gate_engine_kind"), QString::fromLatin1(root.engineKind));
    summary.insert(QStringLiteral("root_gate"), QString::fromLatin1(root.gate));
    summary.insert(QStringLiteral("root_witness_kind"), QString::fromLatin1(root.witnessKind));
    summary.insert(QStringLiteral("root_count"), root.rootCount);
    summary.insert(QStringLiteral("access_anchor"), root.creation.developmentExplicitFixture ? QStringLiteral("library-ready-observation") : QStringLiteral("component-ready-observation"));
    summary.insert(QStringLiteral("retry_interval_ms"), root.retryIntervalMs);
    summary.insert(QStringLiteral("timer_rechecks_admitted"), root.retryRechecks);
    const auto timestamp = [&](const char *key, qint64 value) {
        summary.insert(QString::fromLatin1(key), value < 0 ? QJsonValue(QJsonValue::Null) : QJsonValue(value));
    };
    timestamp("root_witness_at_ms", root.witnessAtMs);
    timestamp("component_ready_at_ms", root.componentReadyAtMs);
    timestamp("first_setData_begin_ms", root.firstSetDataBeginMs);
    timestamp("first_setData_return_ms", root.firstSetDataReturnMs);
    timestamp("first_error_handled_ms", root.firstErrorHandledMs);
    timestamp("last_setData_begin_ms", root.lastSetDataBeginMs);
    timestamp("last_setData_return_ms", root.lastSetDataReturnMs);
    summary.insert(QStringLiteral("metadata"), metadataJson(root.metadata));
    if (root.creation.enabled) summary.insert(QStringLiteral("creation_trial"), creationJson(root.creation));
    QByteArray bytes = QJsonDocument(summary).toJson(QJsonDocument::Compact) + '\n';
    if (bytes.size() > 8192) {
        summary.insert(QStringLiteral("errors"), QJsonArray{});
        summary.insert(QStringLiteral("retained_error_count"), 0);
        summary.insert(QStringLiteral("count_truncated"), true);
        summary.insert(QStringLiteral("output_overflow"), true);
        bytes = QJsonDocument(summary).toJson(QJsonDocument::Compact) + '\n';
    }
    return bytes;
}

class Probe final : public QObject {
public:
    using Receipt = std::function<void(const char *, bool, bool, bool, bool, const QByteArray &)>;
    Probe(QGuiApplication *app, Receipt receipt, int readinessMs = 20000, int accessMs = 5000, CreationConfig creation = {})
        : QObject(app), app_(app), receipt_(std::move(receipt)), creationConfig_(std::move(creation)) {
        rootSummary_.creation.enabled = creationConfig_.enabled;
        rootSummary_.creation.developmentExplicitFixture = creationConfig_.developmentExplicitFixture;
        rootSummary_.readinessBudgetMs = readinessMs;
        rootSummary_.accessBudgetMs = accessMs;
        timer_.setSingleShot(true);
        timer_.setTimerType(Qt::PreciseTimer);
        connect(&timer_, &QTimer::timeout, this, [this] { (void)expired(); });
        connect(app, &QCoreApplication::aboutToQuit, this, [this] { cancel(); });
        elapsed_.start();
        timer_.start(readinessMs);
        app->installEventFilter(this);
        queueAttempt();
    }
    ~Probe() override {
        if (bridge_) bridge_->disarm();
        if (app_) app_->removeEventFilter(this);
        delete owned_.data();
    }
protected:
    bool eventFilter(QObject *object, QEvent *event) override {
        if (!done_ && !component_ && !engineSelected_ && qobject_cast<QWindow *>(object) &&
            (event->type() == QEvent::Show || event->type() == QEvent::Expose ||
             event->type() == QEvent::FocusIn || event->type() == QEvent::FocusOut))
            queueAttempt();
        return false;
    }
private:
    void recordCreationProgress(const QString &stage, const QString &readiness = QStringLiteral("unobserved")) {
        if (!creationConfig_.developmentExplicitFixture || QThread::currentThread() != thread()) return;
        auto &trial = rootSummary_.creation;
        for (size_t i = 0; i < creationProgressStages.size(); ++i) {
            const char *fixed = creationProgressStages[i];
            if (stage != QString::fromLatin1(fixed)) continue;
            if (trial.progressAtMs[i] >= 0) return;
            trial.progressAtMs[i] = elapsed_.elapsed();
            if (stage.endsWith(QStringLiteral("_enter"))) trial.lastEnteredStage = fixed;
            if (stage.endsWith(QStringLiteral("_return"))) trial.lastCompletedStage = fixed;
            if (i == 6 || i == 11) {
                const char *value = "unobserved";
                for (const char *candidate : {"nonboolean", "false", "true"})
                    if (readiness == QString::fromLatin1(candidate)) value = candidate;
                if (i == 6) trial.initialReadiness = value;
                else trial.resumeReadiness = value;
                trial.currentReadiness = value;
            }
            return;
        }
    }
    qint64 deadlineAtMs() const {
        if (creationConfig_.developmentExplicitFixture)
            return rootSummary_.creation.libraryReadyAcceptedAtMs < 0 ? rootSummary_.readinessBudgetMs :
                rootSummary_.creation.libraryReadyAcceptedAtMs + rootSummary_.accessBudgetMs;
        return rootSummary_.componentReadyAtMs < 0 ? rootSummary_.readinessBudgetMs :
            rootSummary_.componentReadyAtMs + rootSummary_.accessBudgetMs;
    }
    bool expired() {
        if (elapsed_.elapsed() < deadlineAtMs()) return false;
        recordCreationProgress(QStringLiteral("deadline_observed"));
        finish(rootSummary_.componentReadyAtMs >= 0 ? "deadline" :
            rootSummary_.witnessAtMs < 0 ? "root-readiness-deadline" : "readiness-deadline");
        return true;
    }
    void disconnectRootGate() {
        QObject::disconnect(rootConnection_);
        rootConnection_ = {};
    }
    void queueRootTransition(const char *failure = nullptr) {
        if (done_ || rootTransitionQueued_) return;
        rootTransitionQueued_ = true;
        const QPointer<QQmlEngine> producer = engine_;
        const unsigned epoch = eventEpoch_;
        QMetaObject::invokeMethod(this, [this, producer, epoch, failure] {
            if (done_ || epoch != eventEpoch_) return;
            rootTransitionQueued_ = false;
            if (!producer || producer != engine_) { finish("engine-lost"); return; }
            if (expired()) return;
            if (failure) { rootSummary_.gate = "failed"; finish(failure); return; }
            // Continue the already admitted acquisition, with fresh checks.
            attempt(true);
        }, Qt::QueuedConnection);
    }
    void acceptRootWitness(QObject *object, const char *kind) {
        if (done_ || rootTransitionQueued_ || rootSummary_.witnessAtMs >= 0) return;
        // Same-thread signal handler only captures evidence; never compiles or
        // finishes inside native load(). Queuing is not a native-stack pin.
        if (!app_ || !applicationEngine_ || applicationEngine_ != engine_ ||
            applicationEngine_->thread() != app_->thread()) {
            queueRootTransition("engine-thread"); return;
        }
        if (elapsed_.elapsed() >= rootSummary_.readinessBudgetMs) {
            queueRootTransition("root-readiness-deadline"); return;
        }
        if (!object) { queueRootTransition("root-load-failed"); return; }
        const QPointer<QObject> witness = object;
        const qint64 witnessedAt = elapsed_.elapsed();
        if (witnessedAt >= rootSummary_.readinessBudgetMs) {
            queueRootTransition("root-readiness-deadline"); return;
        }
        rootWitness_ = witness;
        rootSummary_.witnessKind = kind;
        rootSummary_.witnessAtMs = witnessedAt;
        rootSummary_.gate = "accepted";
        queueRootTransition();
    }
    void queueAttempt() {
        if (queued_ || done_) return;
        queued_ = true;
        const unsigned epoch = eventEpoch_;
        QMetaObject::invokeMethod(this, [this, epoch] {
            if (epoch != eventEpoch_) return;
            queued_ = false;
            attempt();
        }, Qt::QueuedConnection);
    }
    void armRetry() {
        if (done_ || expired()) return;
        if (attempts_ >= 8 || compileAttempts_ >= 8) { finish("readiness-cap"); return; }
        if (retryArmed_) return;
        retryArmed_ = true;
        const unsigned epoch = eventEpoch_;
        const QPointer<QQmlEngine> producer = engine_;
        QTimer::singleShot(rootSummary_.retryIntervalMs, Qt::PreciseTimer, this, [this, epoch, producer] {
            if (done_ || epoch != eventEpoch_ || !retryArmed_) return;
            retryArmed_ = false;
            if (component_ || inCall_ || !waitingAfterFailure_ || expired()) return;
            if (!producer || producer != engine_) { finish("engine-lost"); return; }
            ++rootSummary_.retryRechecks;
            attempt(); // One fresh admission; no elapsed-time catch-up burst.
        });
    }
    void queueComponentReady() {
        const QPointer<QQmlComponent> current = component_;
        QMetaObject::invokeMethod(this, [this, current] {
            if (current && component_ == current) componentReady();
        }, Qt::QueuedConnection);
    }
    bool windowEngine(QPointer<QQmlEngine> &selected) {
        const auto windows = QGuiApplication::allWindows();
        if (windows.size() > 16) { finish("window-cap"); return false; }
        for (QWindow *window : windows) {
            QQmlEngine *candidate = qmlEngine(window);
            if (!candidate) continue;
            if (selected && selected != candidate) { finish("multiple-engines"); return false; }
            selected = candidate;
        }
        return true;
    }
    bool engineContext() {
        appThread_ = app_ && QThread::currentThread() == app_->thread();
        if (!appThread_) { finish("application-thread"); return false; }
        if (!engine_) { finish("engine-lost"); return false; }
        engineThread_ = engine_->thread() == app_->thread();
        if (!engineThread_) { finish("engine-thread"); return false; }
        return true;
    }
    void attempt(bool rootContinuation = false) {
        if (done_ || component_ || expired()) return;
        appThread_ = app_ && QThread::currentThread() == app_->thread();
        if (!appThread_) { finish("application-thread"); return; }
        if (!rootContinuation && ++attempts_ > 8) { finish("readiness-cap"); return; }
        QPointer<QQmlEngine> selected;
        if (!windowEngine(selected)) return;
        if (engineSelected_ && (!engine_ || !selected || selected != engine_)) { finish("engine-changed"); return; }
        if (!selected) return; // Only readiness events or the refusal deadline follow.
        const bool firstEngine = !engineSelected_;
        engineSelected_ = true;
        engine_ = selected;
        engineThread_ = engine_->thread() == app_->thread();
        if (!engineThread_) { finish("engine-thread"); return; }
        if (firstEngine) connect(engine_, &QObject::destroyed, this, [this] {
                QMetaObject::invokeMethod(this, [this] { finish("engine-lost"); }, Qt::QueuedConnection);
            });
        if (firstEngine) {
            applicationEngine_ = qobject_cast<QQmlApplicationEngine *>(engine_.data());
            rootSummary_.engineKind = applicationEngine_ ? "application" : "unsupported";
            if (!applicationEngine_) { rootSummary_.gate = "failed"; finish("unsupported-engine"); return; }
            const QPointer<QQmlEngine> producer = engine_;
            QThread *const expectedThread = app_->thread();
            const unsigned gateEpoch = eventEpoch_;
            rootConnection_ = connect(applicationEngine_, &QQmlApplicationEngine::objectCreated, this,
                [this, producer, expectedThread, gateEpoch](QObject *object, const QUrl &) {
                    // Never queue a raw signal pointer. Unexpected cross-thread
                    // emission queues only refusal, without touching GUI state.
                    if (QThread::currentThread() != expectedThread) {
                        QMetaObject::invokeMethod(this, [this, producer, gateEpoch] {
                            if (!done_ && gateEpoch == eventEpoch_ && producer && producer == engine_ && !gateConsumed_)
                                finish("engine-thread");
                        }, Qt::QueuedConnection);
                        return;
                    }
                    if (producer && producer == engine_) acceptRootWitness(object, "signal");
                }, Qt::DirectConnection);
            const auto roots = applicationEngine_->rootObjects();
            rootSummary_.rootCount = static_cast<int>(qMin<qsizetype>(roots.size(), 17));
            if (roots.size() > 16) { rootSummary_.gate = "failed"; finish("root-cap"); return; }
            if (!roots.isEmpty()) acceptRootWitness(roots.first(), "preexisting");
            return;
        }
        if (!gateConsumed_) {
            if (!rootContinuation || rootSummary_.witnessAtMs < 0) return;
            if (!applicationEngine_ || applicationEngine_ != engine_ ||
                applicationEngine_->thread() != app_->thread()) { finish("engine-lost"); return; }
            const auto roots = applicationEngine_->rootObjects();
            rootSummary_.rootCount = static_cast<int>(qMin<qsizetype>(roots.size(), 17));
            if (roots.size() > 16) { rootSummary_.gate = "failed"; finish("root-cap"); return; }
            if (!rootWitness_ || !roots.contains(rootWitness_.data())) {
                rootSummary_.gate = "failed"; finish("root-witness-lost"); return;
            }
            gateConsumed_ = true;
            rootSummary_.gate = "consumed";
            disconnectRootGate();
        }
        if (expired()) return; // Root-list copying is native work, not budget-free.
        component_ = new QQmlComponent(engine_, this);
        waitingAfterFailure_ = false;
        const QPointer<QQmlComponent> producer = component_;
        connect(component_, &QQmlComponent::statusChanged, this, [this, producer] {
            if (producer && component_ == producer) queueComponentReady();
        });
        if (expired()) return; // Allocation/connection do not reset the budget.
        inCall_ = true;
        rootSummary_.lastSetDataBeginMs = elapsed_.elapsed();
        if (rootSummary_.firstSetDataBeginMs < 0) rootSummary_.firstSetDataBeginMs = rootSummary_.lastSetDataBeginMs;
        ++compileAttempts_;
        component_->setData(creationConfig_.enabled ? creationHelper(creationConfig_) : QByteArray(helper), QUrl());
        rootSummary_.lastSetDataReturnMs = elapsed_.elapsed();
        if (rootSummary_.firstSetDataReturnMs < 0) rootSummary_.firstSetDataReturnMs = rootSummary_.lastSetDataReturnMs;
        inCall_ = false;
        if (settlePending()) return;
        // Never create/delete inside setData or its synchronous status callback.
        queueComponentReady();
    }
    void componentReady() {
        if (done_ || !component_ || inCall_ || expired() || created_) return;
        if (!engineContext()) return;
        if (component_->status() == QQmlComponent::Loading) return;
        if (component_->status() != QQmlComponent::Ready) {
            if (rootSummary_.firstErrorHandledMs < 0) rootSummary_.firstErrorHandledMs = elapsed_.elapsed();
            const auto errors = component_->errors();
            // Creation source embeds private IDs: never retain compiler text.
            diagnostic_ = diagnosticSnapshot(creationConfig_.enabled ? QList<QQmlError>{} : errors, attempts_);
            const char *stage = errorStage(errors);
            if (QByteArrayView(stage) == QByteArrayView("missing-library")) {
                // This failed component owns no helper. Remove it before any
                // one context-bound recheck timer can admit a fresh compilation.
                QQmlComponent *failed = component_;
                // Keep the pointer nonnull during deletion to ignore reentrant
                // window events; weak callback guards invalidate on deletion.
                inCall_ = true;
                delete failed;
                component_ = nullptr;
                inCall_ = false;
                ++eventEpoch_;
                queued_ = false; // Older queued events cannot authorize retry.
                waitingAfterFailure_ = true;
                if (settlePending()) return;
                if (!engine_) { finish("engine-lost"); return; }
                armRetry(); // Absolute readiness deadline remains fixed.
            } else finish(stage);
            return;
        }
        // Accept Ready only when observed here, before the absolute readiness
        // deadline. This is not the earlier status-signal emission timestamp.
        const qint64 readyAt = elapsed_.elapsed();
        if (readyAt >= rootSummary_.readinessBudgetMs) { finish("readiness-deadline"); return; }
        rootSummary_.componentReadyAtMs = readyAt;
        timer_.start(static_cast<int>(qMax<qint64>(deadlineAtMs() - elapsed_.elapsed(), 0)));
        // Delayed queued Ready must still belong to the current window engine.
        // This is a safety snapshot within the same admission/access budget.
        QPointer<QQmlEngine> selected;
        if (!windowEngine(selected) || !engineContext()) return;
        if (!selected || selected != engine_) { finish("engine-changed"); return; }
        if (expired()) return;
        created_ = true;
        inCall_ = true;
        recordCreationProgress(QStringLiteral("helper_create_enter"));
        owned_ = component_->create();
        recordCreationProgress(QStringLiteral("helper_create_return"));
        inCall_ = false;
        if (settlePending()) return;
        if (!engine_) { finish("engine-lost"); return; }
        if (expired()) return;
        if (!owned_ || component_->isError()) { finish("create-error"); return; }
        helperFound_ = true;
        inCall_ = true;
        const QVariant available = owned_->property("controllerAvailable");
        inCall_ = false;
        if (settlePending()) return;
        if (!engine_) { finish("engine-lost"); return; }
        if (expired()) return;
        if (available.metaType() != QMetaType::fromType<bool>()) { finish("create-error"); return; }
        controller_ = available.toBool();
        if (!controller_) { finish("unavailable"); return; }
        // Exactly one read of our helper's typed property. Never read a native
        // controller/view property or convert an arbitrary QVariant type.
        inCall_ = true;
        const QVariant controllerValue = owned_->property("observedController");
        inCall_ = false;
        if (settlePending()) return;
        if (expired() || !engineContext()) return;
        if (controllerValue.metaType() != QMetaType::fromType<QObject *>()) {
            finish("metadata-controller-type"); return;
        }
        observedController_ = *static_cast<QObject *const *>(controllerValue.constData());
        if (creationConfig_.enabled) { queueCreation(); return; }
        const QPointer<QQmlEngine> producer = engine_;
        const unsigned epoch = eventEpoch_;
        QMetaObject::invokeMethod(this, [this, producer, epoch] {
            if (done_ || epoch != eventEpoch_) return;
            if (!producer || producer != engine_) { finish("metadata-engine-lost"); return; }
            if (expired() || !engineContext()) return;
            inCall_ = true;
            const char *result = observeMetadata(app_, engine_, observedController_,
                [this] { return !cancelPending_ && !pendingStage_ && elapsed_.elapsed() < deadlineAtMs(); }, rootSummary_.metadata);
            rootSummary_.metadata.result = result;
            inCall_ = false;
            if (settlePending()) return;
            finish(QByteArrayView(result) == QByteArrayView("metadata-observed") ? "resolved" : result);
        }, Qt::QueuedConnection);
    }

    bool creationContext(bool claim) {
        auto &trial = rootSummary_.creation;
        if (done_ || cancelPending_ || pendingStage_ || !bridge_ || !creationConfig_.valid()) {
            if (!done_ && !pendingStage_) finish("creation-context");
            return false;
        }
        if (!engineContext() || expired()) return false;
        if (!owned_ || !observedController_) { finish("creation-object-lost"); return false; }
        if (owned_->thread() != app_->thread() || observedController_->thread() != app_->thread() ||
            qmlEngine(owned_) != engine_) { finish("creation-affinity"); return false; }
        QPointer<QQmlEngine> fresh;
        if (!windowEngine(fresh)) return false;
        if (!fresh || fresh != engine_) { finish("engine-changed"); return false; }
        if (done_ || cancelPending_ || pendingStage_ || expired() || !owned_ || !observedController_) return false;
        if (claim) {
            if (trial.attempted) return false;
            trial.attempted = true;
            trial.attemptedAtMs = elapsed_.elapsed();
            trial.phase = "effect-uncertain";
            trial.guard = "accepted";
        }
        return true;
    }
    void queueCreation() {
        const QPointer<Probe> self(this);
        const QPointer<QQmlEngine> producer = engine_;
        const unsigned epoch = eventEpoch_;
        QMetaObject::invokeMethod(this, [self, producer, epoch] {
            if (!self || self->done_ || epoch != self->eventEpoch_) return;
            if (!producer || producer != self->engine_) { self->finish("engine-lost"); return; }
            if (!self->creationConfig_.valid()) { self->finish("creation-config"); return; }
            self->bridge_ = new CreationBridge(self);
            auto *bridge = self->bridge_;
            bridge->check = [self](bool claim) { return self && self->creationContext(claim); };
            bridge->libraryAdmission = [self] {
                if (!self || !self->creationConfig_.developmentExplicitFixture || !self->creationContext(false)) return false;
                auto &trial = self->rootSummary_.creation;
                if (trial.libraryReadyAcceptedAtMs >= 0) return true;
                const qint64 acceptedAt = self->elapsed_.elapsed();
                if (acceptedAt >= self->rootSummary_.readinessBudgetMs) { self->expired(); return false; }
                trial.libraryReadyAcceptedAtMs = acceptedAt;
                self->timer_.start(self->rootSummary_.accessBudgetMs);
                return true;
            };
            bridge->progress = [self](const QString &stage, const QString &readiness) {
                if (self) self->recordCreationProgress(stage, readiness);
            };
            bridge->refusal = [self](const QString &stage) {
                if (!self || self->rootSummary_.creation.attempted) return;
                // Only allow fixed stages, never arbitrary native/user text.
                static const std::array<const char *, 14> allowed{{"library-unavailable", "library-method-unavailable", "library-readiness-invalid", "library-not-ready", "enum-unavailable", "missing-document", "identity-mismatch",
                    "not-document", "status-refusal", "count-mismatch", "page-map-mismatch", "template-unavailable", "method-unavailable", "exporting-refusal"}};
                const char *fixed = "guard-refusal";
                for (const char *s : allowed) if (stage == QString::fromLatin1(s)) fixed = s;
                self->rootSummary_.creation.guard = fixed;
                self->rootSummary_.creation.phase = "pre-call-refusal";
                self->finish("creation-refused");
            };
            bridge->returned = [self](bool known, bool value) {
                if (!self || !self->rootSummary_.creation.attempted) return;
                auto &trial = self->rootSummary_.creation;
                trial.returned = true; trial.returnedBoolKnown = known; trial.returnedBool = known && value;
                trial.returnedAtMs = self->elapsed_.elapsed();
            };
            bridge->exception = [self](const QString &operation) {
                if (!self) return;
                auto &trial = self->rootSummary_.creation;
                trial.exception = true;
                trial.exceptionOperation = creationExceptionOperation(operation);
                trial.phase = trial.attempted ? "effect-uncertain" : "pre-call-exception";
                self->finish("creation-exception");
            };
            bridge->privateException = [self](const QString &operation, const QString &category, const QString &name, const QString &message) {
                if (!self || !self->creationConfig_.developmentExplicitFixture) return;
                auto &trial = self->rootSummary_.creation;
                static const std::array<const char *, 5> categories{{"TypeError", "ReferenceError", "RangeError", "SyntaxError", "Error"}};
                for (const char *fixed : categories) if (category == QString::fromLatin1(fixed)) trial.exceptionCategory = fixed;
                trial.privateErrorName = name.left(256); trial.privateErrorMessage = message.left(256);
                trial.exception = true;
                trial.exceptionOperation = creationExceptionOperation(operation);
                trial.phase = trial.attempted ? "effect-uncertain" : "pre-call-exception";
                self->finish("creation-exception");
            };
            bridge->libraryContinuation = [self] {
                if (!self || self->done_ || self->libraryContinuationQueued_) return;
                self->libraryContinuationQueued_ = true;
                self->recordCreationProgress(QStringLiteral("continuation_queued"));
                const unsigned epoch = self->eventEpoch_;
                const bool posted = QMetaObject::invokeMethod(self, [self, epoch] {
                    if (!self || self->done_ || epoch != self->eventEpoch_) return;
                    self->recordCreationProgress(QStringLiteral("continuation_enter"));
                    if (!self->creationContext(false)) return;
                    self->inCall_ = true;
                    const bool invoked = QMetaObject::invokeMethod(self->owned_, "resumeLibrary", Qt::DirectConnection);
                    self->inCall_ = false;
                    if (self->settlePending()) return;
                    if (!invoked) { self->finish("creation-dispatch"); return; }
                    self->completeCreation();
                }, Qt::QueuedConnection);
                if (!posted) {
                    self->bridge_->disarm();
                    if (!self->pendingStage_ && !self->cancelPending_) {
                        self->rootSummary_.creation.guard = "library-dispatch-unavailable";
                        self->rootSummary_.creation.phase = "pre-call-refusal";
                        self->pendingStage_ = "creation-refused";
                    }
                    self->timer_.stop();
                    // Terminal-only defer: never destroy a helper on its native signal stack.
                    QTimer::singleShot(0, self, [self] { if (self && !self->done_) self->settlePending(); });
                }
            };
            bridge->callback = [self] {
                if (!self || !self->rootSummary_.creation.attempted) return;
                auto &trial = self->rootSummary_.creation;
                if (trial.callbackCount < 2) ++trial.callbackCount;
                trial.duplicateCallback = trial.callbackCount > 1;
                if (trial.firstCallbackAtMs < 0) trial.firstCallbackAtMs = self->elapsed_.elapsed();
                // Terminal processing is queued: never tear down on native stack.
                if (!self->creationCompletionQueued_) {
                    self->creationCompletionQueued_ = true;
                    if (!QMetaObject::invokeMethod(self, [self] {
                        if (!self || self->done_) return;
                        self->creationCompletionQueued_ = false;
                        self->completeCreation();
                    }, Qt::QueuedConnection)) self->creationCompletionQueued_ = false;
                }
            };
            QQmlEngine::setObjectOwnership(bridge, QQmlEngine::CppOwnership);
            if (!self->engineContext() || self->expired() || !self->owned_) return;
            self->inCall_ = true;
            const bool wired = self->owned_->setProperty("bridge", QVariant::fromValue<QObject *>(bridge));
            self->inCall_ = false;
            if (self->settlePending()) return;
            if (!wired || !self->creationContext(false)) { if (!self->done_) self->finish("creation-context"); return; }
            self->inCall_ = true;
            const bool invoked = QMetaObject::invokeMethod(self->owned_, "createOnce", Qt::DirectConnection);
            self->inCall_ = false;
            if (self->settlePending()) return;
            if (!invoked) { self->finish("creation-dispatch"); return; }
            self->completeCreation();
        }, Qt::QueuedConnection);
    }
    void completeCreation() {
        if (done_ || inCall_ || expired()) return;
        if (!creationContext(false)) return;
        auto &trial = rootSummary_.creation;
        if (trial.callbackCount > 0 && trial.returned) {
            trial.phase = "call-observed";
            finish("resolved");
        }
        // Return without callback, including false, waits for the same deadline.
    }

    void cancel() {
        if (done_) return;
        if (bridge_) bridge_->disarm();
        if (creationConfig_.enabled && !finishing_) { finish("creation-cancelled"); return; }
        disconnectRootGate();
        if (inCall_) {
            cancelPending_ = true;
            timer_.stop();
            if (app_) app_->removeEventFilter(this);
            return; // No deferred deletion can run inside a nested Qt loop.
        }
        done_ = true;
        retryArmed_ = false;
        ++eventEpoch_;
        timer_.stop();
        if (app_) app_->removeEventFilter(this);
        // App-context destruction cancels queued transitions. Only owned objects.
        delete owned_.data();
        owned_.clear();
        delete component_;
        component_ = nullptr;
        deleteLater();
    }
    void finish(const char *stage) {
        if (done_) return;
        if (bridge_) bridge_->disarm();
        if (inCall_) {
            if (!pendingStage_) pendingStage_ = stage;
            timer_.stop();
            if (app_) app_->removeEventFilter(this);
            return;
        }
        const char *status = "absent";
        if (component_) {
            switch (component_->status()) {
            case QQmlComponent::Null: status = "null"; break;
            case QQmlComponent::Loading: status = "loading"; break;
            case QQmlComponent::Ready: status = "ready"; break;
            case QQmlComponent::Error: status = "error"; break;
            }
        }
        finishing_ = true;
        if (rootSummary_.creation.enabled && rootSummary_.creation.attempted && QByteArrayView(stage) != QByteArrayView("resolved"))
            rootSummary_.creation.phase = "effect-uncertain";
        recordCreationProgress(QStringLiteral("cleanup_enter"));
        cancel();
        recordCreationProgress(QStringLiteral("cleanup_return"));
        // Cleanup can reenter Qt. Terminal success follows cleanup and another
        // weak guard/deadline check; neither check is a lifetime pin.
        if (controller_ && !engine_) { stage = "engine-lost"; controller_ = false; }
        const qint64 elapsedMs = elapsed_.elapsed();
        if (controller_ && elapsedMs >= deadlineAtMs()) {
            recordCreationProgress(QStringLiteral("deadline_observed"));
            stage = "deadline"; controller_ = false;
        }
        if (rootSummary_.creation.enabled && rootSummary_.creation.attempted && QByteArrayView(stage) != QByteArrayView("resolved"))
            rootSummary_.creation.phase = "effect-uncertain";
        if (QByteArrayView(stage) != QByteArrayView("resolved")) {
            rootSummary_.metadata.result = stage;
            rootSummary_.metadata.navigation.location = "not-checked";
            rootSummary_.metadata.navigation.located = false;
        }
        diagnostic_ = runtimeSnapshot(diagnostic_, attempts_, compileAttempts_, admittedPostFailureEvents_, status, stage, elapsedMs, rootSummary_);
        receipt_(stage, appThread_, engineThread_, helperFound_, controller_, diagnostic_);
    }
    bool settlePending() {
        if (cancelPending_) { cancel(); return true; }
        if (pendingStage_) { const char *stage = pendingStage_; pendingStage_ = nullptr; finish(stage); return true; }
        return done_;
    }
    QPointer<QGuiApplication> app_;
    Receipt receipt_;
    QTimer timer_;
    QElapsedTimer elapsed_;
    int attempts_ = 0;
    RootSummary rootSummary_;
    bool gateConsumed_ = false, rootTransitionQueued_ = false;
    QMetaObject::Connection rootConnection_;
    QPointer<QQmlApplicationEngine> applicationEngine_;
    QPointer<QObject> rootWitness_;
    int compileAttempts_ = 0, admittedPostFailureEvents_ = 0;
    bool waitingAfterFailure_ = false, retryArmed_ = false;
    unsigned eventEpoch_ = 0;
    bool engineSelected_ = false;
    bool queued_ = false, done_ = false, created_ = false, inCall_ = false;
    bool cancelPending_ = false;
    const char *pendingStage_ = nullptr;
    bool appThread_ = false, engineThread_ = false;
    bool helperFound_ = false, controller_ = false;
    QPointer<QQmlEngine> engine_;
    QQmlComponent *component_ = nullptr;
    QPointer<QObject> owned_;
    QPointer<QObject> observedController_;
    QByteArray diagnostic_;
    CreationConfig creationConfig_;
    CreationBridge *bridge_ = nullptr;
    bool libraryContinuationQueued_ = false;
    bool finishing_ = false, creationCompletionQueued_ = false;
};
}
