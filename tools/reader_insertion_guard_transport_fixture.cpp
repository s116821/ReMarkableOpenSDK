// Owned file protocol and explicitly synthetic serializer inputs only.
// No Coordinator success, hook or native action is modeled.
#define READER_INSERTION_GUARD_TRANSPORT_TESTING 1
#include "reader_insertion_guard_transport.h"
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTextStream>
#include <memory>

namespace reader_insertion_transport {
struct TestAccess {
    static bool trusted(Entry &e) { return e.trusted(); }
    static bool read(Entry &e, const char *name, const QByteArray &bytes) { return e.read(name, bytes); }
    static void tick(Entry &e) { e.tick(); }
    static void bind(Entry &e, QQuickWindow *window) {
        if (!e.read("selected-request", e.config_.nonce + ' ' + e.identity_, &e.requestStat_)) qFatal("owned request binding");
        e.consumed_ = true; e.begin_ = e.clock_.elapsed(); e.deadline_ = Entry::Clock::now() + std::chrono::seconds(30);
        e.window_ = window; e.engine_ = qmlEngine(window); e.heartbeat_.stop(); e.opportunity_.stop();
    }
    static bool publish(Entry &e, const QByteArray &bytes, bool positive = true) {
        return e.publish("selected-result", ".pending-result", bytes, positive);
    }
    static void terminal(Entry &e) { e.terminal_ = true; }
    static void expire(Entry &e) { e.deadline_ = Entry::Clock::now() - std::chrono::milliseconds(1); }
    static void expireSetup(Entry &e) { e.setupDeadline_ = Entry::Clock::now() - std::chrono::milliseconds(1); }
    static void failure(Entry &e) { e.failure_ = QStringLiteral("owned-failure"); }
    static void finish(Entry &e) { e.failure_ = QStringLiteral("owned-wire-only"); e.finish(); }
    static QJsonObject syntheticWire(Entry &e,const reader_insertion_entry::Result &r,bool success) {
        e.navigationAttempted_=true;e.navigationDispatched_=true;return e.wireBody(r,success);
    }
    static void hooks(Entry &e, std::function<void()> pre, std::function<void()> post) { e.preLink_ = std::move(pre); e.postLink_ = std::move(post); }
};
}
namespace {
using Entry = reader_insertion_transport::Entry;
using Access = reader_insertion_transport::TestAccess;
int checks = 0;
void check(bool yes, const char *what) { if (!yes) qFatal("FAIL %s", what); ++checks; }
void put(const QString &path, const QByteArray &bytes, mode_t mode = 0600) {
    const auto name = path.toLocal8Bit();
    const int fd = ::open(name.constData(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, mode);
    check(fd >= 0 && ::fchmod(fd, mode) == 0, "owned exclusive file");
    check(::write(fd, bytes.constData(), size_t(bytes.size())) == bytes.size(), "owned complete file write");
    check(::close(fd) == 0, "owned file close");
}
QByteArray get(const QString &path) { QFile f(path); return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray(); }
struct stat meta(const QString &path) { struct stat st{}; check(::lstat(path.toLocal8Bit().constData(), &st) == 0, "owned stat"); return st; }
void pump() { for (int i = 0; i < 3; ++i) QCoreApplication::processEvents(QEventLoop::AllEvents, 1); }
reader_insertion_transport::Config configuration() {
    reader_insertion_transport::Config c; c.nonce = QByteArray(32, 'a');
    auto &r = c.coordinator.request;
    r.documentId = QStringLiteral("10000000-0000-0000-0000-000000000100");
    for (int i = 1; i <= 4; ++i) r.expectedPages.append(QStringLiteral("10000000-0000-0000-0000-%1").arg(i, 12, 10, QLatin1Char('0')));
    r.sourceIndex = 1; r.sourcePageId = r.expectedPages[1]; r.targetPageId = QStringLiteral("10000000-0000-0000-0000-000000000099");
    c.coordinator.wrongSourceIndex = 2; c.navigation = {r.documentId, 1, true}; return c;
}
struct Files {
    QTemporaryDir base;
    QString root, held;
    QByteArray identity = reader_selected::selfIdentity();
    reader_insertion_transport::Config config = configuration();
    Files() {
        check(base.isValid() && !identity.isEmpty(), "owned base and actual PID/start");
        root = base.path() + "/root"; held = base.path() + "/held";
        check(QDir().mkdir(root) && ::chmod(root.toLocal8Bit().constData(), 0700) == 0, "private owned root");
        put(root + "/owner", config.nonce); put(root + "/attempt.identity", identity);
    }
    QByteArray request() const { return config.nonce + ' ' + identity; }
    void replaceRoot() {
        check(QDir().rename(root, held) && QDir().mkdir(root) && ::chmod(root.toLocal8Bit().constData(), 0700) == 0, "owned root replacement");
        put(root + "/owner", config.nonce); put(root + "/attempt.identity", identity); put(root + "/selected-request", request());
    }
};
void startupCase(const QByteArray &mode, QGuiApplication &app) {
    Files f;
    if (mode == "root-mode") check(::chmod(f.root.toLocal8Bit().constData(), 0755) == 0, "root mode changed");
    if (mode == "root-symlink") {
        check(QDir().rename(f.root, f.held) && ::symlink(f.held.toLocal8Bit().constData(), f.root.toLocal8Bit().constData()) == 0, "owned root symlink");
    }
    if (mode == "owner-mode") check(::chmod((f.root + "/owner").toLocal8Bit().constData(), 0644) == 0, "owner mode changed");
    if (mode == "owner-link") check(::link((f.root + "/owner").toLocal8Bit().constData(), (f.root + "/owner-link").toLocal8Bit().constData()) == 0, "owner hardlink");
    if (mode == "owner-symlink") {
        check(QFile::rename(f.root + "/owner", f.root + "/owner-real"), "preserve owner");
        check(::symlink("owner-real", (f.root + "/owner").toLocal8Bit().constData()) == 0, "owner symlink");
    }
    if (mode == "wrong-nonce") f.config.nonce[0] = 'b';
    if (mode == "nonce-case") f.config.nonce[0] = 'A';
    if (mode == "identity") { check(QFile::remove(f.root + "/attempt.identity"), "remove identity"); put(f.root + "/attempt.identity", "2 3\n"); }
    if (mode == "ready-existing") put(f.root + "/selected-ready", "preserved");
    if (mode == "result-existing") put(f.root + "/selected-result", "preserved");
    if (mode == "pending-existing") put(f.root + "/.pending-ready", "preserved");
    if (mode == "cancel") put(f.root + "/selected-cancel", "stop");
    if (mode == "restore") check(::symlink("nonexistent", (f.root + "/restore.claim").toLocal8Bit().constData()) == 0, "restore symlink present");
    Entry e(&app, f.root, f.config, {});
    const bool expected = mode == "valid";
    check(e.start() == expected && !e.start(), "startup once and strict file contract");
    if (expected) {
        check(get(f.root + "/selected-ready") == f.request(), "ready exact nonce PID/start bytes");
        const auto s = meta(f.root + "/selected-ready");
        check(S_ISREG(s.st_mode) && (s.st_mode & 07777) == 0600 && s.st_uid == ::geteuid() && s.st_nlink == 1, "ready private singlelink");
        check(Access::trusted(e) && !QFile::exists(f.root + "/selected-result"), "no result or native action before request");
    } else if (mode == "ready-existing") check(get(f.root + "/selected-ready") == "preserved", "ready never overwritten");
    QTextStream(stdout) << "PASS start-" << mode << '\n';
}
void requestCase(const QByteArray &mode, QGuiApplication &app) {
    Files f; Entry e(&app, f.root, f.config, {}); check(e.start(), "request fixture starts");
    const QString path = f.root + "/selected-request";
    if (mode == "fifo") check(::mkfifo(path.toLocal8Bit().constData(), 0600) == 0, "request FIFO");
    else if (mode == "directory") check(QDir().mkdir(path), "request directory");
    else if (mode == "symlink") { put(f.root + "/real-request", f.request()); check(::symlink("real-request", path.toLocal8Bit().constData()) == 0, "request symlink"); }
    else {
        QByteArray bytes = f.request();
        if (mode == "wrong-nonce") bytes[0] = 'b';
        if (mode == "wrong-pid") bytes = f.config.nonce + " 2 3\n";
        if (mode == "partial") bytes.chop(1);
        if (mode == "oversize") bytes.append(QByteArray(80, 'p'));
        put(path, bytes, mode == "mode" ? 0644 : 0600);
        if (mode == "hardlink") check(::link(path.toLocal8Bit().constData(), (f.root + "/request-link").toLocal8Bit().constData()) == 0, "request hardlink");
    }
    check(!Access::read(e, "selected-request", f.request()), "request rejects shape type mode identity bytes");
    Access::tick(e); check(e.terminal(), "malformed request reaches terminal once without navigation");
    const auto result = QJsonDocument::fromJson(get(f.root + "/selected-result")).object();
    check(result["stage"] == "insertion-refusal-incomplete" && !result["creation_attempted"].toBool() && !result["navigation_attempted"].toBool() &&
        !result["native_authority"].toBool() && !result["render_authority"].toBool() && !result["durable_authority"].toBool(), "negative wire distinct from native proof");
    check(!result.contains("post_reject_probe"), "default wire has no post-rejection extension");
    QTextStream(stdout) << "PASS request-" << mode << '\n';
}
void publicationCase(const QByteArray &mode, QGuiApplication &app) {
    Files f; QQmlEngine engine; auto window = std::make_unique<QQuickWindow>();
    QQmlEngine::setContextForObject(window.get(), engine.rootContext()); window->show(); window->requestActivate(); pump();
    Entry e(&app, f.root, f.config, {}); check(e.start(), "publication fixture starts");
    put(f.root + "/selected-request", f.request()); Access::bind(e, window.get()); Access::terminal(e);
    check(Access::trusted(e), "exact consumed request binding");
    const QByteArray payload = mode == "cap4096" ? QByteArray(4096, 'p') : mode == "cap4097" ? QByteArray(4097, 'p') :
        QByteArray("owned-transport-only; not-native-success\n");
    if (mode == "expired") Access::expire(e);
    if (mode == "setup-expired") Access::expireSetup(e);
    if (mode == "failure") Access::failure(e);
    if (mode == "window-death") window.reset();
    if (mode == "request-replaced") { check(QFile::rename(f.root + "/selected-request", f.root + "/old-request"), "preserve admitted request"); put(f.root + "/selected-request", f.request()); }
    if (mode == "root-replaced") f.replaceRoot();
    if (mode == "result-exists") put(f.root + "/selected-result", "preserved");
    if (mode == "pending-exists") put(f.root + "/.pending-result", "preserved");
    if (mode == "pending-symlink") { put(f.root + "/outside", "preserved"); check(::symlink("outside", (f.root + "/.pending-result").toLocal8Bit().constData()) == 0, "pending symlink"); }
    auto change = [&] {
        if (mode.endsWith("cancel")) put(f.root + "/selected-cancel", "stop");
        if (mode.endsWith("restore")) put(f.root + "/restore.claim", "restore");
        if (mode.endsWith("expiry")) Access::expire(e);
        if (mode.endsWith("root")) f.replaceRoot();
        if (mode.endsWith("request")) { check(QFile::rename(f.root + "/selected-request", f.root + "/old-request"), "preserve consumed request"); put(f.root + "/selected-request", f.request()); }
        if (mode == "pre-pending-replace") { check(QFile::rename(f.root + "/.pending-result", f.root + "/saved-pending"), "preserve pending"); put(f.root + "/.pending-result", "substitution"); }
        if (mode == "pre-pending-mode") check(::chmod((f.root + "/.pending-result").toLocal8Bit().constData(), 0644) == 0, "pending mode change");
    };
    if (mode.startsWith("pre-")) Access::hooks(e, change, {});
    if (mode.startsWith("post-")) Access::hooks(e, {}, change);
    const bool positive = mode != "negative";
    const bool published = Access::publish(e, payload, positive);
    const bool expected = mode == "valid" || mode == "negative" || mode == "cap4096";
    check(published == expected, "publication exact permission and pending identity");
    if (expected) {
        check(get(f.root + "/selected-result") == payload && !QFile::exists(f.root + "/.pending-result"), "closed result exact payload once");
        const auto s = meta(f.root + "/selected-result");
        check((s.st_mode & 07777) == 0600 && s.st_nlink == 1 && s.st_uid == ::geteuid(), "committed private singlelink");
        check(!Access::publish(e, "second", positive) && get(f.root + "/selected-result") == payload, "result cannot overwrite on retry");
    } else if (mode.startsWith("post-")) {
        const QString root = mode == "post-root" ? f.held : f.root;
        const auto result = meta(root + "/selected-result"), pending = meta(root + "/.pending-result");
        check(result.st_ino == pending.st_ino && result.st_nlink == 2 && pending.st_nlink == 2, "post-link uncertainty preserves two links");
    } else if (mode == "result-exists") check(get(f.root + "/selected-result") == "preserved", "existing result preserved");
    else if (mode == "pending-symlink") check(get(f.root + "/outside") == "preserved", "no-follow pending leaves target unchanged");
    else check(!QFile::exists(f.root + "/selected-result"), "refused publication leaves no result");
    QTextStream(stdout) << "PASS publish-" << mode << '\n';
}
void wireCase(bool requested,QGuiApplication &app) {
    Files f; f.config.coordinator.wantPostRejectProbe=requested;
    QQmlEngine engine;QQuickWindow window;
    QQmlEngine::setContextForObject(&window,engine.rootContext());window.show();window.requestActivate();pump();
    Entry e(&app,f.root,f.config,{});check(e.start(),"wire fixture starts");
    put(f.root+"/selected-request",f.request());Access::bind(e,&window);Access::finish(e);
    const auto bytes=get(f.root+"/selected-result");
    check(!bytes.isEmpty()&&bytes.size()<=4096,"actual bounded incomplete wire published");
    const auto body=QJsonDocument::fromJson(bytes).object();
    check(body.contains("post_reject_probe")==requested,"extension present exactly in requested mode");
    if(requested) {
        const auto p=body["post_reject_probe"].toObject();
        const QStringList keys{"acquired","attempted","native_authority","released","required","try_result","unlock_result"};
        check(p.keys()==keys&&p["required"].isBool()&&p["required"].toBool(),"closed seven-key required object");
        for(const char *name:{"attempted","acquired","released","native_authority"})check(p[name].isBool()&&!p[name].toBool(),"incomplete facts and authority remain false");
        check(p["try_result"].isDouble()&&p["try_result"].toInt()==-1&&p["unlock_result"].isDouble()&&p["unlock_result"].toInt()==-1,"unattempted results retain negative sentinel");
    }
    check(body["stage"]=="insertion-refusal-incomplete"&&!body["coordinator_observed"].toBool(),"wire-only fixture is not native success");
    QTextStream(stdout)<<"PASS wire-post-reject-"<<requested<<'\n';
}
void conditionalWireCase(int count,int source,QGuiApplication &app) {
    Files f;auto &c=f.config.coordinator;c.wantConditionalForward=true;c.wrongSourceIndex=-1;
    auto &q=c.request;q.expectedPages.clear();
    for(int i=1;i<=count;++i)q.expectedPages.append(QStringLiteral("10000000-0000-0000-0000-%1").arg(i,12,10,QLatin1Char('0')));
    q.sourceIndex=source;q.sourcePageId=q.expectedPages[source];f.config.navigation.pageIndex=source;
    const QJsonObject startup{{"profile_qualified",true},{"condition_qualified",true},{"application_absent_at_install",true},
        {"tasks_at_install",2},{"bind_now",true},{"native_authority",false}};
    QQmlEngine engine;QQuickWindow window;QQmlEngine::setContextForObject(&window,engine.rootContext());
    window.show();window.requestActivate();pump();
    Entry e(&app,f.root,f.config,{},startup);check(e.start(),"conditional file handoff starts without a native runtime");
    put(f.root+"/selected-request",f.request());Access::bind(e,&window);
    reader_insertion_entry::Result r;
    r.finished=r.observed=r.ownersRetained=r.registrationAccepted=r.forwardObserved=true;
    r.probeJob=25;r.job=26;r.matchingCompletions=r.pageAdded=1;r.pageAddedIndex=source+1;
    r.creation.attempted=r.creation.accepted=r.creation.callbackObserved=true;r.creation.callbackCount=1;
    r.core.terminal=r.core.witness=r.core.attributed=r.core.forwardCommitted=r.core.innerReturned=r.core.outerReturned=true;
    r.core.jobId=26;r.core.outerCalls=r.core.innerCalls=r.core.trackedForwards=1;
    auto &condition=r.core.condition;
    condition.matched=condition.sourceAtIndex=condition.targetMatches=condition.targetAbsent=condition.spanUnchanged=condition.treeUnchanged=true;
    condition.count=condition.treeCount=unsigned(count);condition.sourcePosition=source;
    condition.rightAdjacent=source+1<count;condition.endSource=!condition.rightAdjacent;
    condition.rightPosition=condition.rightAdjacent?source+1:-1;
    condition.wordReads=condition.treeReads=2;condition.searches=condition.rightAdjacent?2:1;condition.equalities=condition.finds=1;
    auto &post=r.postOrder;post.observed=true;post.pageCount=count+1;post.currentPageIndex=source;post.currentPageId=q.sourcePageId;
    post.ids=q.expectedPages;post.ids.insert(source+1,q.targetPageId);post.calls=unsigned(2*post.ids.size()+5);
    const auto body=Access::syntheticWire(e,r,true);
    const QByteArray bytes=QJsonDocument(body).toJson(QJsonDocument::Compact)+'\n';
    check(bytes.size()<=4096,"largest conditional wire retains the original file cap");
    check(body["schema"].toInt()==2&&body["diagnostic"]=="insertion-guard-conditional"&&body["stage"]=="insertion-forward-observed",
        "synthetic schema2 positive stage");
    check(!body.contains("post_reject_probe")&&body["expected_pages"].toArray().size()==count&&body["post_order"].toObject()["ids"].toArray().size()==count+1,
        "conditional full arrays and exclusive mode");
    check(body["core"].toObject()["condition"].toObject().size()==20&&body["post_order"].toObject().size()==11&&body["startup"].toObject().size()==6,
        "closed nested conditional shapes");
    for(const char *name:{"native_authority","render_authority","durable_authority"})check(body[name].isBool()&&!body[name].toBool(),"wire authority false");
    const QString stem=QStringLiteral("/owned-wire-n%1-source%2").arg(count).arg(source);
    const QString output=QString::fromLocal8Bit(qgetenv("INSERTION_TRANSPORT_OUTPUT"));check(!output.isEmpty(),"explicit owned output");
    put(output+stem+".json",bytes);
    post.targetSelected=true;post.currentPageId=q.targetPageId;post.currentPageIndex=source+1;
    put(output+stem+"-target.json",QJsonDocument(Access::syntheticWire(e,r,true)).toJson(QJsonDocument::Compact)+'\n');
    Access::finish(e);
    const auto incomplete=QJsonDocument::fromJson(get(f.root+"/selected-result")).object();
    check(incomplete["schema"].toInt()==2&&incomplete["stage"]=="insertion-forward-incomplete"&&!incomplete["forward_observed"].toBool(),
        "actual incomplete serializer never borrows synthetic success");
    put(output+stem+"-incomplete.json",get(f.root+"/selected-result"));
    QTextStream(stdout)<<"PASS synthetic-conditional-wire n="<<count<<" source="<<source<<" bytes="<<bytes.size()<<'\n';
}
}
int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    for (const QByteArray mode : {"valid", "root-mode", "root-symlink", "owner-mode", "owner-link", "owner-symlink", "wrong-nonce", "nonce-case",
            "identity", "ready-existing", "result-existing", "pending-existing", "cancel", "restore"}) startupCase(mode, app);
    for (const QByteArray mode : {"fifo", "directory", "symlink", "wrong-nonce", "wrong-pid", "partial", "oversize", "mode", "hardlink"}) requestCase(mode, app);
    for (const QByteArray mode : {"valid", "negative", "expired", "setup-expired", "failure", "window-death", "request-replaced", "root-replaced",
            "result-exists", "pending-exists", "pending-symlink", "pre-cancel", "pre-restore", "pre-expiry", "pre-root", "pre-request",
            "post-cancel", "post-restore", "post-expiry", "post-root", "post-request", "pre-pending-replace", "pre-pending-mode", "cap4096", "cap4097"}) publicationCase(mode, app);
    wireCase(false,app);wireCase(true,app);
    const int legacyChecks=checks;check(legacyChecks==795,"legacy50 scenarios keep their exact795 assertions");
    conditionalWireCase(1,0,app);conditionalWireCase(4,1,app);conditionalWireCase(4,3,app);
    conditionalWireCase(16,7,app);conditionalWireCase(16,15,app);
    QTextStream(stdout) << "PASS owned transport checks=" << checks << "; no Coordinator or native success\n"; return 0;
}
