#pragma once
// Fixed research file handoff. Native/runtime admission is supplied separately.
#include "reader_insertion_guard_entry.h"
#include "reader_selected_view_entry.h"
#include <QJsonArray>
#include <chrono>

namespace reader_insertion_transport {
struct Config {
    QByteArray nonce;
    reader_selected::FixtureNavigation navigation;
    reader_insertion_entry::Config coordinator;
    bool valid() const {
        if(nonce.size()!=32 || !navigation.valid() || !navigation.legacyComposition || !coordinator.valid() ||
           navigation.documentId!=coordinator.request.documentId || navigation.pageIndex!=coordinator.request.sourceIndex) return false;
        for(char c:nonce) if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')))return false;
        return true;
    }
};
class Entry final:public QObject {
    using Clock=std::chrono::steady_clock;
public:
    Entry(QGuiApplication *app,QString path,Config config,reader_insertion_entry::Runtime runtime,QJsonObject startup={},QObject *parent=nullptr)
        :QObject(parent?parent:app),app_(app),path_(std::move(path)),config_(std::move(config)),startup_(std::move(startup)),
         coordinator_(new reader_insertion_entry::Coordinator(std::move(runtime),this)) {
        clock_.start();setupDeadline_=Clock::now()+std::chrono::milliseconds(reader_selected::SetupWindowMs);
        heartbeat_.setParent(this);heartbeat_.setInterval(25);
        opportunity_.setParent(this);opportunity_.setSingleShot(true);opportunity_.setTimerType(Qt::PreciseTimer);
        connect(&heartbeat_,&QTimer::timeout,this,[this]{tick();});
        connect(&opportunity_,&QTimer::timeout,this,[this]{beginCoordinator();});
        connect(coordinator_,&reader_insertion_entry::Coordinator::finished,this,[this]{finish();});
    }
    ~Entry() override {
        suppressed_=true;
        if(coordinator_)coordinator_->cancel();
        if(root_>=0)::close(root_);
    }
    bool start() {
        if(started_)return false;
        started_=true;
        if(!app_ || thread()!=app_->thread() || QThread::currentThread()!=app_->thread() || !config_.valid())return false;
        identity_=reader_selected::selfIdentity();
        root_=::open(path_.toLocal8Bit().constData(),O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);
        if(root_<0 || identity_.isEmpty() || ::fstat(root_,&rootStat_) || !S_ISDIR(rootStat_.st_mode) ||
           (rootStat_.st_mode&07777)!=0700 || rootStat_.st_uid!=::geteuid() || !trusted())return false;
        for(const char *name:{"selected-ready","selected-result",".pending-ready",".pending-result"})if(!absent(name))return false;
        connect(app_,&QCoreApplication::aboutToQuit,this,[this]{suppressed_=true;fail(QStringLiteral("application-closing"));});
        if(!publish("selected-ready",".pending-ready",config_.nonce+' '+identity_,false))return false;
        heartbeat_.start();tick();return true;
    }
    bool terminal() const {return terminal_;}
private:
#ifdef READER_INSERTION_GUARD_TRANSPORT_TESTING
    friend struct TestAccess;
    std::function<void()> preLink_,postLink_;
#endif
    static bool same(const struct stat&a,const struct stat&b) {
        return a.st_dev==b.st_dev&&a.st_ino==b.st_ino&&a.st_mode==b.st_mode&&a.st_uid==b.st_uid&&a.st_nlink==b.st_nlink&&a.st_size==b.st_size&&
            a.st_mtim.tv_sec==b.st_mtim.tv_sec&&a.st_mtim.tv_nsec==b.st_mtim.tv_nsec&&a.st_ctim.tv_sec==b.st_ctim.tv_sec&&a.st_ctim.tv_nsec==b.st_ctim.tv_nsec;
    }
    bool absent(const char *name)const{struct stat s{};if(!::fstatat(root_,name,&s,AT_SYMLINK_NOFOLLOW))return false;return errno==ENOENT;}
    bool read(const char *name,const QByteArray &expected,struct stat *identity=nullptr)const {
        if(expected.size()>75)return false;
        const int fd=::openat(root_,name,O_RDONLY|O_NOFOLLOW|O_CLOEXEC|O_NONBLOCK);if(fd<0)return false;
        struct stat a{},b{},named{};char bytes[76];
        bool ok=!::fstat(fd,&a)&&S_ISREG(a.st_mode)&&(a.st_mode&07777)==0600&&a.st_uid==rootStat_.st_uid&&a.st_nlink==1&&a.st_size==expected.size();
        const ssize_t n=ok?::read(fd,bytes,sizeof bytes):-1;
        ok=ok&&n==expected.size()&&QByteArray(bytes,n)==expected&&!::fstat(fd,&b)&&same(a,b)&&!::fstatat(root_,name,&named,AT_SYMLINK_NOFOLLOW)&&same(b,named);
        const bool closed=::close(fd)==0;if(ok&&closed&&identity)*identity=b;return ok&&closed;
    }
    bool trusted()const {
        if(root_<0||!app_||QCoreApplication::closingDown())return false;
        const int fresh=::open(path_.toLocal8Bit().constData(),O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);if(fresh<0)return false;
        struct stat s{};const bool identical=!::fstat(fresh,&s)&&s.st_dev==rootStat_.st_dev&&s.st_ino==rootStat_.st_ino&&
            s.st_uid==rootStat_.st_uid&&(s.st_mode&07777)==0700;
        const bool closed=::close(fresh)==0;
        if(!identical||!closed||!absent("selected-cancel")||!absent("restore.claim")||!read("owner",config_.nonce)||!read("attempt.identity",identity_))return false;
        if(consumed_){struct stat r{};return read("selected-request",config_.nonce+' '+identity_,&r)&&same(r,requestStat_);}return true;
    }
    bool permitted()const{return !terminal_&&failure_.isEmpty()&&trusted()&&Clock::now()<setupDeadline_&&(!consumed_||Clock::now()<deadline_);}
    bool sameWindow()const{return window_&&engine_&&QGuiApplication::focusWindow()==window_&&qmlEngine(window_)==engine_;}
    bool publish(const char *name,const char *pending,const QByteArray &bytes,bool positive)const {
        const auto allowed=[this,positive]{
            if(!trusted()||(positive&&(failure_.size()||!consumed_||!sameWindow()||reader_selected::selfIdentity()!=identity_)))return false;
            const auto now=Clock::now();
            return now<setupDeadline_&&(!positive||now<deadline_);
        };
        if(bytes.isEmpty()||bytes.size()>(QByteArray(name)=="selected-result"?4096:2048)||!allowed())return false;
        const int fd=::openat(root_,pending,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);if(fd<0)return false;
        struct stat s{};const bool valid=!::fstat(fd,&s)&&S_ISREG(s.st_mode)&&(s.st_mode&07777)==0600&&s.st_uid==rootStat_.st_uid&&s.st_nlink==1&&s.st_size==0;
        const ssize_t n=valid?::write(fd,bytes.constData(),size_t(bytes.size())):-1;
        struct stat written{};
        const bool stable=n==bytes.size()&&!::fstat(fd,&written)&&written.st_dev==s.st_dev&&written.st_ino==s.st_ino&&
            written.st_mode==s.st_mode&&written.st_uid==s.st_uid&&written.st_nlink==1&&written.st_size==bytes.size();
        const bool closed=::close(fd)==0;
        if(!stable||!closed||!allowed())return false;
#ifdef READER_INSERTION_GUARD_TRANSPORT_TESTING
        if(preLink_&&QByteArray(name)=="selected-result")preLink_();
#endif
        struct stat named{};
        if(!allowed()||::fstatat(root_,pending,&named,AT_SYMLINK_NOFOLLOW)||!same(written,named))return false;
        if(::linkat(root_,pending,root_,name,0))return false;
#ifdef READER_INSERTION_GUARD_TRANSPORT_TESTING
        if(postLink_&&QByteArray(name)=="selected-result")postLink_();
#endif
        // Preserve pending+nlink2 on lost permission. Readers must also bracket
        // their receipt check; this sampled commit is not a cancellation fence.
        const auto expected=[&written](const struct stat &value,nlink_t links){
            return value.st_dev==written.st_dev&&value.st_ino==written.st_ino&&value.st_mode==written.st_mode&&
                value.st_uid==written.st_uid&&value.st_size==written.st_size&&value.st_nlink==links&&
                value.st_mtim.tv_sec==written.st_mtim.tv_sec&&value.st_mtim.tv_nsec==written.st_mtim.tv_nsec;
        };
        struct stat linked{},result{};
        if(!allowed()||::fstatat(root_,pending,&linked,AT_SYMLINK_NOFOLLOW)||!expected(linked,2)||
           ::fstatat(root_,name,&result,AT_SYMLINK_NOFOLLOW)||!same(linked,result)||::unlinkat(root_,pending,0))return false;
        return !::fstatat(root_,name,&result,AT_SYMLINK_NOFOLLOW)&&expected(result,1)&&absent(pending);
    }
    void tick() {
        if(terminal_)return;
        if(++ticks_>4000||!permitted()){fail(QStringLiteral("permission-or-deadline"));return;}
        if(consumed_){if(!sameWindow())fail(QStringLiteral("window-engine-changed"));return;}
        if(absent("selected-request"))return;
        if(!read("selected-request",config_.nonce+' '+identity_,&requestStat_)){fail(QStringLiteral("request-contract"));return;}
        consumed_=true;begin_=clock_.elapsed();deadline_=qMin(setupDeadline_,Clock::now()+std::chrono::seconds(30));
        window_=qobject_cast<QQuickWindow*>(QGuiApplication::focusWindow());engine_=window_?qmlEngine(window_):nullptr;
        if(!sameWindow()){fail(QStringLiteral("window-engine"));return;}
        const QPointer<Entry> weak(this);busy_=true;
        const auto opened=reader_selected::openFixture(window_,config_.navigation,[weak]{return weak&&weak->permitted()&&weak->sameWindow();},remaining(20000));
        if(!weak)return;
        busy_=false;navigationAttempted_=opened.attempted;navigationDispatched_=opened.dispatched;
        if(!failure_.isEmpty()){finish();return;}
        if(!permitted()||!sameWindow()||!opened.dispatched||!opened.refusal.isEmpty()||remaining(30000)<=5000){fail(opened.refusal.isEmpty()?QStringLiteral("bootstrap-refused"):opened.refusal);return;}
        opportunity_.start(5000);
    }
    int remaining(int cap)const{return qMax(0,qMin(cap,int(std::chrono::duration_cast<std::chrono::milliseconds>(deadline_-Clock::now()).count())));}
    void beginCoordinator() {
        if(terminal_||coordinatorStarted_)return;
        if(!permitted()||!sameWindow()){fail(QStringLiteral("before-coordinator-disarmed"));return;}
        coordinatorStarted_=true;auto config=config_.coordinator;
        config.absoluteDeadline=deadline_;
        const QPointer<Entry> weak(this);busy_=true;
        coordinator_->start(window_,config,[weak]{return weak&&weak->permitted()&&weak->sameWindow();});
        if(!weak)return;
        busy_=false;if(coordinator_->snapshot().finished||!failure_.isEmpty())finish();
    }
    void fail(QString why) {
        if(terminal_)return;
        if(failure_.isEmpty())failure_=std::move(why);
        const QPointer<Entry> weak(this);coordinator_->cancel();if(weak&&!busy_)finish();
    }
    void finish() {
        if(terminal_||busy_)return;
        const auto r=coordinator_->snapshot();
        if(!r.finished&&failure_.isEmpty())return;
        const bool forward=config_.coordinator.wantConditionalForward;
        const bool success=failure_.isEmpty()&&permitted()&&sameWindow()&&r.observed&&!r.unknown&&!r.canceled&&
            (!forward||(r.forwardObserved&&!r.reconciliationRequired&&r.postOrder.observed))&&
            (!config_.coordinator.wantTargetAcquisition||(r.targetOrder.observed&&r.targetOrder.targetSelected&&
                r.targetNavigation.admitted&&!r.targetNavigation.unknown));
        terminal_=true;heartbeat_.stop();opportunity_.stop();
        if(suppressed_||!trusted())return;
        const auto body=wireBody(r,success);if(body.isEmpty())return;
        const auto bytes=QJsonDocument(body).toJson(QJsonDocument::Compact)+'\n';
        publish("selected-result",".pending-result",bytes,success);
    }
    QJsonObject wireBody(const reader_insertion_entry::Result &r,bool success)const {
        const bool forward=config_.coordinator.wantConditionalForward;
        const auto identity=identity_.trimmed().split(' ');if(identity.size()!=2)return {};
        QJsonObject core{{"terminal",r.core.terminal},{"witness",r.core.witness},{"private_caught",r.core.privateCaught},{"source_mismatch",r.core.observedMismatch},
            {"attributed",r.core.attributed},{"job_id",QString::number(r.core.jobId)},{"outer_calls",int(r.core.outerCalls)},{"inner_calls",int(r.core.innerCalls)},
            {"tracked_forwards",int(r.core.trackedForwards)},{"unknown",r.core.unknown},{"interference",r.core.interference},{"canceled",r.core.canceled},{"refusal",int(r.core.refusal)}};
        QJsonObject body{{"schema",1},{"diagnostic","insertion-guard-negative"},{"nonce",QString::fromLatin1(config_.nonce)},{"pid",QString::fromLatin1(identity[0])},{"start",QString::fromLatin1(identity[1])},
            {"stage",success?QStringLiteral("insertion-refusal-observed"):QStringLiteral("insertion-refusal-incomplete")},{"refusal",success?QString():failure_.isEmpty()?r.refusal:failure_},
            {"coordinator_finished",r.finished},{"coordinator_observed",r.observed},{"coordinator_unknown",r.unknown},{"coordinator_canceled",r.canceled},{"owners_retained",r.ownersRetained},
            {"registration_accepted",r.registrationAccepted},{"probe_job_id",QString::number(r.probeJob)},{"job_id",QString::number(r.job)},
            {"matching_completions",int(r.matchingCompletions)},{"page_added",int(r.pageAdded)},{"creation_attempted",r.creation.attempted},{"creation_accepted",r.creation.accepted},
            {"creation_callback",r.creation.callbackObserved},{"creation_callbacks",int(r.creation.callbackCount)},{"core",core},{"startup",startup_},
            {"navigation_attempted",navigationAttempted_},{"navigation_dispatched",navigationDispatched_},{"same_process",reader_selected::selfIdentity()==identity_},{"same_window_engine",sameWindow()},
            {"plan_begin_ms",begin_},{"plan_end_ms",clock_.elapsed()},{"native_authority",false},{"render_authority",false},{"durable_authority",false}};
        // terminal_ is set to prevent reentry; publication uses the original
        // explicit success and clocks rather than granting another request.
        if(config_.coordinator.wantPostRejectProbe) {
            const auto &p=r.core.postReject;
            body.insert("post_reject_probe",QJsonObject{{"required",true},{"attempted",p.attempted},{"acquired",p.acquired},
                {"released",p.released},{"try_result",p.tryResult},{"unlock_result",p.unlockResult},{"native_authority",false}});
        }
        if(forward) {
            const auto &c=r.core.condition;
            QJsonObject condition{{"matched",c.matched},{"refusal",int(c.refusal)},{"word_reads",int(c.wordReads)},
                {"tree_reads",int(c.treeReads)},{"searches",int(c.searches)},{"equalities",int(c.equalities)},
                {"finds",int(c.finds)},{"count",int(c.count)},{"tree_count",qint64(c.treeCount)},
                {"source_position",c.sourcePosition},{"right_position",c.rightPosition},{"source_at_index",c.sourceAtIndex},
                {"right_adjacent",c.rightAdjacent},{"end_source",c.endSource},{"target_matches",c.targetMatches},
                {"target_absent",c.targetAbsent},{"span_unchanged",c.spanUnchanged},{"tree_unchanged",c.treeUnchanged},
                {"native_authority",false},{"insertion_permission",false}};
            core.insert("condition",condition);
            core.insert("forward_committed",r.core.forwardCommitted);core.insert("inner_returned",r.core.innerReturned);
            core.insert("outer_returned",r.core.outerReturned);core.insert("reconciliation_required",r.core.reconciliationRequired);
            body.insert("core",core);body.insert("schema",2);body.insert("diagnostic","insertion-guard-conditional");
            body.insert("stage",success?QStringLiteral("insertion-forward-observed"):QStringLiteral("insertion-forward-incomplete"));
            const auto &request=config_.coordinator.request;
            QJsonArray expected;for(const auto &id:request.expectedPages)expected.append(id);
            body.insert("document_id",request.documentId);body.insert("expected_pages",expected);
            body.insert("source_index",request.sourceIndex);body.insert("target_id",request.targetPageId);
            body.insert("forward_observed",r.forwardObserved);body.insert("reconciliation_required",r.reconciliationRequired);
            body.insert("page_added_index",r.pageAddedIndex);
            const auto &p=r.postOrder;QJsonArray ids;for(const auto &id:p.ids)ids.append(id);
            body.insert("post_order",QJsonObject{{"observed",p.observed},{"target_selected",p.targetSelected},
                {"page_count",p.pageCount},{"current_page_index",p.currentPageIndex},{"current_page_id",p.currentPageId},
                {"ids",ids},{"calls",int(p.calls)},{"refusal",p.refusal},{"native_authority",false},
                {"render_authority",false},{"durable_authority",false}});
            if(config_.coordinator.wantTargetAcquisition) {
                const auto &m=r.targetMethod;const auto &n=r.targetNavigation;const auto &t=r.targetOrder;
                QJsonObject method{{"admitted",m.admitted},{"index",m.index},{"signature",m.signature},
                    {"class",m.declaringClass},{"return",m.returnName},{"type",m.returnTypeId},
                    {"access",m.access},{"kind",m.kind},{"params",m.parameterCount},{"matches",m.matches},{"refusal",m.refusal}};
                body.insert("schema",3);body.insert("diagnostic","insertion-guard-target");
                body.insert("stage",success?QStringLiteral("insertion-target-observed"):QStringLiteral("insertion-target-incomplete"));
                body.insert("target_acquisition",QJsonObject{{"method",method},{"admitted",n.admitted},
                    {"attempted",n.attempted},{"dispatched",n.dispatched},{"unknown",n.unknown},
                    {"already_selected",n.alreadySelected},{"refusal",n.refusal},
                    {"observed",t.observed},{"target_selected",t.targetSelected},{"count",t.pageCount},
                    {"index",t.currentPageIndex},{"id",t.currentPageId},{"calls",int(t.calls)},{"order_refusal",t.refusal}});
            }
        }
        return body;
    }
    QPointer<QGuiApplication> app_;QString path_;Config config_;QJsonObject startup_;reader_insertion_entry::Coordinator *coordinator_;
    QElapsedTimer clock_;QTimer heartbeat_,opportunity_;QPointer<QQuickWindow> window_;QPointer<QQmlEngine> engine_;
    int root_=-1;struct stat rootStat_{},requestStat_{};QByteArray identity_;Clock::time_point setupDeadline_,deadline_;
    qint64 begin_=-1;unsigned ticks_=0;QString failure_;
    bool started_=false,consumed_=false,terminal_=false,busy_=false,suppressed_=false,coordinatorStarted_=false,navigationAttempted_=false,navigationDispatched_=false;
};
}
