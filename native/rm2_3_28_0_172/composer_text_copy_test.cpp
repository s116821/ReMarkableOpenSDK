#define RESIDENT_WORKER_SLOT_TESTING
#define COMPOSER_SCENE_TESTING
#define COMPOSER_DELIVERY_TESTING
#define COMPOSER_TEXT_TESTING
#include "composer_scene_delivery.h"
#include <QAbstractTextDocumentLayout>
#include <QGuiApplication>
#include <QTextCursor>
#include <QTextTable>
#include <QPainter>
#include <cassert>
#include <future>
#include <iostream>
#include <stdexcept>
using namespace std::chrono_literals;
using namespace remarkable;
namespace text = composer_text;
namespace scene = composer_scene;
namespace remarkable::composer_scene::testing {
struct Access {
    static std::unique_ptr<Profile> profile() { return std::unique_ptr<Profile>(new Profile); }
    static void join(Attachment &a, Join (*fn)(::Scene *)) { a.state_->join = fn; }
    static void hooks(Attachment &a, void *context, void (*after)(void *),
                      void (*publish)(void *) = nullptr, void (*contended)(void *) = nullptr) {
        a.state_->hookContext = context; a.state_->afterTextCopy = after;
        a.state_->beforePublish = publish; a.state_->stopContended = contended;
    }
};
struct DeliveryAccess {
    static auto make(const Profile &p, reader_worker_native::ResidentSlot &slot,
            const reader_worker_native::ResidentSlot::Token &token, SceneRequest fn,
            text::ReadRange range) {
        return Delivery::make(p,slot,token,QStringLiteral("copied-page"),120s,fn,range);
    }
    static Attachment &attachment(Delivery &d) { return *d.state_->attachment; }
};
}
struct Borrow {
    QTextDocument *document;
    unsigned calls = 0;
    scene::Refusal failure = scene::Refusal::None;
    bool drift = false;
};
static scene::Join join(::Scene *raw) {
    auto &b = *reinterpret_cast<Borrow *>(raw); ++b.calls;
    scene::Join value; value.document = b.document;
    value.facts.documentAddress = reinterpret_cast<std::uintptr_t>(b.document);
    value.facts.idHigh = b.drift && b.calls == 2 ? 2 : 1;
    value.refusal = b.failure; return value;
}
static auto prepare(text::ReadRange range, std::chrono::milliseconds budget = 120s) {
    auto p = scene::testing::Access::profile();
    auto a = scene::Attachment::prepare(*p,budget,range);
    if (a) scene::testing::Access::join(*a,join);
    return a;
}
static text::Result read(QTextDocument &doc, text::ReadRange range) {
    std::atomic<bool> canceled{false};
    return text::detail::copy(doc,range,{canceled,nullptr,text::Clock::now()+120s});
}
static void copied(const text::Result &r, const QString &expected) {
    assert(r.status == text::Status::Copied && r.failure == text::Failure::None);
    assert(r.count == static_cast<std::size_t>(expected.size()));
    for (std::size_t i=0;i<r.count;++i) assert(r.units[i] == expected[static_cast<qsizetype>(i)].unicode());
    assert(r.revisionsPresent && r.revisionBefore == r.revisionAfter && r.visits <= 8192);
}
static void refused(const text::Result &r, text::Failure why) {
    assert(r.status == text::Status::Refused && r.failure == why && r.count == 0);
    for (auto unit:r.units) assert(unit == 0);
}
class CalloutLayout final : public QAbstractTextDocumentLayout {
public:
    mutable unsigned calls = 0;
    explicit CalloutLayout(QTextDocument *d):QAbstractTextDocumentLayout(d) {}
    void draw(QPainter *,const PaintContext &) override { ++calls; }
    int hitTest(const QPointF &,Qt::HitTestAccuracy) const override { ++calls; return -1; }
    int pageCount() const override { ++calls; return 1; }
    QSizeF documentSize() const override { ++calls; return {}; }
    QRectF frameBoundingRect(QTextFrame *) const override { ++calls; return {}; }
    QRectF blockBoundingRect(const QTextBlock &) const override { ++calls; return {}; }
protected:
    void documentChanged(int,int,int) override { ++calls; }
};
class Events final : public QObject {
public:
    unsigned calls = 0;
    bool eventFilter(QObject *,QEvent *) override { ++calls; return false; }
};
static void exactUnitsAndNoCallouts() {
    QTextDocument doc;
    auto *layout = new CalloutLayout(&doc); doc.setDocumentLayout(layout);
    unsigned resourceCalls = 0, signalCalls = 0;
    doc.setResourceProvider([&](const QUrl &) { ++resourceCalls; return QVariant{}; });
    QObject::connect(&doc,&QTextDocument::contentsChanged,[&] { ++signalCalls; });
    QTextCursor cursor(&doc);
    const char16_t raw[]{'a',0,0xa0,0x2028,0x2029,'b',0x301,0xd83d,0xdc69,0x200d,0xd83d,0xdcbb,0xd800,'z'};
    const auto expected = QString::fromUtf16(raw,static_cast<qsizetype>(std::size(raw)));
    cursor.insertText(expected);
    const auto revision = doc.revision();
    const bool undo = doc.isUndoAvailable(), redo = doc.isRedoAvailable();
    const auto position = cursor.position(), anchor = cursor.anchor();
    Events events; doc.installEventFilter(&events);
    layout->calls = resourceCalls = signalCalls = 0;
    copied(read(doc,{0,expected.size()}),expected);
    copied(read(doc,{2,2}),expected.mid(2,2));
    refused(read(doc,{8,1}),text::Failure::SplitSurrogate);
    refused(read(doc,{7,1}),text::Failure::SplitSurrogate);
    copied(read(doc,{12,1}),expected.mid(12,1)); // Isolated surrogate is raw data.
    assert(layout->calls == 0 && resourceCalls == 0 && signalCalls == 0 && events.calls == 0);
    assert(doc.revision() == revision && cursor.position() == position && cursor.anchor() == anchor);
    assert(doc.isUndoAvailable() == undo && doc.isRedoAvailable() == redo);
    // A fresh document has no materialized root/layout; the read must not create either.
    QTextDocument empty;
    const auto oldChildren = empty.children();
    const auto *oldLayout = QTextDocumentPrivate::block(empty.begin())->layout;
    copied(read(empty,{0,0}),QString{});
    assert(empty.children() == oldChildren && QTextDocumentPrivate::block(empty.begin())->layout == oldLayout);
}
static void rangeAndBudget() {
    QTextDocument doc; QTextCursor c(&doc); c.insertText(QString(4096,QChar('x')));
    copied(read(doc,{0,4096}),QString(4096,QChar('x')));
    copied(read(doc,{4096,0}),QString{});
    for (auto r : {text::ReadRange{-1,1}, {0,-1}, {0,4097}, {4097,0},
                   {4096,1}, {std::numeric_limits<std::int64_t>::max(),1}})
        refused(read(doc,r),text::Failure::Range);
    assert(!prepare({0,4097}) && !prepare({-1,0}));
    // Both passes share a budget. 4096 distinct-format fragments fit one pass but not two.
    QTextDocument fragmented; QTextCursor cursor(&fragmented);
    for (int i=0;i<4096;++i) {
        QTextCharFormat format; format.setFontItalic(i%2);
        cursor.insertText(QStringLiteral("a"),format);
    }
    const auto result = read(fragmented,{0,4096});
    refused(result,text::Failure::InspectionLimit); assert(result.visits == 8192);
    copied(read(fragmented,{4095,1}),QStringLiteral("a")); // Direct map lookup avoids a hidden prefix scan.
}
static void structureAndComposition() {
    QTextDocument doc; QTextCursor cursor(&doc); cursor.insertText(QStringLiteral("one\ntwo"));
    copied(read(doc,{0,7}),QStringLiteral("one\u2029two"));
    QTextCharFormat style; style.setFontWeight(QFont::Bold);
    cursor.select(QTextCursor::Document); cursor.mergeCharFormat(style);
    copied(read(doc,{0,3}),QStringLiteral("one"));
    auto second = doc.findBlock(4);
    second.layout()->setPreeditArea(0,QStringLiteral("pending"));
    refused(read(doc,{0,4}),text::Failure::Composition); // End point checks next block.
    copied(read(doc,{0,3}),QStringLiteral("one"));
    second.layout()->setPreeditArea(0,QString{});
    for (int kind=0;kind<5;++kind) {
        QTextDocument complex; QTextCursor edit(&complex);
        edit.insertText(QStringLiteral("prefix"));
        int start = 0;
        if (kind == 0) { auto *t=edit.insertTable(1,1); start=t->firstPosition(); edit.insertText("cell"); }
        if (kind == 1) { auto *f=edit.insertFrame(QTextFrameFormat{}); start=f->firstPosition(); edit.insertText("frame"); }
        if (kind == 2) { edit.createList(QTextListFormat::ListDisc); start=0; }
        if (kind == 3) { start=edit.position(); edit.insertImage(QStringLiteral("unfetched")); }
        if (kind == 4) {
            start=edit.position(); QTextCharFormat object;
            object.setObjectType(QTextFormat::UserObject+1); edit.insertText("Q",object);
        }
        refused(read(complex,{start,0}),text::Failure::Structure);
        if (start < complex.characterCount()-1) refused(read(complex,{start,1}),text::Failure::Structure);
    }
    QTextDocument withFrame; QTextCursor edit(&withFrame);
    auto *frame = edit.insertFrame(QTextFrameFormat{}); edit.insertText("inside");
    edit.setPosition(frame->lastPosition()+1); edit.insertText("outside");
    copied(read(withFrame,{frame->lastPosition()+1,7}),QStringLiteral("outside"));
}
static void guardsAndDrift() {
    QTextDocument doc; doc.setPlainText("abc");
    std::thread foreign([&] { refused(read(doc,{0,1}),text::Failure::Thread); }); foreign.join();
    class Derived final:public QTextDocument {} derived;
    refused(read(derived,{0,0}),text::Failure::Type);
    QTextCursor cursor(&doc); cursor.beginEditBlock();
    refused(read(doc,{0,1}),text::Failure::Transaction); cursor.endEditBlock();
    for (int kind=0;kind<4;++kind) {
        auto a=prepare({0,1}); Borrow borrow{&doc};
        if (kind==0) {
            scene::testing::Access::hooks(*a,&doc,[](void *p) {
                QTextCursor edit(static_cast<QTextDocument *>(p)); edit.insertText("changed");
            });
        } else if (kind==1) borrow.drift=true;
        else if (kind==2) borrow.failure=scene::Refusal::NativeTransaction;
        else scene::testing::Access::hooks(*a,nullptr,[](void *) { throw std::runtime_error("fixture"); });
        assert(!a->callback()(reinterpret_cast<::Scene *>(&borrow)));
        auto result=a->snapshot(); assert(result.retired && result.text.status==text::Status::Refused);
        assert(result.text.count==0);
        if (kind==0) assert(result.text.failure==text::Failure::Revision);
        if (kind==1) assert(result.text.failure==text::Failure::Join);
    }
}
static void cancellationAndHistory() {
    QTextDocument doc; doc.setPlainText("abc");
    auto a=prepare({0,3}); Borrow borrow{&doc};
    auto callback=a->callback();
    assert(!callback(reinterpret_cast<::Scene *>(&borrow)));
    copied(a->snapshot().text,QStringLiteral("abc")); assert(borrow.calls==2);
    assert(!callback(reinterpret_cast<::Scene *>(1)));
    copied(a->snapshot().text,QStringLiteral("abc"));
    a->stop(); doc.setPlainText("new"); copied(a->snapshot().text,QStringLiteral("abc"));
    a.reset(); assert(!callback(reinterpret_cast<::Scene *>(1)));
    auto stopped=prepare({0,1}); auto late=stopped->callback(); stopped->stop();
    assert(!late(reinterpret_cast<::Scene *>(1))); refused(stopped->snapshot().text,text::Failure::Canceled);
    auto expires=prepare({0,1},1ms);
    std::this_thread::sleep_until(expires->deadline()+1ms);
    assert(!expires->callback()(reinterpret_cast<::Scene *>(1)));
    refused(expires->snapshot().text,text::Failure::Deadline);

    struct Phases {
        std::promise<void> entered, blocked, release;
        std::shared_future<void> permit=release.get_future().share();
    } phases;
    auto active=prepare({0,3});
    scene::testing::Access::hooks(*active,&phases,nullptr,[](void *p) {
        auto &v=*static_cast<Phases *>(p); v.entered.set_value(); v.permit.wait();
    },[](void *p) { static_cast<Phases *>(p)->blocked.set_value(); });
    auto entered=phases.entered.get_future(), blocked=phases.blocked.get_future();
    std::thread emitter([&] {
        QTextDocument owned; owned.setPlainText("abc"); Borrow b{&owned};
        assert(!active->callback()(reinterpret_cast<::Scene *>(&b)));
    });
    assert(entered.wait_for(5s)==std::future_status::ready);
    std::atomic<bool> returned{false};
    std::thread closer([&] { active->stop(); returned=true; });
    assert(blocked.wait_for(5s)==std::future_status::ready && !returned.load());
    phases.release.set_value(); emitter.join(); closer.join();
    auto settled=active->snapshot(); assert(settled.retired);
    refused(settled.text,text::Failure::Canceled);
}
struct NativeId { std::uint32_t words[3]{1,2,3}; };
Q_DECLARE_METATYPE(NativeId)
namespace xofm::libs::library { class DocumentController final:public QObject { Q_OBJECT }; }
class DocumentWorker final:public QThread { Q_OBJECT };
namespace reader_worker_native::testing {
struct SlotAccess {
    static ResidentSlot *make(detail::Functions fn) { return new ResidentSlot(fn,nullptr); }
    static void destroy(ResidentSlot *p) { delete p; }
    static auto retire(ResidentSlot &s,const ResidentSlot::Token &t) { return s.retirePhysically(t); }
};
}
static void typedDelivery() {
    using Slot=reader_worker_native::ResidentSlot;
    static DocumentWorker worker;
    static int document,control,workerControl;
    static scene::Callback retained;
    auto pair=[](void *out,void *value,void *owner) {
        std::array<std::uintptr_t,2> words{reinterpret_cast<std::uintptr_t>(value),reinterpret_cast<std::uintptr_t>(owner)};
        std::memcpy(out,words.data(),sizeof words);
    };
    static auto put=pair;
    reader_worker_native::detail::Functions fn{
        [](void *out,void *,const void *) { put(out,&document,&control); },
        [](void *out,void *,int) { put(out,&worker,&workerControl); },
        [](void *,const std::function<void()> &)->qulonglong { std::abort(); },
        [](void *) { std::abort(); }, QMetaType::fromType<NativeId>().iface(),&DocumentWorker::staticMetaObject};
    xofm::libs::library::DocumentController controller;
    auto profile=scene::testing::Access::profile();
    for (int cancel=0;cancel<2;++cancel) {
        std::unique_ptr<Slot,void(*)(Slot*)> slot(reader_worker_native::testing::SlotAccess::make(fn),
            reader_worker_native::testing::SlotAccess::destroy);
        auto ready=slot->prepare(&controller,QVariant::fromValue(NativeId{}),&worker,[]{return true;});
        assert(ready.ready);
        auto delivery=scene::testing::DeliveryAccess::make(*profile,*slot,ready.token,
            [](void *,const QString &,const scene::Callback &c)->qulonglong { retained=c; return 0x100000001ULL; },
            {0,3});
        assert(delivery);
        auto &attachment=scene::testing::DeliveryAccess::attachment(*delivery);
        scene::testing::Access::join(attachment,join);
        std::promise<void> entered,release;
        auto waiting=entered.get_future(); auto permission=release.get_future().share();
        struct Hook { std::promise<void> *entered; std::shared_future<void> permission; } hook{&entered,permission};
        if (cancel) scene::testing::Access::hooks(attachment,&hook,[](void *p) {
            auto &v=*static_cast<Hook *>(p);v.entered->set_value();v.permission.wait();
        });
        assert(delivery->submit([]{return true;}).accepted);
        std::thread emitter([&] {
            QTextDocument doc; doc.setPlainText("abc"); Borrow borrow{&doc};
            assert(!retained(reinterpret_cast<::Scene *>(&borrow)));
        });
        if(cancel) {
            assert(waiting.wait_for(5s)==std::future_status::ready);
            assert(slot->cancel(ready.token)); release.set_value();
        }
        emitter.join();
        const auto result=delivery->snapshot();
        if(cancel) refused(result.attachment.text,text::Failure::Canceled);
        else copied(result.attachment.text,QStringLiteral("abc"));
        assert(result.submission.jobId==0x100000001ULL && result.ownersPinned);
        auto retirement=reader_worker_native::testing::SlotAccess::retire(*slot,ready.token);
        assert(!retirement.reclaimed && retirement.documentRetained && retirement.workerRetained);
        delivery.reset(); assert(!retained(reinterpret_cast<::Scene *>(1))); retained={};
    }
}
int main(int argc,char **argv) {
    QGuiApplication app(argc,argv);
    exactUnitsAndNoCallouts(); rangeAndBudget(); structureAndComposition();
    guardsAndDrift(); cancellationAndHistory(); typedDelivery();
    std::cout<<"text copy: 6 owned Unicode/structure/guard/settlement/delivery groups passed; Qt "
             <<qVersion()<<"; no native execution\n";
}
#include "composer_text_copy_test.moc"
