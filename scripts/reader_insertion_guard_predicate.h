#pragma once
// Research-only negative predicate. No loader, native declaration, hook or grant.
#include <QString>
#include <array>
#include <cstdint>
#include <limits>
#include <memory>

namespace reader_insertion_predicate {
using Address = std::uint32_t;
constexpr unsigned MaxCurrentRecords = 16;
constexpr Address RecordStride = 8;
enum class Refusal {
    None, InputRange, InputId, InputCorrespondence, Admission, MissingCall,
    AddressRange, EffectiveIndex, InitialRead, Span, Count, SearchMissing,
    SearchPointer, SearchPosition, TargetMismatch, FinalRead, SpanChanged,
    Reentered
};
struct Config {
    int sourceIndex = -1, wrongIndex = -1, pageCount = -1;
    QString realSource, rightNeighbor, wrongSource, requestedTarget;
};
class Frozen;
struct Freezing { std::unique_ptr<const Frozen> frozen; Refusal refusal = Refusal::None; };
Freezing freeze(const Config &);
class Frozen final {
    int source_, wrong_, count_;
    std::array<QString, 4> ids_;
    explicit Frozen(const Config &c)
        : source_(c.sourceIndex), wrong_(c.wrongIndex), count_(c.pageCount),
          ids_{{c.realSource, c.rightNeighbor, c.wrongSource, c.requestedTarget}} {}
    friend Freezing freeze(const Config &);
public:
    int sourceIndex() const { return source_; }
    int wrongIndex() const { return wrong_; }
    int pageCount() const { return count_; }
    const QString &realSource() const { return ids_[0]; }
    const QString &rightNeighbor() const { return ids_[1]; }
    const QString &wrongSource() const { return ids_[2]; }
    const QString &requestedTarget() const { return ids_[3]; }
};
namespace detail {
inline bool canonical(const QString &id) {
    if (id.size() != 36) return false;
    for (int i = 0; i != 36; ++i) {
        const auto c = id.at(i).unicode();
        const bool dash = i == 8 || i == 13 || i == 18 || i == 23;
        if (dash ? c != '-' : !((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
    }
    return true;
}
inline bool add(Address a, Address b, Address &out) {
    if (a > std::numeric_limits<Address>::max() - b) return false;
    out = a + b; return true;
}
inline bool aligned(Address a) { return a != 0 && a % 4 == 0; }
inline thread_local bool reading = false, reentered = false;
struct Gate {
    bool entered = false;
    Gate() { if (reading) reentered = true; else { reading = true; reentered = false; entered = true; } }
    ~Gate() { if (entered) reading = false; }
};
}
inline Freezing freeze(const Config &c) {
    if (c.pageCount < 2 || c.pageCount > int(MaxCurrentRecords) || c.sourceIndex < 0 ||
        c.sourceIndex >= c.pageCount - 1 || c.wrongIndex < 0 || c.wrongIndex >= c.pageCount ||
        c.wrongIndex == c.sourceIndex) return {nullptr, Refusal::InputRange};
    for (const QString *id : {&c.realSource, &c.rightNeighbor, &c.wrongSource, &c.requestedTarget})
        if (!detail::canonical(*id)) return {nullptr, Refusal::InputId};
    const bool wrongIsRight = c.wrongIndex == c.sourceIndex + 1;
    if (c.realSource == c.rightNeighbor || c.realSource == c.wrongSource ||
        (c.wrongSource == c.rightNeighbor) != wrongIsRight ||
        c.requestedTarget == c.realSource || c.requestedTarget == c.rightNeighbor ||
        c.requestedTarget == c.wrongSource) return {nullptr, Refusal::InputCorrespondence};
    return {std::unique_ptr<const Frozen>(new Frozen(c)), Refusal::None};
}
// These are caller assertions, not qualifications established by this core.
struct CallerAdmission {
    bool profileAndProviders = false, originalCallsite = false, operationScope = false;
    bool workerThread = false, nativeOwners = false, finalLock = false;
};
struct Words { Address begin = 0, end = 0; };
struct Calls {
    // Reader consumes the two contiguous pointer words beginning at firstWord.
    bool (*readWords)(Address firstWord, Words &) = nullptr;
    Address (*search)(Address opaqueReceiver, const QString &) = nullptr;
    bool (*equal)(const QString &, const QString &) = nullptr;
};
struct Frame {
    Address worker = 0, map = 0;
    int effectiveIndex = -1;
    const QString *effectiveTarget = nullptr;
};
struct Result {
    bool observedMismatch = false;
    Refusal refusal = Refusal::None;
    unsigned wordReads = 0, searches = 0, equalities = 0, count = 0;
    Address begin = 0, end = 0;
    std::array<int, 3> positions{{-1, -1, -1}};
    bool realAtSource = false, rightAdjacent = false, wrongAtFrozenPosition = false;
    bool targetMatches = false, spanUnchanged = false;
    static constexpr bool insertionPermission = false;
    static constexpr bool nativeAuthority = false;
};
// No QString construction/copy/detach, GUI calls, IO, ownership or allocation.
// Injected calls must meet the separately qualified passive native contracts.
// Exceptions propagate: this is not recovery for cantunwind/native corruption.
inline Result observe(const Frozen &f, const Frame &frame, const Calls &calls,
                      const CallerAdmission &admission) {
    Result result;
    detail::Gate gate;
    if (!gate.entered) { result.refusal = Refusal::Reentered; return result; }
    auto fail = [&](Refusal why) { result.refusal = why; return result; };
    if (!admission.profileAndProviders || !admission.originalCallsite || !admission.operationScope ||
        !admission.workerThread || !admission.nativeOwners || !admission.finalLock)
        return fail(Refusal::Admission);
    if (!calls.readWords || !calls.search || !calls.equal || !frame.effectiveTarget)
        return fail(Refusal::MissingCall);
    Address map = 0, receiver = 0, firstWord = 0, lastByteEnd = 0;
    if (!detail::aligned(frame.worker) || !detail::aligned(frame.map) ||
        !detail::add(frame.worker, 200, map) || map != frame.map ||
        !detail::add(map, 80, receiver) || !detail::add(map, 84, firstWord) ||
        !detail::add(map, 92, lastByteEnd)) return fail(Refusal::AddressRange);
    (void)lastByteEnd;
    if (frame.effectiveIndex != f.sourceIndex() + 1) return fail(Refusal::EffectiveIndex);
    Words before;
    ++result.wordReads;
    if (!calls.readWords(firstWord, before)) return fail(Refusal::InitialRead);
    if (detail::reentered) return fail(Refusal::Reentered);
    result.begin = before.begin; result.end = before.end;
    if (!detail::aligned(before.begin) || !detail::aligned(before.end) || before.end <= before.begin)
        return fail(Refusal::Span);
    const Address span = before.end - before.begin;
    if (span > MaxCurrentRecords * RecordStride || span % RecordStride != 0)
        return fail(Refusal::Span);
    const Address count = span / RecordStride;
    result.count = count;
    if (count < 2 || count != Address(f.pageCount())) return fail(Refusal::Count);
    const std::array<int, 3> expected{{f.sourceIndex(), f.sourceIndex() + 1, f.wrongIndex()}};
    std::array<Address, 3> positions{};
    for (unsigned i = 0; i != 3; ++i) {
        if (expected[i] < 0 || Address(expected[i]) >= count ||
            !detail::add(before.begin, Address(expected[i]) * RecordStride, positions[i]) ||
            positions[i] >= before.end) return fail(Refusal::Count);
    }
    const std::array<const QString *, 3> ids{{&f.realSource(), &f.rightNeighbor(), &f.wrongSource()}};
    for (unsigned i = 0; i != 3; ++i) {
        ++result.searches;
        const Address found = calls.search(receiver, *ids[i]);
        if (detail::reentered) return fail(Refusal::Reentered);
        if (found < before.begin || found > before.end || (found - before.begin) % RecordStride != 0)
            return fail(Refusal::SearchPointer);
        if (found == before.end) return fail(Refusal::SearchMissing);
        result.positions[i] = int((found - before.begin) / RecordStride);
        if (found != positions[i]) return fail(Refusal::SearchPosition);
        if (i == 0) result.realAtSource = true;
        if (i == 1) result.rightAdjacent = true;
        if (i == 2) result.wrongAtFrozenPosition = true;
    }
    ++result.equalities;
    result.targetMatches = calls.equal(*frame.effectiveTarget, f.requestedTarget());
    if (detail::reentered) return fail(Refusal::Reentered);
    if (!result.targetMatches) return fail(Refusal::TargetMismatch);
    Words after;
    ++result.wordReads;
    if (!calls.readWords(firstWord, after)) return fail(Refusal::FinalRead);
    if (detail::reentered) return fail(Refusal::Reentered);
    if (after.begin != before.begin || after.end != before.end) return fail(Refusal::SpanChanged);
    result.spanUnchanged = true;
    result.observedMismatch = true; // real != wrong was validated before freeze.
    return result;
}
}
