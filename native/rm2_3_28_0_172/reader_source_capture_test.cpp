// Actual guarded capture/encoder/order reader with explicitly modeled join
// delivery. The separately imported identity fixture exercises real QML joins.
#define READER_IDENTITY_CAPTURE_TESTING
#define VISUAL_CAPTURE_TESTING
#define main retained_post_order_tests
#include "reader_insertion_post_order_fixture.cpp"
#undef main
#include "reader_source_capture.h"
#include "window_visual_capture.h"
#include <QImageReader>
#include <QBuffer>
#include "../resident/endpoint.h"
#include <QTemporaryDir>
#include <QQuickPaintedItem>
#include <QPainter>

using remarkable::rm2_3_28_0_172::captureSource;
using remarkable::rm2_3_28_0_172::sameFacts;
struct CaptureFixture : Fixture {
    QQuickWindow window;
    QObject reader, controller;
    bool live = true;
    unsigned samples = 0, grabs = 0;
    std::function<void(unsigned)> before;
    explicit CaptureFixture(int count = 3) {
        doc->pages.clear();
        for (int i = 1; i <= count; ++i) doc->pages.append(pageId(i));
        joined.pageCount = count; joined.pageIndex = count / 2;
        joined.pageId = doc->pages[joined.pageIndex];
        joined.status = joined.pageStatus = 0;
        QQmlEngine::setContextForObject(&window, engine->rootContext());
        window.resize(160, 120); window.show(); window.requestActivate();
        QCoreApplication::processEvents();
        check(window.isActive() && QGuiApplication::focusWindow() == &window, "owned capture window active");
    }
    remarkable::resident::Facts facts() const {
        remarkable::resident::Facts result;
        result.document = QUuid(joined.documentId).toRfc4122();
        result.page = QUuid(joined.pageId).toRfc4122();
        for (const auto &page : doc->pages) result.order.push_back(QUuid(page).toRfc4122());
        return result;
    }
    reader_identity_capture::TestBoundary boundary() {
        reader_identity_capture::TestBoundary test;
        test.grab = [&](QQuickWindow *w) {
            check(w == &window, "capture uses original window"); ++grabs;
            QImage image(32, 24, QImage::Format_ARGB32); image.fill(QColor(13, 43, 91)); return image;
        };
        test.sample = [&](QQuickWindow *w, const reader_identity_capture::Guard &permit, int budget,
                          const reader_selected::ValidatedSourceAction &action) {
            check(w == &window && budget > 0, "original modeled join scope");
            const auto count = ++samples;
            if (before) before(count);
            auto active = std::make_shared<bool>(true);
            const auto close = qScopeGuard([active] { *active = false; });
            const auto guard = [&, active, permit] { return *active && live && permit(); };
            auto observation = joined;
            if (guard()) action(&reader, doc.get(), &controller, id, observation, guard);
            return observation;
        };
        return test;
    }
};
class OwnedCanvas final : public QQuickPaintedItem {
public:
    QImage pixels{160,120,QImage::Format_RGBA8888};
    explicit OwnedCanvas(QQuickItem *parent):QQuickPaintedItem(parent) {
        setWidth(160);setHeight(120);quint32 state=0x12345678;
        for(int y=0;y<120;++y)for(int x=0;x<160;++x) {
            state^=state<<13;state^=state>>17;state^=state<<5;
            pixels.setPixelColor(x,y,QColor(int(state&255),int((state>>8)&255),int((state>>16)&255)));
        }
    }
    void paint(QPainter *painter)override {painter->drawImage(0,0,pixels);}
};
static int serveOwnedCapture(QGuiApplication &app,int peerPid,bool reorder) {
    using namespace remarkable::resident;
    CaptureFixture fixture;OwnedCanvas canvas(fixture.window.contentItem());
    QTemporaryDir directory;check(directory.isValid(),"owned server directory");
    EndpointConfig config;config.directory=directory.path();config.peerUid=::geteuid();
    config.peerPid=peerPid;config.peerStart=processStart(peerPid);config.operationBudgetMs=20000;
    detail::Writer runtime;runtime.bytes=QByteArray(32,1)+QByteArray(32,2)+QByteArray(32,3)+bootIdentity();
    runtime.number<quint32>(quint32(::getpid()));runtime.number<quint64>(processStart(::getpid()));runtime.bytes+=Id(16,7);
    config.runtime=runtime.bytes;
    Endpoint *endpoint=nullptr;unsigned submitted=0;
    Endpoint server(config,[&](Endpoint::Lease lease) {
        ++submitted;Frame reply;
        if(lease->request().kind==0) {reply.kind=0;reply.facts=fixture.facts();}
        else if(lease->request().kind==5) {
            auto test=fixture.boundary();
            test.grab=[&](QQuickWindow *window){++fixture.grabs;return window->grabWindow();};
            if(reorder)fixture.before=[&](unsigned n){if(n==2)fixture.doc->pages.swapItemsAt(0,2);};
            reply=remarkable::rm2_3_28_0_172::captureReply(captureSource(&fixture.window,lease->request().facts,
                [lease]{return lease->active();},20000,test));
        } else if(lease->request().kind==6) {
            // Deliberately unusable Reader facts: visual evidence needs none.
            fixture.doc->pages.clear();
            reply=remarkable::rm2_3_28_0_172::captureVisual(&fixture.window,[lease]{return lease->active();},20000,
                [&](QQuickWindow *window){++fixture.grabs;return window->grabWindow();});
        } else {reply.kind=3;reply.rejection=0;}
        check(endpoint->finish(lease,std::move(reply)),"owned producer reply delivered");
    });endpoint=&server;check(server.start(),"owned capture endpoint start");
    Frame hello;hello.runtime=config.runtime;hello.client=Id(16,8);hello.server=Id(16,0);hello.operation=Id(16,9);
    const auto bytes=encode(hello);
    std::printf("%s\n",QFile::encodeName(directory.path()).constData());std::fflush(stdout);
    check(::write(STDOUT_FILENO,bytes.constData(),std::size_t(bytes.size()))==bytes.size(),"owned bootstrap frame");
    QSocketNotifier shutdown(STDIN_FILENO,QSocketNotifier::Read,&app);
    QObject::connect(&shutdown,&QSocketNotifier::activated,&app,[&]{
        char byte=0;check(::read(STDIN_FILENO,&byte,1)==1&&byte=='Q',"owned fixture shutdown");
        std::printf("submissions=%u samples=%u grabs=%u\n",submitted,fixture.samples,fixture.grabs);std::fflush(stdout);app.quit();
    });
    QTimer::singleShot(25000,&app,&QCoreApplication::quit);return app.exec();
}
int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    qRegisterMetaType<entry::Id>("entry::Id");
    QMetaType::registerConverter<entry::Id, QString>([](const entry::Id &id) { return id.text; });
    if(argc==4&&QByteArray(argv[1])=="--rust-server") {
        bool okay=false;const auto pid=QString::fromLocal8Bit(argv[2]).toInt(&okay);
        check(okay&&pid>1,"owned peer pid");return serveOwnedCapture(app,pid,QByteArray(argv[3])=="reorder");
    }
    for (int count : {1, 3, 16, 17}) {
        CaptureFixture f(count); const auto expected = f.facts();
        const auto result = captureSource(&f.window, expected, [&] { return f.live; }, 20000, f.boundary());
        check(result.captured() && f.samples == 2 && f.grabs == 1, "two joins and one grab publish owned PNG");
        check(sameFacts(result.before, expected) && sameFacts(result.after, expected), "both complete order samples retained");
        check(result.png.size() == result.identity.encoded &&
              QCryptographicHash::hash(result.png, QCryptographicHash::Sha256).toHex() == result.identity.sha, "exact private image publication");
        QBuffer buffer; buffer.setData(result.png); buffer.open(QIODevice::ReadOnly);
        const auto decoded = QImageReader(&buffer, "PNG").read();
        check(decoded.size() == QSize(32, 24), "published PNG decodes");
        check(!result.identity.nativeAuthority && !result.identity.durableAuthority, "historical capture grants no authority");
    }
    for (int fault = 0; fault < 8; ++fault) {
        CaptureFixture f; auto expected = f.facts();
        if (fault == 0) std::swap(expected.order[0], expected.order[2]);
        if (fault == 1) expected.document = QUuid(pageId(90)).toRfc4122();
        if (fault == 2) f.before = [&](unsigned n) { if (n == 2) f.doc->pages.swapItemsAt(0, 2); };
        if (fault == 3) f.before = [&](unsigned n) { if (n == 2) f.live = false; };
        if (fault == 4) f.before = [&](unsigned n) { if (n == 2) f.doc->badInverse = true; };
        if (fault == 5) f.before = [&](unsigned n) { if (n == 2) f.doc = std::make_unique<QmlDocumentWrapper>(); };
        if (fault == 6) f.doc->onRead = [&](const QString &, unsigned n) { if (n == 4) f.live = false; };
        if (fault == 7) f.live = false;
        const auto result = captureSource(&f.window, expected, [&] { return f.live; }, 20000, f.boundary());
        check(!result.captured() && result.png.isEmpty(), "refusal never publishes intermediate PNG");
        if (fault >= 2 && fault <= 5) check(f.grabs == 1 && result.identity.attempted, "final refusal follows completed private encoding");
        else check(f.grabs == 0, "initial refusal precedes grab");
    }
    {
        CaptureFixture f(18);
        const auto result = captureSource(&f.window, f.facts(), [] { return true; }, 20000, f.boundary());
        check(!result.captured() && result.png.isEmpty() && f.samples == 0, "profile count bound before join");
    }
    for (int budget : {0, 20001}) {
        CaptureFixture f;
        check(!captureSource(&f.window, f.facts(), [] { return true; }, budget, f.boundary()).captured(), "invalid original budget refuses");
        check(f.samples == 0, "invalid budget admits no native read");
    }
    {
        CaptureFixture f; const auto expected = f.facts();
        bool refused = false;
        std::thread worker([&] {
            const auto result = captureSource(&f.window, expected, [] { return true; }, 20000);
            refused = !result.captured() && result.identity.refusal == QStringLiteral("source-gui");
        });
        worker.join();
        check(refused && f.samples == 0, "wrong thread refuses before window or engine access");
    }
    {
        CaptureFixture f; auto test = f.boundary();
        test.grab = [&](QQuickWindow *) {
            ++f.grabs;
            const auto nested = captureSource(&f.window, f.facts(), [] { return true; }, 20000, f.boundary());
            check(!nested.captured() && nested.png.isEmpty(), "nested capture refused");
            return QImage(32, 24, QImage::Format_ARGB32);
        };
        check(!captureSource(&f.window, f.facts(), [] { return true; }, 20000, test).captured(), "nested capture poisons outer result");
    }
    {
        visual_capture::Sink sink;
        check(sink.write(QByteArray(visual_capture::PngCap, 'x')) == visual_capture::PngCap, "owned sink accepts exact quota");
        check(sink.write("x", 1) == -1 && sink.takeOwned().isEmpty(), "over-quota sink cannot publish prefix");
    }
    for (int fault : {0, 1, 2}) {
        CaptureFixture f; const auto expected = f.facts(); bool injected = false;
        const auto result = captureSource(&f.window, expected, [&] {
            if (f.samples == 2 && !reader_identity_capture::detail::running && !injected) {
                injected = true;
                if (fault == 0) {
                    const auto nested = captureSource(&f.window, expected, [] { return true; }, 20000, f.boundary());
                    check(!nested.captured() && nested.png.isEmpty(), "post-join publication reentry refused");
                } else if (fault == 1) f.doc.reset();
                else f.window.hide();
            }
            return true;
        }, 20000, f.boundary());
        check(injected && !result.captured() && result.png.isEmpty() && f.grabs == 1,
              "post-run reentry, owner loss or context loss discards encoded bytes");
    }
    {
        CaptureFixture f; auto test = f.boundary(); const auto sample = test.sample; bool injected = false;
        test.sample = [&](QQuickWindow *window, const reader_identity_capture::Guard &permit, int budget,
                          const reader_selected::ValidatedSourceAction &action) {
            if (f.samples == 0) return sample(window, permit, budget, action);
            ++f.samples;
            const auto joined = f.joined;
            action(&f.reader, f.doc.get(), &f.controller, f.id, joined, [&] {
                injected = true; f.doc.reset(); return true;
            });
            return joined;
        };
        const auto result = captureSource(&f.window, f.facts(), [] { return true; }, 20000, test);
        check(injected && !result.captured() && result.png.isEmpty() && f.grabs == 1,
              "second borrowed guard owner deletion is checked before source callback");
    }
    // Sweep the caller-controlled admission points before the first order read.
    // This exercises owner deletion both before and during sourceCheck entry.
    unsigned firstReadPermits = 0;
    {
        CaptureFixture f; unsigned permits = 0;
        f.doc->onRead = [&](const QString &, unsigned) { if (!firstReadPermits) firstReadPermits = permits; };
        const auto result = captureSource(&f.window, f.facts(), [&] { if (f.samples == 1) ++permits; return true; }, 20000, f.boundary());
        check(result.captured() && firstReadPermits > 0, "first order read admission points measured");
    }
    for (unsigned at = 1; at <= firstReadPermits; ++at) {
        CaptureFixture f; unsigned permits = 0; bool injected = false;
        const auto result = captureSource(&f.window, f.facts(), [&] {
            if (f.samples == 1 && ++permits == at) { injected = true; f.doc.reset(); }
            return true;
        }, 20000, f.boundary());
        check(injected && !result.captured() && result.png.isEmpty() && f.grabs == 0,
              "owner deletion at first source admission cannot publish or dereference released owner");
    }
    using remarkable::rm2_3_28_0_172::captureVisual;
    unsigned visualPermits=0;
    {
        CaptureFixture f;
        auto result=captureVisual(&f.window,[&]{++visualPermits;return true;},20000,f.boundary().grab);
        check(result.kind==8&&result.capture.procedure==2&&result.facts.order.empty()&&f.samples==0&&f.grabs==1,
              "visual evidence contains no Reader facts or joins");
        QBuffer buffer;buffer.setData(result.capture.png);buffer.open(QIODevice::ReadOnly);
        check(QImageReader(&buffer,"PNG").read().size()==QSize(32,24),"visual PNG fully decodes");
    }
    for(unsigned at=1;at<=visualPermits;++at)for(int fault=0;fault<3;++fault) {
        CaptureFixture f;unsigned permits=0;bool injected=false;
        auto result=captureVisual(&f.window,[&]{
            if(++permits==at){
                injected=true;
                if(fault==0)return false;
                if(fault==1)f.window.hide();
                if(fault==2){
                    const auto nested=captureSource(&f.window,f.facts(),[]{return true;},20000,f.boundary());
                    check(!nested.captured(),"nested source during visual refuses");
                }
            }
            return true;
        },20000,f.boundary().grab);
        check(injected&&result.kind==3&&result.capture.png.isEmpty(),"every visual admission refuses lost permit/context/reentry");
    }
    for(int fault=0;fault<5;++fault) {
        CaptureFixture f;
        auto result=captureVisual(&f.window,[]{return true;},fault==0?1:20000,[&](QQuickWindow*){
            if(fault==0)std::this_thread::sleep_for(std::chrono::milliseconds(3));
            if(fault==1)throw std::runtime_error("owned grab fault");
            if(fault==2)f.window.hide();
            if(fault==3){auto nested=captureVisual(&f.window,[]{return true;},20000);check(nested.kind==3,"nested visual refuses");}
            if(fault==4)return QImage(1873,1,QImage::Format_ARGB32);
            QImage image(32,24,QImage::Format_ARGB32);image.fill(Qt::black);return image;
        });
        check(result.kind==3&&result.capture.png.isEmpty(),"late/throw/hidden/nested/oversized grab cannot publish");
    }
    // Delete the actual pinned window/root/engine, including final admission
    // points after encoding. QPointer checks must precede any later access.
    for(unsigned at=1;at<=visualPermits;++at) {
        auto fixture=std::make_unique<CaptureFixture>();
        auto *window=&fixture->window;unsigned permits=0;bool injected=false;
        const auto result=captureVisual(window,[&]{
            if(++permits==at){injected=true;fixture.reset();}
            return true;
        },20000,[](QQuickWindow*){
            QImage image(32,24,QImage::Format_ARGB32);image.fill(Qt::black);return image;
        });
        check(injected&&result.kind==3&&result.capture.png.isEmpty(),"deleted visual owners cannot publish or be dereferenced");
    }
    {
        CaptureFixture f;auto test=f.boundary();
        test.grab=[&](QQuickWindow*){
            check(captureVisual(&f.window,[]{return true;},20000).kind==3,"nested visual during source refuses");
            QImage image(32,24,QImage::Format_ARGB32);image.fill(Qt::black);return image;
        };
        check(!captureSource(&f.window,f.facts(),[]{return true;},20000,test).captured(),"visual reentry poisons source");
    }
    QTextStream(stdout) << "PASS SDK owned source/visual capture checks=" << checks << "; modeled joins; no native qualification\n";
}
