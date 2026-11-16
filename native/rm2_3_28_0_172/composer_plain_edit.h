#pragma once
// Private preflight for a joined native write borrow. No mutation or lease.
#include "composer_scene_attachment.h"
#include <QAbstractTextDocumentLayout>

namespace remarkable::composer_writer {
class OwnedEdit;
#ifdef COMPOSER_WRITER_TESTING
namespace testing { struct Access; }
#endif
namespace detail {
enum class PlainFailure {
    None, Unsupported, Join, Thread, MissingLayout, LayoutType, LayoutOwner,
    UndoState, Text, Canceled, Deadline
};
struct PlainSample {
    PlainFailure failure = PlainFailure::None;
    composer_text::Result text;
    bool admitted = false;
};
class PlainProfile final {
    const std::uintptr_t layoutVptr_, layoutRtti_;
    composer_scene::detail::CacheBytes (*readCaches_)(const composer_scene::Join &) =
        composer_scene::detail::readScalarCaches;
    explicit PlainProfile(std::uintptr_t vptr, std::uintptr_t rtti)
        : layoutVptr_(vptr), layoutRtti_(rtti) {}
    friend class remarkable::composer_writer::OwnedEdit;
#ifdef COMPOSER_WRITER_TESTING
    friend struct remarkable::composer_writer::testing::Access;
#endif
    static std::unique_ptr<PlainProfile> load() {
#if defined(__linux__) && defined(__arm__) && defined(__ARM_PCS_VFP) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) && QT_VERSION == QT_VERSION_CHECK(6,10,3)
        auto providers = composer_scene::Profile::load();
        if (!providers) return {};
        Dl_info gui{};
        void *revision = ::dlsym(RTLD_DEFAULT, "_ZNK13QTextDocument8revisionEv");
        if (!revision || !::dladdr(revision, &gui) || !gui.dli_fbase) return {};
        const auto base = reinterpret_cast<std::uintptr_t>(gui.dli_fbase);
        using composer_scene::detail::word;
        const auto vptr = base + 0x477cc0;
        if (word(vptr - 4, 0) != base + 0x477d18 ||
            word(vptr, 0x3c) != base + 0x2d42c8 ||
            word(vptr, 0x44) != base + 0x2d4b28 ||
            word(vptr, 0x48) != base + 0x2d4f80)
            return {};
        return std::unique_ptr<PlainProfile>(new PlainProfile(vptr, base + 0x477d18));
#else
        return {};
#endif
    }
    PlainSample sample(const composer_scene::Join &joined,
            const composer_text::detail::Guard &guard) const {
        PlainSample result;
        auto refuse = [&](PlainFailure why) { result.failure = why; return result; };
        if (const auto why = guard.check(); why != composer_text::Failure::None)
            return refuse(why == composer_text::Failure::Canceled ? PlainFailure::Canceled : PlainFailure::Deadline);
        if (joined.refusal != composer_scene::Refusal::None || !joined.document ||
            !joined.root || !joined.control || !joined.participant || !joined.privateData ||
            joined.facts.documentAddress != reinterpret_cast<std::uintptr_t>(joined.document))
            return refuse(PlainFailure::Join);
        auto &document = *joined.document;
        if (document.thread() != QThread::currentThread()) return refuse(PlainFailure::Thread);
#if defined(COMPOSER_TEXT_PRIVATE_QT) && (QT_VERSION == QT_VERSION_CHECK(6,10,3) || defined(COMPOSER_WRITER_TESTING))
        const auto *p = QTextDocumentPrivate::get(&document);
        if (reinterpret_cast<std::uintptr_t>(p) != joined.privateData || p->isInEditBlock())
            return refuse(PlainFailure::Join);
        // This PRIVATE inline getter only returns the existing lout pointer.
        // QTextDocument::documentLayout() is forbidden here: it creates a
        // missing layout and calls into application code.
        auto *layout = p->layout();
        if (!layout) return refuse(PlainFailure::MissingLayout);
        std::uintptr_t vptr = 0, rtti = 0;
        std::memcpy(&vptr, layout, sizeof vptr);
        if (vptr != layoutVptr_) return refuse(PlainFailure::LayoutType);
        std::memcpy(&rtti, reinterpret_cast<const void *>(vptr - sizeof(void *)), sizeof rtti);
        if (rtti != layoutRtti_) return refuse(PlainFailure::LayoutType);
        if (layout->thread() != QThread::currentThread() || layout->parent() != &document ||
            layout->document() != &document)
            return refuse(PlainFailure::LayoutOwner);
        // Retained r35: avoid first-undo/redo-reset availability transitions.
        // This does not claim that the eventual edit emits no notifications.
        const auto caches = readCaches_(joined);
        if (!caches || (*caches)[0] != 1 || (*caches)[1] != 0 ||
            !document.isUndoRedoEnabled() || !document.isUndoAvailable() ||
            document.isRedoAvailable() ||
            document.maximumBlockCount() != 0 || document.signalsBlocked())
            return refuse(PlainFailure::UndoState);
        const auto count = document.characterCount();
        if (count < 1 || count - 1 > static_cast<int>(composer_text::Result::Capacity))
            return refuse(PlainFailure::Text);
        // Inspect/copy the ENTIRE document. Default layout's size calculation
        // may visit rich objects outside the requested deletion interval.
        result.text = composer_text::detail::copy(document, {0, count - 1}, guard);
        if (result.text.status != composer_text::Status::Copied) return refuse(PlainFailure::Text);
        result.admitted = true;
        return result;
#else
        return refuse(PlainFailure::Unsupported);
#endif
    }
};
} // namespace detail
} // namespace remarkable::composer_writer
