#pragma once
// Bounded diagnostic data, never a draft/edit lease. No public raw-document admission.
#include <QTextDocument>
#include <QThread>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <limits>
#include <typeinfo>
#ifdef COMPOSER_TEXT_PRIVATE_QT
#include <QtGui/private/qtextdocument_p.h>
#include <QTextLayout>
#endif

namespace remarkable::composer_text {
using Clock = std::chrono::steady_clock;
struct ReadRange { std::int64_t start = 0, length = 0; };
enum class Status { NotRequested, Pending, Copied, Refused };
enum class Failure {
    None, Unsupported, Range, SplitSurrogate, Structure, Composition, Transaction,
    Thread, Type, InspectionLimit, Canceled, Deadline, Revision, Join, Attachment, Exception
};
struct Result {
    static constexpr std::size_t Capacity = 4096;
    Status status = Status::NotRequested;
    Failure failure = Failure::None;
    ReadRange range;
    std::array<std::uint16_t, Capacity> units{};
    std::size_t count = 0, visits = 0;
    bool revisionsPresent = false;
    int revisionBefore = 0, revisionAfter = 0, characterCount = 0;
    void refuse(Failure why) noexcept {
        status = Status::Refused; failure = why; count = 0; units.fill(0);
    }
};
namespace detail {
// Only Attachment calls this with a live, owner-thread native borrow.
// The caller must serialize relevant writers and preserve lifetime throughout.
struct Guard {
    const std::atomic<bool> &canceled;
    const std::atomic<bool> *external;
    Clock::time_point deadline;
    Failure check() const noexcept {
        if (canceled.load(std::memory_order_acquire) ||
            (external && external->load(std::memory_order_acquire))) return Failure::Canceled;
        if (Clock::now() >= deadline) return Failure::Deadline;
        return Failure::None;
    }
};
inline bool validRequest(ReadRange range) noexcept {
    return range.start >= 0 && range.length >= 0 &&
        range.length <= static_cast<std::int64_t>(Result::Capacity) &&
        range.start <= std::numeric_limits<std::int64_t>::max() - range.length;
}
#ifdef COMPOSER_TEXT_PRIVATE_QT
struct Inspection {
    static constexpr std::size_t Limit = 8192;
    const Guard &guard;
    std::size_t visits = 0;
    Failure failure = Failure::None;
    bool step() {
        if ((failure = guard.check()) != Failure::None) return false;
        if (visits == Limit) { failure = Failure::InspectionLimit; return false; }
        ++visits; return true;
    }
};
// No public rootFrame/objectForFormat/block.layout/fragment() getter: they may
// create objects or hide unbounded same-format fragment scans. Matching private
// Qt headers expose the existing data and one physical fragment per counted step.
inline bool inspect(const QTextDocumentPrivate &p, int start, int end, Inspection &scan) {
    if (!scan.step()) return false; // Root inspection, also on the second pass.
    if (p.isInEditBlock()) { scan.failure = Failure::Transaction; return false; }
    // QTextObjects are children of their document. Inspect existing frame objects
    // without rootFrame()/objectForIndex(), either of which may create one.
    // RTTI casts perform no virtual QObject/meta-object/application callout.
    for (auto *object : p.document()->children()) {
        if (!scan.step()) return false;
        const auto *frame = dynamic_cast<const QTextFrame *>(object);
        if (!frame || !frame->parentFrame()) continue;
        const int first = frame->firstPosition() - 1;
        const int last = frame->lastPosition();
        if (first <= end && last >= start) {
            scan.failure = Failure::Structure; return false;
        }
    }
    const auto &blocks = p.blockMap();
    for (auto n = blocks.findNode(start); n; n = blocks.next(n)) {
        if (!scan.step()) return false;
        const int position = blocks.position(n);
        if (position > end) break;
        const auto *block = blocks.fragment(n);
        const auto format = p.formatCollection()->blockFormat(block->format);
        if (format.objectIndex() != -1 || format.objectType() != QTextFormat::NoObject) {
            scan.failure = Failure::Structure; return false; // Includes lists, without object lookup.
        }
        // Reading an existing layout is safe; absence means no stored preedit.
        if (block->layout && !block->layout->preeditAreaText().isEmpty()) {
            scan.failure = Failure::Composition; return false;
        }
        if (static_cast<std::int64_t>(position) + blocks.size(n) > end) break; // End-point block included.
    }
    const auto &fragments = p.fragmentMap();
    // Inspect the end point's format too, including an empty interval/sentinel.
    for (auto n = fragments.findNode(start); n; n = fragments.next(n)) {
        if (!scan.step()) return false;
        const int position = fragments.position(n);
        if (position > end) break;
        const auto format = p.formatCollection()->charFormat(fragments.fragment(n)->format);
        if (format.objectIndex() != -1 || format.objectType() != QTextFormat::NoObject) {
            scan.failure = Failure::Structure; return false;
        }
        if (static_cast<std::int64_t>(position) + fragments.size(n) > end) break;
    }
    return true;
}
#endif
inline Result copy(QTextDocument &doc, ReadRange range, const Guard &guard
#ifdef COMPOSER_TEXT_TESTING
        , void *hookContext = nullptr, void (*afterCopy)(void *) = nullptr
#endif
) {
    Result result; result.range = range; result.status = Status::Pending;
    auto refuse = [&](Failure why) { result.refuse(why); return result; };
    if (!validRequest(range)) return refuse(Failure::Range);
    if (auto fail = guard.check(); fail != Failure::None) return refuse(fail);
    if (doc.thread() != QThread::currentThread()) return refuse(Failure::Thread);
    if (typeid(doc) != typeid(QTextDocument)) return refuse(Failure::Type);
#if defined(COMPOSER_TEXT_PRIVATE_QT) && (QT_VERSION == QT_VERSION_CHECK(6,10,3) || defined(COMPOSER_TEXT_TESTING))
    const auto *p = QTextDocumentPrivate::get(&doc);
    if (p->isInEditBlock()) return refuse(Failure::Transaction);
    result.revisionBefore = doc.revision();
    result.characterCount = doc.characterCount();
    if (result.characterCount < 1 || range.start > result.characterCount - 1 ||
        range.length > result.characterCount - 1 - range.start) return refuse(Failure::Range);
    const int start = static_cast<int>(range.start), end = static_cast<int>(range.start + range.length);
    auto split = [&](int at) {
        return at > 0 && at < result.characterCount - 1 &&
            doc.characterAt(at - 1).isHighSurrogate() && doc.characterAt(at).isLowSurrogate();
    };
    if (split(start) || split(end)) return refuse(Failure::SplitSurrogate);
    Inspection inspection{guard};
    if (!inspect(*p,start,end,inspection)) {
        result.visits = inspection.visits; return refuse(inspection.failure);
    }
    for (int at = start; at < end; ++at) {
        if (auto fail = guard.check(); fail != Failure::None) return refuse(fail);
        const auto unit = doc.characterAt(at).unicode();
        if (unit == 0xfffc || unit == 0xfdd0 || unit == 0xfdd1)
            return refuse(Failure::Structure);
        result.units[result.count++] = unit;
    }
#ifdef COMPOSER_TEXT_TESTING
    if (afterCopy) afterCopy(hookContext);
#endif
    if (!inspect(*p,start,end,inspection)) {
        result.visits = inspection.visits; return refuse(inspection.failure);
    }
    result.visits = inspection.visits;
    result.revisionAfter = doc.revision(); result.revisionsPresent = true;
    if (result.revisionBefore != result.revisionAfter ||
        result.characterCount != doc.characterCount()) return refuse(Failure::Revision);
    if (auto fail = guard.check(); fail != Failure::None) return refuse(fail);
    result.status = Status::Copied;
    return result;
#else
    return refuse(Failure::Unsupported);
#endif
}
} // namespace detail
} // namespace remarkable::composer_text
