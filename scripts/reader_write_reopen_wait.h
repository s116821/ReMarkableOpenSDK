#pragma once
// Bounded diagnostic history only. This type grants no readiness or action authority.
#include <QString>
#include <utility>
namespace reader_write_reopen {
class StartupWait final {
public:
    void note(QString reason) {
        reason=reason.left(160);
        if(reason==last_)return;
        previous_=std::move(last_);last_=std::move(reason);
    }
    void beginComposition() {
        // Repeated inspection must not replace a useful refusal with pending.
        if(last_==QStringLiteral("not-polled")||last_==QStringLiteral("holder-not-ready"))
            note(QStringLiteral("composition-not-sampled"));
    }
    QString terminal(const char *why)const {
        return QString::fromLatin1(why)+QStringLiteral("|last_wait=")+last_+
            QStringLiteral("|previous_wait=")+previous_;
    }
private:
    QString last_=QStringLiteral("not-polled"),previous_=QStringLiteral("none");
};
}

