#pragma once
// Borrow-local CRDT correspondence, NOT persisted range ownership or admission.
#include "composer_text_copy.h"
#include <cstring>
#include <optional>

namespace remarkable::composer_writer::detail {
struct NativeId {
    std::uint32_t counterLow = 0, counterHighAuthor = 0;
    std::uint64_t counter() const noexcept {
        return counterLow | (std::uint64_t(counterHighAuthor & 0xffffU) << 32);
    }
    std::uint16_t author() const noexcept { return counterHighAuthor >> 16; }
    bool empty() const noexcept { return counter() == 0; }
    bool operator==(const NativeId &other) const noexcept {
        return counterLow == other.counterLow && counterHighAuthor == other.counterHighAuthor;
    }
};
enum class AnchorFailure {
    None, Unsupported, Canceled, Deadline, InvalidId, Link, NodeLimit, TextLimit,
    CounterOverflow, Variant, Tombstone, Duplicate, Missing, NotLineFeed, Order,
    Projection, TextNotCopied
};
struct AnchorPositions {
    AnchorFailure failure = AnchorFailure::None;
    int left = -1, right = -1;
    std::size_t nodes = 0, units = 0;
    bool matched = false;
    std::array<std::uint16_t,composer_text::Result::Capacity> draft{};
    std::size_t draftCount = 0;
};
struct NativeSequenceReader {
    static std::uint32_t word(std::uintptr_t base, std::size_t offset) noexcept {
        std::uint32_t value;
        std::memcpy(&value, reinterpret_cast<const void *>(base + offset), sizeof value);
        return value;
    }
    static std::uint16_t unit(std::uintptr_t data, std::size_t at) noexcept {
        std::uint16_t value;
        std::memcpy(&value, reinterpret_cast<const void *>(data + at * 2), sizeof value);
        return value;
    }
};

// Preconditions: exact admitted native owner-thread borrow, pinned layout,
// closed write scopes; root remains live for this entire call. This function
// does NOT make arbitrary pointers safe. It invokes no native search helper:
// those searches can walk tombstones without our inspection budget.
template<class Reader = NativeSequenceReader>
inline AnchorPositions walkAnchors(std::uintptr_t root, NativeId left, NativeId right,
        const composer_text::Result &projection, const composer_text::detail::Guard &guard,
        const Reader &reader = {}, bool requireAnchors = true) {
    AnchorPositions result;
    auto fail = [&](AnchorFailure why) {
        result.failure = why; result.draft.fill(0); result.draftCount=0; return result;
    };
    auto interrupted = [&]() {
        const auto check = guard.check();
        if (check == composer_text::Failure::Canceled) return AnchorFailure::Canceled;
        if (check != composer_text::Failure::None) return AnchorFailure::Deadline;
        return AnchorFailure::None;
    };
    if (const auto why = interrupted(); why != AnchorFailure::None) return fail(why);
    if (!root || root > std::numeric_limits<std::uintptr_t>::max() - 0x80)
        return fail(AnchorFailure::Link);
    if (requireAnchors && (left.empty() || right.empty() || left == right)) return fail(AnchorFailure::InvalidId);
    if (projection.status != composer_text::Status::Copied || projection.range.start != 0 ||
        projection.count > composer_text::Result::Capacity ||
        projection.range.length != static_cast<std::int64_t>(projection.count) ||
        projection.characterCount != static_cast<int>(projection.count + 1))
        return fail(AnchorFailure::TextNotCopied);
    constexpr std::size_t NodeLimit = 1024;
    constexpr std::uint64_t MaximumCounter = (std::uint64_t(1) << 48) - 1;
    const auto sentinel = root + 0x80; // sequence+0x48, not the fixed root role ID.
    const std::uintptr_t head = reader.word(sentinel, 0);
    const std::uintptr_t tail = reader.word(sentinel, 4);
    auto node = head;
    auto previous = sentinel;
    bool foundLeft = false, foundRight = false;
    while (node != sentinel) {
        if (const auto why = interrupted(); why != AnchorFailure::None) return fail(why);
        if (result.nodes == NodeLimit) return fail(AnchorFailure::NodeLimit);
        if (!node || node > std::numeric_limits<std::uintptr_t>::max() - 0x34 ||
            reader.word(node, 4) != previous)
            return fail(AnchorFailure::Link);
        ++result.nodes; // ALL nodes, including tombstones and empty runs.
        const NativeId base{reader.word(node, 8), reader.word(node, 12)};
        const auto deleted = reader.word(node, 0x30);
        const auto variant = reader.word(node, 0x2c);
        if (!deleted && variant) return fail(AnchorFailure::Variant);
        const std::uint64_t length = deleted ? deleted : reader.word(node, 0x28);
        if (length && base.empty()) return fail(AnchorFailure::InvalidId);
        if (length && length - 1 > MaximumCounter - base.counter())
            return fail(AnchorFailure::CounterOverflow);
        auto offset = [&](NativeId anchor) -> std::optional<std::size_t> {
            if (!requireAnchors) return {};
            if (anchor.author() != base.author() || anchor.counter() < base.counter() ||
                anchor.counter() - base.counter() >= length) return {};
            return static_cast<std::size_t>(anchor.counter() - base.counter());
        };
        const auto leftAt = offset(left), rightAt = offset(right);
        if (deleted) {
            // Native reverse lookup alone can resolve a deleted ID. Refuse
            // even if another live interval appears to contain the same ID.
            if (leftAt || rightAt) return fail(AnchorFailure::Tombstone);
        } else {
            if (length > composer_text::Result::Capacity - result.units)
                return fail(AnchorFailure::TextLimit);
            const std::uintptr_t data = reader.word(node, 0x24);
            if (length && (!data || (data & 1) ||
                    length > (std::numeric_limits<std::uintptr_t>::max() - data) / 2))
                return fail(AnchorFailure::Link);
            if ((leftAt && foundLeft) || (rightAt && foundRight))
                return fail(AnchorFailure::Duplicate);
            for (std::size_t at = 0; at < length; ++at) {
                if (const auto why = interrupted(); why != AnchorFailure::None) return fail(why);
                const auto native = reader.unit(data, at);
                if ((leftAt && *leftAt == at) || (rightAt && *rightAt == at)) {
                    if (native != 0x000a) return fail(AnchorFailure::NotLineFeed);
                    if (leftAt && *leftAt == at) {
                        result.left = static_cast<int>(result.units + at); foundLeft = true;
                    }
                    if (rightAt && *rightAt == at) {
                        result.right = static_cast<int>(result.units + at); foundRight = true;
                    }
                }
                if (foundLeft && !foundRight && !(leftAt && *leftAt == at))
                    result.draft[result.draftCount++]=native;
                // Qt's actual paragraph separator represents a native LF.
                // No visible text delimiter can substitute for the native ID.
                const auto qt = native == 0x000a ? 0x2029 : native;
                if (result.units + at >= projection.count ||
                    projection.units[result.units + at] != qt)
                    return fail(AnchorFailure::Projection);
            }
            result.units += static_cast<std::size_t>(length);
        }
        previous = node;
        node = reader.word(node, 0);
    }
    if (previous != tail || reader.word(sentinel, 0) != head ||
        reader.word(sentinel, 4) != tail)
        return fail(AnchorFailure::Link);
    if (result.units != projection.count) return fail(AnchorFailure::Projection);
    if (requireAnchors && (!foundLeft || !foundRight)) return fail(AnchorFailure::Missing);
    if (requireAnchors && result.left >= result.right) return fail(AnchorFailure::Order);
    if (const auto why = interrupted(); why != AnchorFailure::None) return fail(why);
    result.matched = true;
    return result;
}
template<class Reader = NativeSequenceReader>
inline AnchorPositions inspectSequence(std::uintptr_t root,const composer_text::Result &projection,
        const composer_text::detail::Guard &guard,const Reader &reader = {}) {
    // Complete native shape/projection admission before initial provisioning.
    // No anchors, range or ownership are produced by this form.
    return walkAnchors(root,{}, {},projection,guard,reader,false);
}
} // namespace remarkable::composer_writer::detail
