#pragma once
// Exact-build research only. Six individually locked reads are not a transaction.
#include "reader_native_worker_bridge.h"
#include <thread>
#include <limits>

namespace reader_worker_order {
using Permit = reader_worker_native::Permit;
using IdBytes = std::array<char, 37>;
enum class Refusal { None, NullWorker, GuiThread, Reentered, EmptyId, InvalidId,
                     IdMismatch, InverseMismatch, AccessorException };
struct Slot { int index = -1, inverse = -1; IdBytes id{}; };
struct Result {
    bool observed = false;
    unsigned calls = 0;
    Refusal refusal = Refusal::None;
    std::array<Slot, 3> entries{};
};
namespace detail {
using IdAt = QString (*)(QObject *, int);
using IndexFor = int (*)(QObject *, const QString &);
inline bool canonical(const QString &value, IdBytes &bytes) {
    if (value.size() != 36) return false;
    for (int i = 0; i < 36; ++i) {
        const ushort c = value.at(i).unicode();
        const bool dash = i == 8 || i == 13 || i == 18 || i == 23;
        if (dash ? c != '-' : !((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
        bytes[static_cast<size_t>(i)] = static_cast<char>(c);
    }
    bytes[36] = '\0'; return true;
}
inline thread_local bool reading = false, readReentered = false;
struct ReadGate {
    bool entered = false;
    ReadGate() { if (reading) readReentered = true; else { reading = true; readReentered = false; entered = true; } }
    ~ReadGate() { if (entered) reading = false; }
};
inline thread_local bool loading = false, loadReentered = false;
struct LoadGate {
    bool entered = false;
    LoadGate() { if (loading) loadReentered = true; else { loading = true; loadReentered = false; entered = true; } }
    ~LoadGate() { if (entered) loading = false; }
};
inline bool allowed(const Permit &permit) {
    QPointer<QCoreApplication> app = QCoreApplication::instance();
    if (!reader_worker_native::detail::gui() || loadReentered || !permit) return false;
    try { if (!permit()) return false; } catch (...) { return false; }
    return app && QCoreApplication::instance() == app && reader_worker_native::detail::gui() && !loadReentered;
}
}
class Frozen;
struct FrozenLoading { std::shared_ptr<const Frozen> frozen; QString refusal; };
FrozenLoading freeze(int sourceIndex, int pageCount, const std::array<QString, 3> &expected);
class Frozen final {
    std::array<int, 3> indexes_{};
    std::array<IdBytes, 3> expected_{};
    std::thread::id guiThread_;
    Frozen(int source, const std::array<IdBytes, 3> &expected)
        : indexes_{{source - 1, source, source + 1}}, expected_(expected), guiThread_(std::this_thread::get_id()) {}
    friend FrozenLoading freeze(int, int, const std::array<QString, 3> &);
    friend class Accessors;
public:
    const std::array<int, 3> &indexes() const { return indexes_; }
    const std::array<IdBytes, 3> &expectedIds() const { return expected_; }
};
inline FrozenLoading freeze(int sourceIndex, int pageCount, const std::array<QString, 3> &expected) {
    FrozenLoading result;
    // Compare against count-1 only after checking count; no signed addition overflow.
    if (!reader_worker_native::detail::gui() || pageCount < 3 || sourceIndex < 1 || sourceIndex >= pageCount - 1) {
        result.refusal = QStringLiteral("worker-order-input-range"); return result;
    }
    std::array<IdBytes, 3> bytes{};
    for (size_t i = 0; i < bytes.size(); ++i)
        if (!detail::canonical(expected[i], bytes[i])) {
            result.refusal = QStringLiteral("worker-order-input-id"); return result;
        }
    if (bytes[0] == bytes[1] || bytes[0] == bytes[2] || bytes[1] == bytes[2]) {
        result.refusal = QStringLiteral("worker-order-input-duplicate"); return result;
    }
    result.frozen = std::shared_ptr<const Frozen>(new Frozen(sourceIndex, bytes));
    return result;
}
class Accessors;
struct Loading {
    std::unique_ptr<Accessors> accessors;
    // GUI caller may reuse this already validated bridge; no preparation occurred.
    std::unique_ptr<reader_worker_native::Bridge> nativeBridge;
    QString refusal;
};
Loading loadPinned(const Permit &permit);
#ifdef READER_WORKER_ORDER_TESTING
namespace testing { Loading load(detail::IdAt, detail::IndexFor); }
#endif
class Accessors final {
    detail::IdAt idAt_;
    detail::IndexFor indexFor_;
    Accessors(detail::IdAt first, detail::IndexFor second) : idAt_(first), indexFor_(second) {}
    friend Loading loadPinned(const Permit &);
#ifdef READER_WORKER_ORDER_TESTING
    friend Loading testing::load(detail::IdAt, detail::IndexFor);
#endif
public:
    // Caller owns an external native lease through actual callback completion.
    // No QObject/GUI API, dynamic permit, file IO or owner release occurs here.
    Result read(QObject *externallyLeasedWorker, const Frozen &frozen) const noexcept {
        Result result;
        for (size_t i = 0; i < 3; ++i) result.entries[i].index = frozen.indexes_[i];
        if (!externallyLeasedWorker) { result.refusal = Refusal::NullWorker; return result; }
        if (std::this_thread::get_id() == frozen.guiThread_) { result.refusal = Refusal::GuiThread; return result; }
        detail::ReadGate gate;
        if (!gate.entered) { result.refusal = Refusal::Reentered; return result; }
        try {
            for (size_t i = 0; i < 3; ++i) {
                auto &slot = result.entries[i];
                ++result.calls;
                const QString id = idAt_(externallyLeasedWorker, slot.index);
                if (detail::readReentered) { result.refusal = Refusal::Reentered; return result; }
                if (id.isEmpty()) { result.refusal = Refusal::EmptyId; return result; }
                if (!detail::canonical(id, slot.id)) { result.refusal = Refusal::InvalidId; return result; }
                if (slot.id != frozen.expected_[i]) { result.refusal = Refusal::IdMismatch; return result; }
                ++result.calls;
                slot.inverse = indexFor_(externallyLeasedWorker, id);
                if (detail::readReentered) { result.refusal = Refusal::Reentered; return result; }
                if (slot.inverse != slot.index) { result.refusal = Refusal::InverseMismatch; return result; }
            }
            result.observed = true;
        } catch (...) { result.refusal = Refusal::AccessorException; }
        return result;
    }
};
#ifdef READER_WORKER_ORDER_TESTING
namespace testing {
inline Loading load(detail::IdAt first, detail::IndexFor second) {
    Loading result;
    if (!first || !second) { result.refusal = QStringLiteral("worker-order-test-functions"); return result; }
    result.accessors.reset(new Accessors(first, second)); return result;
}
}
#endif
inline Loading loadPinned(const Permit &permit) {
    Loading result;
    detail::LoadGate gate;
    if (!gate.entered || !detail::allowed(permit)) { result.refusal = QStringLiteral("worker-order-load-refused"); return result; }
    auto base = reader_worker_native::loadPinned([&permit] { return detail::allowed(permit); });
    if (!base.bridge) { result.refusal = base.refusal; return result; }
#if defined(__linux__) && defined(__arm__) && defined(__ARM_PCS_VFP) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
    static_assert(sizeof(QString) == 12 && alignof(QString) == 4 && sizeof(void *) == 4);
    struct Image {
        int mains = 0;
        bool valid = true;
        std::array<quint64, 2> offsets{};
        static int visit(dl_phdr_info *info, size_t, void *opaque) {
            if (info->dlpi_name && *info->dlpi_name) return 0;
            auto &image = *static_cast<Image *>(opaque); ++image.mains;
            if (info->dlpi_addr != 0 || info->dlpi_phnum > 128) image.valid = false;
            const std::array<std::uintptr_t, 2> addresses{{0x6d5bf0, 0x6d5c98}};
            const std::array<std::uintptr_t, 2> lengths{{0xa8, 0x84}};
            for (size_t n = 0; n < 2; ++n) {
                int matches = 0;
                for (ElfW(Half) p = 0; p < info->dlpi_phnum && p < 128; ++p) {
                    const auto &segment = info->dlpi_phdr[p];
                    if (segment.p_type != PT_LOAD || (segment.p_flags & (PF_R | PF_X)) != (PF_R | PF_X) ||
                        segment.p_vaddr > addresses[n]) continue;
                    const quint64 offset = quint64(addresses[n]) - segment.p_vaddr;
                    if (offset <= segment.p_memsz && lengths[n] <= segment.p_memsz - offset &&
                        offset <= segment.p_filesz && lengths[n] <= segment.p_filesz - offset) {
                        ++matches; image.offsets[n] = quint64(segment.p_offset) + offset;
                    }
                }
                if (matches != 1) image.valid = false;
            }
            return 0;
        }
    } image;
    dl_iterate_phdr(&Image::visit, &image);
    if (image.mains != 1 || !image.valid || !detail::allowed(permit)) {
        result.refusal = QStringLiteral("worker-order-main-ranges"); return result;
    }
    QFile exe(QStringLiteral("/proc/self/exe")); struct stat before{}, after{};
    if (::stat("/proc/self/exe", &before) || !S_ISREG(before.st_mode) || before.st_size != 22046100 || !exe.open(QIODevice::ReadOnly)) {
        result.refusal = QStringLiteral("worker-order-code-file"); return result;
    }
    const std::array<std::uintptr_t, 2> addresses{{0x6d5bf0, 0x6d5c98}};
    const std::array<qint64, 2> lengths{{0xa8, 0x84}};
    for (size_t i = 0; i < 2; ++i) {
        if (!detail::allowed(permit) || !exe.seek(static_cast<qint64>(image.offsets[i]))) {
            result.refusal = QStringLiteral("worker-order-code-refused"); return result;
        }
        const auto bytes = exe.read(lengths[i]);
        if (bytes.size() != lengths[i] || std::memcmp(bytes.constData(), reinterpret_cast<const void *>(addresses[i]), size_t(lengths[i]))) {
            result.refusal = QStringLiteral("worker-order-mapped-code"); return result;
        }
    }
    if (::stat("/proc/self/exe", &after) || before.st_dev != after.st_dev || before.st_ino != after.st_ino ||
        before.st_size != after.st_size || before.st_mtime != after.st_mtime || !detail::allowed(permit)) {
        result.refusal = QStringLiteral("worker-order-load-refused"); return result;
    }
    result.accessors.reset(new Accessors(reinterpret_cast<detail::IdAt>(0x6d5bf0U), reinterpret_cast<detail::IndexFor>(0x6d5c98U)));
    result.nativeBridge = std::move(base.bridge);
#else
    result.refusal = QStringLiteral("worker-order-unsupported-image");
#endif
    return result;
}
} // namespace reader_worker_order
