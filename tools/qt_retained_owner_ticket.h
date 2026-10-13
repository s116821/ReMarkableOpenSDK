#pragma once
#include "qt_page_owner.h"
#include <memory>

namespace qml_access {
class FactsEntry;
class PageFactsSession;
class RetainedOwnerFactoryKey final {
    friend class FactsEntry;
    RetainedOwnerFactoryKey() {}
    RetainedOwnerFactoryKey(const RetainedOwnerFactoryKey &) = delete;
    RetainedOwnerFactoryKey &operator=(const RetainedOwnerFactoryKey &) = delete;
public:
    ~RetainedOwnerFactoryKey() = default;
};
class RetainedOwnerRecord final {
    friend class FactsEntry;
    friend class RetainedOwnerTicket;
    enum class Boundary { EntryProgress, SessionOwner, DeliveryProgress, InvalidateOnly };
    using Bridge=bool (*)(RetainedOwnerRecord &, Boundary);
    RetainedOwnerRecord(const RetainedOwnerFactoryKey &, QPointer<QObject> entry,
        quint64 generation, PageOwner owner, QPointer<QQuickItem> root,
        QPointer<QQuickItem> anchor, quint64 epoch, Bridge bridge)
        : entry_(entry), generation_(generation), owner_(owner), root_(root),
          anchor_(anchor), captureEpoch_(epoch), bridge_(bridge) {}
    RetainedOwnerRecord(const RetainedOwnerRecord &) = delete;
    RetainedOwnerRecord &operator=(const RetainedOwnerRecord &) = delete;
    QPointer<QObject> entry_;
    const quint64 generation_;
    const PageOwner owner_;
    const QPointer<QQuickItem> root_,anchor_;
    const quint64 captureEpoch_;
    const Bridge bridge_;
    bool revoked_=false,validating_=false,issued_=false,transferred_=false;
public:
    ~RetainedOwnerRecord() = default;
};
class RetainedOwnerTicket final {
    friend class FactsEntry;
    friend class PageFactsSession;
    RetainedOwnerTicket(const RetainedOwnerFactoryKey &, std::weak_ptr<RetainedOwnerRecord> record,
        quint64 generation, PageOwner owner) : record_(std::move(record)),generation_(generation),owner_(owner) {}
    RetainedOwnerTicket(const RetainedOwnerTicket &) = delete;
    RetainedOwnerTicket &operator=(const RetainedOwnerTicket &) = delete;
    RetainedOwnerTicket(RetainedOwnerTicket &&) = delete;
    RetainedOwnerTicket &operator=(RetainedOwnerTicket &&) = delete;
    bool entryProgress() { return validate(RetainedOwnerRecord::Boundary::EntryProgress); }
    bool sessionOwner() { return validate(RetainedOwnerRecord::Boundary::SessionOwner); }
    bool deliveryProgress() { return validate(RetainedOwnerRecord::Boundary::DeliveryProgress); }
    void invalidate() {
        const auto record=record_.lock();
        if (!record) return;
        record->revoked_=true;
        if (record->bridge_) (void)record->bridge_(*record,RetainedOwnerRecord::Boundary::InvalidateOnly);
    }
    bool validate(RetainedOwnerRecord::Boundary boundary) {
        const auto record=record_.lock();
        if (!record || generation_!=record->generation_ || !record->entry_ || !record->bridge_ ||
            owner_.window!=record->owner_.window || owner_.receiver!=record->owner_.receiver ||
            owner_.scene!=record->owner_.scene || owner_.document!=record->owner_.document) return false;
        if (record->validating_) {
            record->revoked_=true;
            (void)record->bridge_(*record,RetainedOwnerRecord::Boundary::InvalidateOnly);
            return false;
        }
        if (record->revoked_ || !record->issued_ || !record->transferred_) return false;
        record->validating_=true;
        struct Unwind { RetainedOwnerRecord &record; ~Unwind(){record.validating_=false;} } unwind{*record};
        const bool valid=record->bridge_(*record,boundary);
        return valid && !record->revoked_ && record->entry_;
    }
    std::weak_ptr<RetainedOwnerRecord> record_;
    const quint64 generation_;
    const PageOwner owner_;
public:
    ~RetainedOwnerTicket() = default;
};
}
