#include "qt_page_facts_refusal.h"
#include <cstdio>
#include <cstring>
using namespace qml_access;
int main(int argc,char **argv) {
    int cases=0;
    FactsRefusalSample sample;
    sample.nonce=QStringLiteral("0123456789abcdef0123456789abcdef");
    sample.process=QStringLiteral("1234"); sample.processStart=QStringLiteral("5678");
    sample.acceptedMs=119999; sample.sampledMs=120010; sample.contextCurrent=true;
    sample.rootCurrent=true; sample.closureAbsent=true; sample.identityCurrent=true; sample.withinAcceptedDeadline=true;
    if (argc==2 && std::strcmp(argv[1],"--emit-refusal")==0) {
        sample.readerStage=QStringLiteral("facts-values-refused");
        std::puts(factsRefusalBytes(sample).constData()); return 0;
    }
    if (argc!=1) return 6;
    for (const char *stage:{"facts-reentry-refused","facts-config-or-context-refused",
         "open-engine-thread","open-current-window-unavailable","open-context-lost","open-item-lost",
         "open-topology-bound","open-candidate-bound","open-owner-ambiguous","open-owner-unavailable",
         "facts-metadata-or-context-refused","facts-helper-refused","facts-changed-or-context-refused",
         "facts-values-refused","facts-final-boundary-refused","facts-delivery-boundary-refused",
         "facts-observed-no-change-during-read"}) {
        sample.readerStage=QString::fromLatin1(stage);
        const auto bytes=factsRefusalBytes(sample); const auto json=QJsonDocument::fromJson(bytes).object();
        if (bytes.isEmpty() || bytes.size()>2048 || json.size()!=27 || json.value("reader_stage").toString()!=sample.readerStage ||
            json.value("accepted_read_elapsed_ms").toInteger()!=11 || json.value("refusal_path").toString()!=QStringLiteral("reader-result") ||
            !json.value("entry_context_current").isBool() || json.value("atomic_snapshot").toBool(true) ||
            json.value("native_authority").toBool(true) || json.value("render_authority").toBool(true)) return 1;
        ++cases;
    }
    sample.readerStage=QString(20000,QLatin1Char('x'))+QStringLiteral(" raw-error 0x12345678");
    const auto unknown=factsRefusalBytes(sample);
    if (unknown.contains("raw-error") || unknown.contains("0x12345678") ||
        QJsonDocument::fromJson(unknown).object().value("reader_stage").toString()!=QStringLiteral("unlisted-reader-stage")) return 2;
    ++cases;
    sample.readerHadFacts=true;
    if (QJsonDocument::fromJson(factsRefusalBytes(sample)).object().value("refusal_path").toString()!=QStringLiteral("entry-read-boundary")) return 3;
    ++cases;
    sample.completionBoundary=true; sample.withinAcceptedDeadline=false; sample.sampledMs=124999;
    if (QJsonDocument::fromJson(factsRefusalBytes(sample)).object().value("entry_stage").toString()!=QStringLiteral("facts-entry-delivery-refused")) return 4;
    ++cases;
    sample.nonce=QString(3000,QLatin1Char('a'));
    if (!factsRefusalBytes(sample).isEmpty()) return 5; // bounded output, no partial/retry.
    ++cases;
    std::printf("refusal-format: PASS %d allowlist/bounded/typed/path cases\n",cases);
    return 0;
}
