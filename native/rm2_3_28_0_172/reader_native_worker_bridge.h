#pragma once
// Exact-build research adapter. No portable native capability or action authority.
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QFile>
#include <QMetaType>
#include <QPointer>
#include <QThread>
#include <QVariant>
#include <array>
#include <cstring>
#include <functional>
#include <memory>
#include <cstdint>
#if defined(__linux__)
#include <elf.h>
#include <link.h>
#include <sys/stat.h>
#endif

namespace reader_worker_native {
using Permit = std::function<bool()>;
struct Submission {
    bool attempted = false, accepted = false, unknown = false;
    qulonglong jobId = 0;
    QString refusal;
};
namespace detail {
using Resolve = void (*)(void *, void *, const void *);
using Acquire = void (*)(void *, void *, int);
using Request = qulonglong (*)(void *, const std::function<void()> &);
using Release = void (*)(void *);
struct Functions {
    Resolve resolve = nullptr;
    Acquire acquire = nullptr;
    Request request = nullptr;
    Release release = nullptr;
    const void *idInterface = nullptr;
    const QMetaObject *workerMeta = nullptr;
};
inline thread_local bool busy = false, reentered = false;
struct Gate {
    bool entered = false;
    Gate() { if (busy) reentered = true; else { busy = true; reentered = false; entered = true; } }
    ~Gate() { if (entered) busy = false; }
};
inline bool gui() {
    auto *app = QCoreApplication::instance();
    return app && !QCoreApplication::closingDown() && app->thread() == QThread::currentThread();
}
inline bool allowed(const Permit &permit) {
    QPointer<QCoreApplication> app = QCoreApplication::instance();
    if (!gui() || reentered || !permit) return false;
    try { if (!permit()) return false; } catch (...) { return false; }
    return app && QCoreApplication::instance() == app && gui() && !reentered;
}
inline bool ancestry(QObject *object, const char *name, const QMetaObject *exact = nullptr) {
    if (!object) return false;
    const QMetaObject *meta = object->metaObject();
    for (int n = 0; meta && n < 64; ++n, meta = meta->superClass())
        if ((!exact || exact == meta) && std::strcmp(meta->className(), name) == 0) return true;
    return false;
}
// Only fully returned native results are adopted. No guessed private member copy.
class Owner {
    alignas(std::uintptr_t) std::array<std::uintptr_t, 2> words{};
    Release releaseFn = nullptr;
public:
    explicit Owner(Release fn = nullptr) : releaseFn(fn) {}
    Owner(const Owner &) = delete;
    Owner &operator=(const Owner &) = delete;
    ~Owner() { reset(); }
    void adopt(const void *result) { std::memcpy(words.data(), result, sizeof(words)); }
    void *raw() const { return reinterpret_cast<void *>(words[0]); }
    bool owned() const { return words[0] && words[1]; }
    bool empty() const { return !words[0] && !words[1]; }
    bool reset() noexcept {
        const auto control = words[1];
        words = {};
        if (!control) return true;
        // Wrong-thread destruction deliberately leaks rather than running native teardown there.
        if (!gui() || !releaseFn) return false;
        try { releaseFn(reinterpret_cast<void *>(control)); return true; } catch (...) { return false; }
    }
};
struct State {
    Functions fn;
    Owner document, workerOwner;
    QPointer<QObject> controller, publicWorker;
    bool consumed = false, closed = false, closeRequested = false, releaseOk = true;
    explicit State(Functions value) : fn(value), document(value.release), workerOwner(value.release) {}
    bool live() const {
        return !closed && !closeRequested && controller && publicWorker && gui() &&
            controller->thread() == QThread::currentThread() && publicWorker->thread() == QThread::currentThread();
    }
    bool close() noexcept {
        if (closed) return releaseOk;
        if (!gui()) return false;
        closed = true;
        const bool workerOk = workerOwner.reset();
        const bool documentOk = document.reset();
        releaseOk = workerOk && documentOk;
        return releaseOk;
    }
};
} // namespace detail

class Prepared final {
    std::shared_ptr<detail::State> state;
    explicit Prepared(std::shared_ptr<detail::State> value) : state(std::move(value)) {}
    friend class Bridge;
public:
    Prepared(const Prepared &) = delete;
    Prepared &operator=(const Prepared &) = delete;
    ~Prepared() { close(); }
    QObject *worker() const { return state && state->live() ? state->publicWorker.data() : nullptr; }
    bool close() {
        auto current = state;
        if (!current) return true;
        if (current->closed) return current->releaseOk;
        if (detail::busy) { current->closeRequested = true; detail::reentered = true; return false; }
        detail::Gate gate;
        return current->close();
    }
    Submission submit(const std::function<void()> &callback, const Permit &permit) {
        // Local state survives even if a permit callback deletes the Prepared wrapper.
        auto current = state;
        detail::Gate gate;
        Submission result;
        if (!gate.entered) { result.refusal = QStringLiteral("native-reentered"); return result; }
        if (!current || current->consumed || current->closed) {
            result.refusal = QStringLiteral("native-already-consumed"); return result;
        }
        current->consumed = true;
        std::function<void()> retainedCallback;
        try { retainedCallback = callback; }
        catch (...) { result.refusal = QStringLiteral("native-callback-copy-refused"); return result; }
        if (!retainedCallback || !current->live() || !detail::allowed(permit) || !current->live()) {
            result.refusal = QStringLiteral("native-submission-refused"); return result;
        }
        result.attempted = true;
        try { result.jobId = current->fn.request(current->workerOwner.raw(), retainedCallback); }
        catch (...) { result.unknown = true; result.refusal = QStringLiteral("native-request-threw"); return result; }
        // Enqueue intentionally changes worker/UI notifications. Never consult the
        // old selected-page epoch or QObject getters after this native dispatch.
        if (!result.jobId || current->closed || current->closeRequested || detail::reentered) {
            result.unknown = true; result.refusal = QStringLiteral("native-request-outcome-unknown"); return result;
        }
        result.accepted = true;
        return result;
    }
};
struct Preparation {
    std::unique_ptr<Prepared> prepared;
    QString refusal;
    bool unknown = false;
};
struct Loading;
class Bridge;
class ResidentSlot;
Loading loadPinned(const Permit &permit);
#ifdef READER_WORKER_NATIVE_TESTING
namespace testing { Loading load(detail::Functions); }
#endif
class Bridge final {
    detail::Functions fn;
    bool preparedOnce = false;
    explicit Bridge(detail::Functions value) : fn(value) {}
    friend Loading loadPinned(const Permit &);
    friend class ResidentSlot;
#ifdef READER_WORKER_NATIVE_TESTING
    friend Loading testing::load(detail::Functions);
#endif
public:
    Bridge(const Bridge &) = delete;
    Bridge &operator=(const Bridge &) = delete;
    Preparation prepare(QObject *controller, const QVariant &actualId, QObject *expectedPublicWorker,
                        const Permit &permit) {
        detail::Gate gate;
        Preparation result;
        if (!gate.entered) { result.refusal = QStringLiteral("native-reentered"); return result; }
        if (preparedOnce) { result.refusal = QStringLiteral("native-already-prepared"); return result; }
        preparedOnce = true;
        const auto functions = fn;
        const auto type = actualId.metaType();
        // Reject unknown copy hooks before copying a potentially nontrivial value.
        if (type.iface() != functions.idInterface || type.sizeOf() != 12 || type.alignOf() != 4 || !actualId.constData()) {
            result.refusal = QStringLiteral("native-input-contract"); return result;
        }
        // Capture caller-owned values before the first potentially reentrant permit.
        QVariant id = actualId;
        auto current = std::make_shared<detail::State>(functions);
        current->controller = controller;
        current->publicWorker = expectedPublicWorker;
        if (!current->live() || !detail::ancestry(current->controller, "xofm::libs::library::DocumentController") ||
            !detail::ancestry(current->publicWorker, "DocumentWorker", functions.workerMeta) ||
            !qobject_cast<QThread *>(current->publicWorker.data())) {
            result.refusal = QStringLiteral("native-input-contract"); return result;
        }
        if (!detail::allowed(permit) || !current->live()) {
            result.refusal = QStringLiteral("native-preparation-refused"); return result;
        }
        alignas(std::uintptr_t) std::array<std::uintptr_t, 2> returned{};
        try { functions.resolve(returned.data(), current->controller.data(), id.constData()); }
        catch (...) { result.unknown = true; result.refusal = QStringLiteral("native-resolver-threw"); return result; }
        current->document.adopt(returned.data());
        if (!current->document.owned() || !detail::allowed(permit) || !current->live()) {
            result.refusal = QStringLiteral("native-document-lease-refused"); return result;
        }
        returned = {};
        try { functions.acquire(returned.data(), current->document.raw(), 1); }
        catch (...) { result.unknown = true; result.refusal = QStringLiteral("native-acquire-threw"); return result; }
        current->workerOwner.adopt(returned.data());
        if (!current->workerOwner.owned() || current->workerOwner.raw() != current->publicWorker.data() ||
            !detail::allowed(permit) || !current->live()) {
            result.refusal = QStringLiteral("native-worker-lease-refused"); return result;
        }
        result.prepared.reset(new Prepared(std::move(current)));
        return result;
    }
};
struct Loading { std::unique_ptr<Bridge> bridge; QString refusal; };

#ifdef READER_WORKER_NATIVE_TESTING
namespace testing {
inline Loading load(detail::Functions functions) {
    Loading value;
    if (!functions.resolve || !functions.acquire || !functions.request || !functions.release ||
        !functions.idInterface || !functions.workerMeta) { value.refusal = QStringLiteral("test-functions"); return value; }
    value.bridge.reset(new Bridge(functions));
    return value;
}
}
#endif

inline Loading loadPinned(const Permit &permit) {
    detail::Gate gate;
    Loading result;
    if (!gate.entered || !detail::allowed(permit)) { result.refusal = QStringLiteral("native-load-refused"); return result; }
#if defined(__linux__) && defined(__arm__) && defined(__ARM_PCS_VFP) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
    static_assert(sizeof(void *) == 4 && sizeof(std::function<void()>) == 16 && alignof(std::function<void()>) == 4);
    struct stat before{}, after{};
    QFile exe(QStringLiteral("/proc/self/exe"));
    if (::stat("/proc/self/exe", &before) || !S_ISREG(before.st_mode) || before.st_size != 22046100 ||
        !exe.open(QIODevice::ReadOnly)) { result.refusal = QStringLiteral("native-image-file"); return result; }
    const QByteArray header = exe.peek(52);
    if (header.size() != 52 || std::memcmp(header.constData(), "\177ELF\1\1", 6) ||
        static_cast<unsigned char>(header[18]) != EM_ARM || static_cast<unsigned char>(header[19]) != 0 ||
        static_cast<unsigned char>(header[16]) != ET_EXEC || static_cast<unsigned char>(header[17]) != 0) {
        result.refusal = QStringLiteral("native-image-abi"); return result;
    }
    std::uint32_t flags = 0; std::memcpy(&flags, header.constData() + 36, 4);
    if (flags != 0x05000400U) { result.refusal = QStringLiteral("native-image-abi"); return result; }
    QCryptographicHash hash(QCryptographicHash::Sha256);
    qint64 bytes = 0;
    while (bytes < 22046100) {
        if (!detail::allowed(permit)) { result.refusal = QStringLiteral("native-load-refused"); return result; }
        const auto chunk = exe.read(qMin<qint64>(1048576, 22046100 - bytes));
        if (chunk.isEmpty()) break;
        hash.addData(chunk); bytes += chunk.size();
    }
    if (bytes != 22046100 || !exe.atEnd() ||
        hash.result().toHex() != "071d85beef3ef2d4cc0e11002140b27b82a2cc04a2ed740a5669f591069b77df" ||
        ::stat("/proc/self/exe", &after) || before.st_dev != after.st_dev || before.st_ino != after.st_ino ||
        before.st_size != after.st_size || before.st_mtime != after.st_mtime) {
        result.refusal = QStringLiteral("native-image-digest"); return result;
    }
    struct Image {
        int mains = 0;
        bool ranges = true;
        std::array<quint64, 4> codeOffsets{};
        static int visit(dl_phdr_info *info, size_t, void *opaque) {
            if (info->dlpi_name && *info->dlpi_name) return 0;
            auto &image = *static_cast<Image *>(opaque); ++image.mains;
            if (info->dlpi_addr != 0) image.ranges = false;
            const std::array<std::uintptr_t, 6> addresses{{0x5d4c14, 0x6b9adc, 0x6d6b7c, 0x9d620, 0x14ff5e0, 0xe624d8}};
            const std::array<std::uintptr_t, 6> lengths{{0x13c, 0x2a8, 0x168, 0xcc, 64, 28}};
            for (size_t n = 0; n < addresses.size(); ++n) {
                bool found = false;
                for (ElfW(Half) p = 0; p < info->dlpi_phnum && p < 128; ++p) {
                    const auto &segment = info->dlpi_phdr[p];
                    if (segment.p_type == PT_LOAD && (segment.p_flags & PF_R) &&
                        (n >= 4 || (segment.p_flags & PF_X)) && addresses[n] >= segment.p_vaddr &&
                        addresses[n] + lengths[n] >= addresses[n] &&
                        addresses[n] + lengths[n] <= segment.p_vaddr + segment.p_memsz) {
                        found = true;
                        if (n < 4) {
                            if (addresses[n] + lengths[n] > segment.p_vaddr + segment.p_filesz) image.ranges = false;
                            image.codeOffsets[n] = segment.p_offset + addresses[n] - segment.p_vaddr;
                        }
                    }
                }
                if (!found) image.ranges = false;
            }
            return 0;
        }
    } image;
    dl_iterate_phdr(&Image::visit, &image);
    if (image.mains != 1 || !image.ranges || !detail::allowed(permit)) {
        result.refusal = QStringLiteral("native-main-image-ranges"); return result;
    }
    const std::array<std::uintptr_t, 4> code{{0x5d4c14, 0x6b9adc, 0x6d6b7c, 0x9d620}};
    const std::array<qint64, 4> lengths{{0x13c, 0x2a8, 0x168, 0xcc}};
    for (size_t n = 0; n < code.size(); ++n) {
        if (!detail::allowed(permit) || !exe.seek(static_cast<qint64>(image.codeOffsets[n]))) {
            result.refusal = QStringLiteral("native-code-refused"); return result;
        }
        const auto bytesOnDisk = exe.read(lengths[n]);
        if (bytesOnDisk.size() != lengths[n] ||
            std::memcmp(bytesOnDisk.constData(), reinterpret_cast<const void *>(code[n]), static_cast<size_t>(lengths[n]))) {
            result.refusal = QStringLiteral("native-mapped-code-mismatch"); return result;
        }
    }
    if (!detail::allowed(permit)) { result.refusal = QStringLiteral("native-load-refused"); return result; }
    detail::Functions functions;
    functions.resolve = reinterpret_cast<detail::Resolve>(0x5d4c14U);
    functions.acquire = reinterpret_cast<detail::Acquire>(0x6b9adcU);
    functions.request = reinterpret_cast<detail::Request>(0x6d6b7cU);
    functions.release = reinterpret_cast<detail::Release>(0x9d620U);
    functions.idInterface = reinterpret_cast<const void *>(0x14ff5e0U);
    functions.workerMeta = reinterpret_cast<const QMetaObject *>(0xe624d8U);
    result.bridge.reset(new Bridge(functions));
#else
    result.refusal = QStringLiteral("native-unsupported-image");
#endif
    return result;
}
} // namespace reader_worker_native
