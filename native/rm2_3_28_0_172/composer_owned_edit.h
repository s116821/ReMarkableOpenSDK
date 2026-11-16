#pragma once
// Private transaction engine. No public Scene/range/currentness admission.
#include "composer_native_insert.h"
#include "composer_plain_edit.h"
#include <QLoggingCategory>
#include <QCoreApplication>
#include <QPointer>
#include <thread>

namespace remarkable::composer_writer {
class Issuer;
enum class EditStatus { Refused, ProjectionVerified, Unknown };
enum class EditFailure {
    None, Thread, Context, Canceled, Deadline, Plain, Incarnation, Anchors,
    Revision, Text, Empty, Preparation, Submission, Settlement, Release, Exception
};
struct EditReceipt {
    EditStatus status = EditStatus::Refused;
    EditFailure failure = EditFailure::None;
    detail::ActionReceipt action;
    int beforeRevision = 0, afterRevision = 0;
    bool revisionsPresent = false;
    // ProjectionVerified is intentionally not a durable/native finalization
    // receipt. The worker's post-callback Scene step and job must still settle.
};
class OwnedEdit final {
    using Clock = composer_text::Clock;
    using Guard = composer_text::detail::Guard;
    using Join = composer_scene::Join;
    using Text = composer_text::Result;
    struct Range final {
        // Accessed only on the original native worker. A weak document witness
        // detects root destruction/reallocation; saved addresses alone cannot.
        QPointer<QTextDocument> document;
        std::uintptr_t root = 0, control = 0, participant = 0, privateData = 0;
        detail::NativeId left, right;
        std::shared_ptr<std::atomic<bool>> visitRevoked;
        explicit Range(const Join &joined, detail::NativeId l, detail::NativeId r,
                std::shared_ptr<std::atomic<bool>> visit)
            : document(joined.document), root(joined.root), control(joined.control),
              participant(joined.participant), privateData(joined.privateData),
              left(l), right(r), visitRevoked(std::move(visit)) {}
    };
    struct Provisioned { EditReceipt receipt; std::shared_ptr<Range> range; };
    struct ObservedDraft {
        bool observed=false;
        EditFailure failure=EditFailure::None;
        int revision=0;
        QString text;
        // A copied observation of this borrow, never an edit/currentness lease.
    };
    const std::thread::id gui_;
    const std::uintptr_t worker_;
    std::unique_ptr<detail::ActionProfile> remove_;
    std::unique_ptr<detail::InsertProfile> insert_;
    std::unique_ptr<detail::PlainProfile> plain_;
    Join (*join_)(::Scene *) = composer_scene::detail::join;
    std::uintptr_t (*context_)(::Scene *, const Join &) = nativeContext;
    detail::AnchorPositions (*anchors_)(std::uintptr_t, detail::NativeId, detail::NativeId,
        const Text &, const Guard &) = [](std::uintptr_t root, detail::NativeId left,
            detail::NativeId right,const Text &text,const Guard &guard) {
                return detail::walkAnchors(root,left,right,text,guard);
            };
    detail::AnchorPositions (*sequence_)(std::uintptr_t,const Text &,const Guard &) =
        [](std::uintptr_t root,const Text &text,const Guard &guard) {
            return detail::inspectSequence(root,text,guard);
        };
    friend class Issuer;
#ifdef COMPOSER_WRITER_TESTING
    friend struct testing::Access;
#endif
    OwnedEdit(std::uintptr_t worker, std::unique_ptr<detail::ActionProfile> remove,
            std::unique_ptr<detail::InsertProfile> insert, std::unique_ptr<detail::PlainProfile> plain,
            std::thread::id gui = std::this_thread::get_id())
        : gui_(gui), worker_(worker), remove_(std::move(remove)),
          insert_(std::move(insert)), plain_(std::move(plain)) {}
    static std::unique_ptr<OwnedEdit> prepare(std::uintptr_t pinnedWorker) {
        auto *app = QCoreApplication::instance();
        if (!pinnedWorker || !app || QThread::currentThread()!=app->thread()) return {};
        auto remove = detail::ActionProfile::load();
        auto insert = detail::InsertProfile::load();
        auto plain = detail::PlainProfile::load();
        if (!remove || !insert || !plain) return {};
        return std::unique_ptr<OwnedEdit>(new OwnedEdit(pinnedWorker,std::move(remove),std::move(insert),std::move(plain)));
    }
    static std::uintptr_t nativeContext(::Scene *borrowed, const Join &joined) {
#if defined(__linux__) && defined(__arm__) && defined(__ARM_PCS_VFP) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
        using composer_scene::detail::word;
        const auto scene = reinterpret_cast<std::uintptr_t>(borrowed);
        if (!scene || joined.refusal != composer_scene::Refusal::None ||
            word(scene,0x190) != joined.root || word(scene,0x194) != joined.control ||
            word(scene,0x26c) != joined.participant || word(joined.root,0x1e8) != scene)
            return 0;
        const auto stack = word(scene,0x260);
        if (!stack || word(stack,8) != scene) return 0;
        // aac1f0's exact fixed-size null-page-ID test. Do not manufacture an ID.
        unsigned char page[16];
        std::memcpy(page,reinterpret_cast<const void *>(scene + 0x19a),sizeof page);
        bool nonzero = false;
        for (const auto byte : page) nonzero = nonzero || byte;
        if (!nonzero) return 0;
        unsigned char bypass = 0;
        std::memcpy(&bypass,reinterpret_cast<const void *>(0x1524a48),1);
        if (bypass) return 0;
        // Refuse the lazy/category-filter and debug-message callout paths;
        // never initialize/change logging from the native write callback.
        // Both SceneActionStack and the nested CRDT first-effect routines have
        // their own category. Admission must not skip the latter's callout.
        for (const std::uintptr_t guard : {std::uintptr_t(0x1524a4c),std::uintptr_t(0x1525134),
                std::uintptr_t(0x1525120),std::uintptr_t(0x152510c)}) {
            if (!(__atomic_load_n(reinterpret_cast<const std::uint32_t *>(guard),__ATOMIC_ACQUIRE) & 1U))return 0;
            const auto *category = reinterpret_cast<const QLoggingCategory *>(guard+4);
            if(category->isDebugEnabled())return 0;
        }
        return stack;
#else
        (void)borrowed; (void)joined;
        return 0;
#endif
    }
    bool thread() const noexcept {
        return std::this_thread::get_id() != gui_ && worker_ &&
            reinterpret_cast<std::uintptr_t>(QThread::currentThread()) == worker_;
    }
    static EditFailure interrupted(const Guard &guard) noexcept {
        const auto why = guard.check();
        if (why == composer_text::Failure::None) return EditFailure::None;
        return why == composer_text::Failure::Canceled ? EditFailure::Canceled : EditFailure::Deadline;
    }
    static bool same(const Text &a, const Text &b) noexcept {
        return a.status == composer_text::Status::Copied && b.status == composer_text::Status::Copied &&
            a.revisionAfter == b.revisionAfter && a.characterCount == b.characterCount &&
            a.count == b.count && std::equal(a.units.begin(),a.units.begin()+a.count,b.units.begin());
    }
    static bool matches(const Range &range, const Join &joined) {
        return range.visitRevoked && !range.visitRevoked->load(std::memory_order_acquire) &&
            range.document && range.document.data() == joined.document &&
            joined.root == range.root && joined.control == range.control &&
            joined.participant == range.participant && joined.privateData == range.privateData;
    }
    // Private issuer-only provisioning. The caller must have the genuine
    // selected-page binding and an independently watched continuous visit.
    // Neither a page-name string nor the callback's role ID can mint that.
    Provisioned provision(::Scene *scene, const Guard &guard,
            const std::shared_ptr<std::atomic<bool>> &visitRevoked) const {
        Provisioned output;
        auto &result = output.receipt;
        std::unique_ptr<detail::Insertion> action;
        auto run = [&]() -> Provisioned {
        auto refuse = [&](EditFailure why) { result.failure = why; return output; };
        if (!thread()) return refuse(EditFailure::Thread); // Before ANY Scene access.
        if (!visitRevoked || visitRevoked->load(std::memory_order_acquire)) return refuse(EditFailure::Canceled);
        if (const auto why = interrupted(guard); why != EditFailure::None) return refuse(why);
        const auto joined = join_(scene);
        const auto before = plain_->sample(joined,guard);
        if (!before.admitted || before.text.count > Text::Capacity - 2) return refuse(EditFailure::Plain);
        if (!sequence_(joined.root,before.text,guard).matched) return refuse(EditFailure::Anchors);
        const auto stack = context_(scene,joined);
        if (!stack) return refuse(EditFailure::Context);
        action.reset(new detail::Insertion(*insert_));
        if (!action->prepare(joined.root,static_cast<int>(before.text.count),QStringLiteral("\n\n"))) {
            result.action = action->receipt(); return refuse(EditFailure::Preparation);
        }
        const auto again = join_(scene);
        const auto final = plain_->sample(again,guard);
        if (!composer_scene::detail::sameScalarBorrow(joined,again) || !final.admitted ||
            context_(scene,again) != stack) return refuse(EditFailure::Context);
        if (!sequence_(again.root,final.text,guard).matched) return refuse(EditFailure::Anchors);
        if (!same(before.text,final.text)) return refuse(EditFailure::Revision);
        if (visitRevoked->load(std::memory_order_acquire)) return refuse(EditFailure::Canceled);
        if (const auto why = interrupted(guard); why != EditFailure::None) return refuse(why);
        // All getter/factory/preflight work has completed. These final scalar
        // comparisons precede the admitted stock initialize→apply path.
        if (again.document->revision() != final.text.revisionAfter ||
            again.document->characterCount() != final.text.characterCount) return refuse(EditFailure::Revision);
        result.status = EditStatus::Unknown;
        action->submit(stack);
        result.action = action->receipt();
        if (!result.action.submissionReturned) return refuse(EditFailure::Submission);
        const auto ids = action->interval();
        const auto settledJoin = join_(scene);
        const auto after = plain_->sample(settledJoin,guard);
        bool verified = ids && composer_scene::detail::sameScalarBorrow(joined,settledJoin) && after.admitted &&
            after.text.count == before.text.count + 2 &&
            std::equal(before.text.units.begin(),before.text.units.begin()+before.text.count,after.text.units.begin()) &&
            after.text.units[before.text.count] == 0x2029 && after.text.units[before.text.count+1] == 0x2029;
        if (verified) {
            const auto anchors = anchors_(settledJoin.root,ids->first,ids->last,after.text,guard);
            verified = anchors.matched && anchors.left == static_cast<int>(before.text.count) &&
                anchors.right == anchors.left + 1;
        }
        action->finish(); result.action = action->receipt();
        if (!verified || visitRevoked->load(std::memory_order_acquire)) return refuse(EditFailure::Settlement);
        if (!result.action.releaseReturned) return refuse(EditFailure::Release);
        if (const auto why = interrupted(guard); why != EditFailure::None) return refuse(why);
        output.range = std::shared_ptr<Range>(new Range(settledJoin,ids->first,ids->last,visitRevoked));
        result.beforeRevision = before.text.revisionAfter; result.afterRevision = after.text.revisionAfter;
        result.revisionsPresent = true; result.status = EditStatus::ProjectionVerified;
        return output;
        };
        try { (void)run(); } catch (...) { result.failure=EditFailure::Exception; }
        if (action) {
            action->finish(); result.action=action->receipt();
            if (result.status==EditStatus::ProjectionVerified && !result.action.releaseReturned) {
                result.status=EditStatus::Unknown; result.failure=EditFailure::Release; output.range.reset();
            }
        }
        return output;
    }
    ObservedDraft observe(::Scene *scene,const Range &range,const Guard &guard) const {
        ObservedDraft output;
        auto refuse=[&](EditFailure why) {output.failure=why;output.text.clear();return output;};
        try {
            if(!thread())return refuse(EditFailure::Thread);
            if(!range.visitRevoked || range.visitRevoked->load(std::memory_order_acquire))return refuse(EditFailure::Canceled);
            if(const auto why=interrupted(guard);why!=EditFailure::None)return refuse(why);
            const auto joined=join_(scene);
            if(!matches(range,joined))return refuse(EditFailure::Incarnation);
            const auto plain=plain_->sample(joined,guard);
            if(!plain.admitted)return refuse(EditFailure::Plain);
            const auto anchors=anchors_(joined.root,range.left,range.right,plain.text,guard);
            if(!anchors.matched)return refuse(EditFailure::Anchors);
            const auto again=join_(scene);
            if(!matches(range,again) || !composer_scene::detail::sameScalarBorrow(joined,again))return refuse(EditFailure::Incarnation);
            // fromUtf16 is a BOM-detecting decoder: it may remove an initial
            // FEFF or byte-swap after FFFE. Native code units must stay exact.
            output.text.resize(static_cast<qsizetype>(anchors.draftCount));
            for(std::size_t i=0;i<anchors.draftCount;++i)
                output.text[static_cast<qsizetype>(i)]=QChar(anchors.draft[i]);
            if(again.document->revision()!=plain.text.revisionAfter || again.document->characterCount()!=plain.text.characterCount)
                return refuse(EditFailure::Revision);
            if(const auto why=interrupted(guard);why!=EditFailure::None)return refuse(why);
            if(!matches(range,again))return refuse(EditFailure::Incarnation);
            output.revision=plain.text.revisionAfter;output.observed=true;return output;
        }catch(...) {return refuse(EditFailure::Exception);}
    }
    EditReceipt clear(::Scene *scene, const Range &range, int expectedRevision,
            const QString &expectedText, const Guard &guard) const {
        EditReceipt result;
        std::unique_ptr<detail::Removal> action;
        auto run = [&]() -> EditReceipt {
        auto refuse = [&](EditFailure why) { result.failure=why; return result; };
        if (!thread()) return refuse(EditFailure::Thread);
        if (!range.visitRevoked || range.visitRevoked->load(std::memory_order_acquire)) return refuse(EditFailure::Canceled);
        if (const auto why=interrupted(guard);why!=EditFailure::None) return refuse(why);
        const auto joined=join_(scene);
        if (!matches(range,joined)) return refuse(EditFailure::Incarnation);
        const auto before=plain_->sample(joined,guard);
        if (!before.admitted) return refuse(EditFailure::Plain);
        const auto anchors=anchors_(joined.root,range.left,range.right,before.text,guard);
        if (!anchors.matched) return refuse(EditFailure::Anchors);
        const int start=anchors.left+1, length=anchors.right-start;
        if (!length) return refuse(EditFailure::Empty);
        if (before.text.revisionAfter!=expectedRevision) return refuse(EditFailure::Revision);
        if (expectedText.size()!=length || anchors.draftCount!=static_cast<std::size_t>(length)) return refuse(EditFailure::Text);
        for(int i=0;i<length;++i)
            if (expectedText.at(i).unicode()!=anchors.draft[static_cast<std::size_t>(i)]) return refuse(EditFailure::Text);
        const auto stack=context_(scene,joined);
        if (!stack) return refuse(EditFailure::Context);
        action.reset(new detail::Removal(*remove_));
        if (!action->prepare(joined.root,start,length)) { result.action=action->receipt(); return refuse(EditFailure::Preparation); }
        const auto again=join_(scene);
        const auto final=plain_->sample(again,guard);
        if (!matches(range,again) || !final.admitted || context_(scene,again)!=stack) return refuse(EditFailure::Context);
        const auto finalAnchors=anchors_(again.root,range.left,range.right,final.text,guard);
        if (!finalAnchors.matched || finalAnchors.left!=anchors.left || finalAnchors.right!=anchors.right)
            return refuse(EditFailure::Anchors);
        if (finalAnchors.draftCount!=anchors.draftCount || !std::equal(anchors.draft.begin(),
                anchors.draft.begin()+anchors.draftCount,finalAnchors.draft.begin()))return refuse(EditFailure::Text);
        if (!same(before.text,final.text)) return refuse(EditFailure::Revision);
        if (!matches(range,again)) return refuse(EditFailure::Incarnation);
        if (const auto why=interrupted(guard);why!=EditFailure::None) return refuse(why);
        if (again.document->revision()!=expectedRevision || again.document->characterCount()!=final.text.characterCount)
            return refuse(EditFailure::Revision);
        result.status=EditStatus::Unknown;
        action->submit(stack); result.action=action->receipt();
        if (!result.action.submissionReturned) return refuse(EditFailure::Submission);
        const auto settledJoin=join_(scene);
        const auto after=plain_->sample(settledJoin,guard);
        bool verified=matches(range,settledJoin) && after.admitted && after.text.count+length==before.text.count;
        if (verified) {
            const auto remaining=anchors_(settledJoin.root,range.left,range.right,after.text,guard);
            verified=remaining.matched && remaining.left==anchors.left && remaining.right==anchors.left+1 &&
                std::equal(before.text.units.begin(),before.text.units.begin()+start,after.text.units.begin()) &&
                std::equal(before.text.units.begin()+anchors.right,before.text.units.begin()+before.text.count,
                    after.text.units.begin()+start);
        }
        action->finish(); result.action=action->receipt();
        if (!verified) return refuse(EditFailure::Settlement);
        if (!matches(range,settledJoin)) return refuse(EditFailure::Incarnation);
        if (!result.action.releaseReturned) return refuse(EditFailure::Release);
        if (const auto why=interrupted(guard);why!=EditFailure::None) return refuse(why);
        result.beforeRevision=before.text.revisionAfter;result.afterRevision=after.text.revisionAfter;
        result.revisionsPresent=true;result.status=EditStatus::ProjectionVerified;
        return result;
        };
        try { (void)run(); } catch (...) { result.failure=EditFailure::Exception; }
        if (action) {
            action->finish(); result.action=action->receipt();
            if (result.status==EditStatus::ProjectionVerified && !result.action.releaseReturned) {
                result.status=EditStatus::Unknown; result.failure=EditFailure::Release;
            }
        }
        return result;
    }
public:
    OwnedEdit(const OwnedEdit &)=delete;
    OwnedEdit &operator=(const OwnedEdit &)=delete;
};
} // namespace remarkable::composer_writer
