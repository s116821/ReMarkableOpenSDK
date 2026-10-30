#pragma once
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

namespace qml_access {
// Only fixed existing reader/owner labels may enter private refusal evidence.
// No helper error, native value, address or arbitrary result string is retained.
inline QString factsRefusalReaderStage(const QString &stage,bool focusAncestry=false) {
    if(focusAncestry && stage==QStringLiteral("facts-retained-owner-refused"))return stage;
    for (const char *allowed:{"facts-reentry-refused","facts-config-or-context-refused",
         "open-engine-thread","open-current-window-unavailable","open-context-lost","open-item-lost",
         "open-topology-bound","open-candidate-bound","open-owner-ambiguous","open-owner-unavailable",
         "facts-metadata-or-context-refused","facts-helper-refused","facts-changed-or-context-refused",
         "facts-values-refused","facts-final-boundary-refused","facts-delivery-boundary-refused",
         "facts-observed-no-change-during-read"})
        if (stage==QLatin1String(allowed)) return stage;
    return QStringLiteral("unlisted-reader-stage");
}
struct FactsRefusalSample {
    QString nonce, process, processStart, readerStage;
    bool readerHadFacts=false, completionBoundary=false;
    qint64 sampledMs=-1, acceptedMs=-1;
    bool contextCurrent=false, rootCurrent=false, closureAbsent=false, identityCurrent=false;
    bool withinAcceptedDeadline=false;
    bool focusAncestry=false;
};
inline QByteArray factsRefusalBytes(const FactsRefusalSample &sample) {
    if(sample.focusAncestry && sample.readerStage==QStringLiteral("facts-retained-owner-refused") &&
        (sample.readerHadFacts || sample.completionBoundary))return {};
    // The entry supplies its already validated attempt identity. This format is
    // intentionally distinct from the unchanged successful 23-field facts schema.
    const QJsonObject json{{"kind","development-facts-refusal"},{"version",1},
        {"nonce",sample.nonce},{"attempt_pid",sample.process},{"attempt_start",sample.processStart},
        {"reader_stage",factsRefusalReaderStage(sample.readerStage,sample.focusAncestry)},
        {"reader_result_had_facts",sample.readerHadFacts},
        {"entry_stage",sample.completionBoundary ? "facts-entry-delivery-refused" : "facts-entry-read-refused"},
        {"refusal_path",sample.completionBoundary ? "entry-completion-boundary" :
            sample.readerHadFacts ? "entry-read-boundary" : "reader-result"},
        {"sample_phase","before-refusal-callback"},{"sample_ms",sample.sampledMs},
        {"request_accepted_ms",sample.acceptedMs},
        {"accepted_read_elapsed_ms",sample.acceptedMs<0 ? qint64(-1) : sample.sampledMs-sample.acceptedMs},
        {"setup_clock_origin","entry-startup-monotonic"},{"read_clock_origin","accepted-request-monotonic"},
        {"setup_budget_ms",120000},{"read_budget_ms",5000},{"setup_selection","main-dev-facts-120s"},
        {"development_setup_opt_in",true},{"entry_context_current",sample.contextCurrent},
        {"root_identity_current",sample.rootCurrent},{"closure_absent",sample.closureAbsent},
        {"attempt_identity_current",sample.identityCurrent},{"within_accepted_deadline",sample.withinAcceptedDeadline},
        {"atomic_snapshot",false},{"native_authority",false},{"render_authority",false}};
    const QByteArray bytes=QJsonDocument(json).toJson(QJsonDocument::Compact);
    return bytes.size()<=2048 ? bytes : QByteArray{};
}
}
