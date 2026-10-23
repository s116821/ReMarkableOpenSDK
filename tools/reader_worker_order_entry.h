#pragma once
// One fixed read-only native worker experiment. No supported native capability.
#include "reader_selected_view_entry.h"
#include "reader_worker_order_accessors.h"
#include "reader_worker_owner_quarantine.h"
#include <QJsonArray>

namespace reader_worker_order_entry {
struct Config {
    QByteArray nonce;
    reader_selected::FixtureNavigation navigation;
    std::array<QString,3> pageIds;
    int pageCount=-1;
    bool valid() const {
        if(nonce.size()!=32||!navigation.valid()||!navigation.legacyComposition||pageCount<3||pageCount>32||
           navigation.pageIndex<1||navigation.pageIndex>=pageCount-1)return false;
        for(char c:nonce)if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')))return false;
        return bool(reader_worker_order::freeze(navigation.pageIndex,pageCount,pageIds).frozen);
    }
};
class Entry final : public QObject {
public:
    Entry(QGuiApplication *app,QString path,Config config,QObject *parent=nullptr)
        :QObject(parent?parent:app),app_(app),path_(std::move(path)),config_(std::move(config)),
         pool_(reader_worker_quarantine::ResidentStorage::create(),this) {clock_.start();}
    ~Entry() override {
        pool_.cancel(token_,QStringLiteral("entry-destroyed"));
        if(root_>=0)::close(root_);
        // Pool destruction quarantines an unresolved lease; it never closes it.
    }
    bool start() {
        if(started_)return false;
        started_=true;
        if(!app_||QThread::currentThread()!=app_->thread()||thread()!=app_->thread()||!config_.valid())return terminal_=true,false;
        identity_=reader_selected::selfIdentity();
        root_=::open(path_.toLocal8Bit().constData(),O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);
        if(identity_.isEmpty()||root_<0||::fstat(root_,&rootStat_)||!S_ISDIR(rootStat_.st_mode)||
           (rootStat_.st_mode&07777)!=0700||rootStat_.st_uid!=::geteuid()||!trusted())return terminal_=true,false;
        for(const char *name:{"selected-ready","selected-result",".pending-ready",".pending-result"})if(!absent(name))return terminal_=true,false;
        connect(&watcher_,&QFileSystemWatcher::directoryChanged,this,[this]{queue();});
        connect(&setup_,&QTimer::timeout,this,[this]{fail(QStringLiteral("setup-expired"));});
        connect(&operation_,&QTimer::timeout,this,[this]{fail(QStringLiteral("deadline"));});
        connect(&opportunity_,&QTimer::timeout,this,[this]{sample();});
        connect(&pool_,&reader_worker_quarantine::Pool::released,this,[this](quint64){queueOutcome();});
        connect(&pool_,&reader_worker_quarantine::Pool::settled,this,[this](quint64){queueOutcome();});
        connect(&pool_,&reader_worker_quarantine::Pool::quarantined,this,[this](quint64){queueOutcome();});
        connect(app_,&QCoreApplication::aboutToQuit,this,[this]{suppressed_=true;fail(QStringLiteral("application-closing"));});
        setup_.setSingleShot(true);operation_.setSingleShot(true);opportunity_.setSingleShot(true);opportunity_.setTimerType(Qt::PreciseTimer);
        if(!watcher_.addPath(path_))return terminal_=true,false;
        setup_.start(int(qMax<qint64>(1,reader_selected::SetupWindowMs-now())));
        if(!publish("selected-ready",".pending-ready",config_.nonce+' '+identity_,false)){fail(QStringLiteral("ready-publication-refused"));return false;}
        queue();return true;
    }
    bool terminal()const{return terminal_;}
#ifdef READER_WORKER_ORDER_ENTRY_TESTING
    using PrepareTest=std::function<reader_worker_quarantine::Lease(QObject*)>;
    void adaptersForTest(reader_worker_order::detail::IdAt first,reader_worker_order::detail::IndexFor second,
                         PrepareTest prepare,reader_worker_quarantine::Pool::Submit submit) {
        firstTest_=first;secondTest_=second;prepareTest_=std::move(prepare);submitTest_=std::move(submit);
    }
    void checkForTest(){queue();}
    void opportunityForTest(){opportunity_.stop();sample();}
    bool opportunityPendingForTest()const{return opportunity_.isActive();}
    bool samplingForTest()const{return sampling_;}
    void advanceForTest(qint64 amount){offset_+=amount;}
    void postLinkForTest(std::function<void()> hook){postLink_=std::move(hook);}
#endif
private:
    struct WorkState {
        std::unique_ptr<reader_worker_order::Accessors> accessors;
        std::shared_ptr<const reader_worker_order::Frozen> frozen;
        reader_worker_order::Result result;
        std::atomic<bool> ready{false};
        static void run(void *worker,void *context)noexcept {
            auto *state=static_cast<WorkState*>(context);
            state->result=state->accessors->read(static_cast<QObject*>(worker),*state->frozen);
            state->ready.store(true,std::memory_order_release);
        }
    };
    struct LeaseBox {
        std::unique_ptr<reader_worker_native::Prepared> prepared;
        static bool close(void *p){return static_cast<LeaseBox*>(p)->prepared->close();}
        static void destroy(void *p){delete static_cast<LeaseBox*>(p);}
    };
    qint64 now()const {
#ifdef READER_WORKER_ORDER_ENTRY_TESTING
        return clock_.elapsed()+offset_;
#else
        return clock_.elapsed();
#endif
    }
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
        struct stat s{};const bool identical=!::fstat(fresh,&s)&&s.st_dev==rootStat_.st_dev&&s.st_ino==rootStat_.st_ino&&s.st_uid==rootStat_.st_uid&&(s.st_mode&07777)==0700;
        const bool closed=::close(fresh)==0;
        if(!identical||!closed||!absent("selected-cancel")||!absent("restore.claim")||!read("owner",config_.nonce)||!read("attempt.identity",identity_))return false;
        if(consumed_){struct stat request{};return read("selected-request",config_.nonce+' '+identity_,&request)&&same(request,requestStat_);}return true;
    }
    bool permitted()const{return !terminal_&&failure_.isEmpty()&&trusted()&&now()<reader_selected::SetupWindowMs&&(end_<0||now()<end_);}
    bool sameWindow()const{return window_&&engine_&&QGuiApplication::focusWindow()==window_&&qmlEngine(window_)==engine_;}
    bool publish(const char *name,const char *pending,const QByteArray &bytes,bool positive)const {
        const auto admitted=[this,positive]{return trusted()&&now()<reader_selected::SetupWindowMs&&(!positive||(end_>=0&&now()<end_));};
        if(bytes.isEmpty()||bytes.size()>(QByteArray(name)=="selected-result"?4096:2048)||!admitted())return false;
        const int fd=::openat(root_,pending,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);if(fd<0)return false;
        struct stat s{};const bool valid=!::fstat(fd,&s)&&S_ISREG(s.st_mode)&&(s.st_mode&07777)==0600&&s.st_uid==rootStat_.st_uid&&s.st_nlink==1&&s.st_size==0;
        const ssize_t n=valid?::write(fd,bytes.constData(),size_t(bytes.size())):-1;const bool closed=::close(fd)==0;
        if(n!=bytes.size()||!closed||!admitted()||::linkat(root_,pending,root_,name,0))return false;
        const QPointer<const Entry> weak(this);
#ifdef READER_WORKER_ORDER_ENTRY_TESTING
        const auto hook=postLink_;
        if(hook&&QByteArray(name)=="selected-result")hook();
#endif
        if(!weak||!admitted()||::unlinkat(root_,pending,0))return false;
        return true;
    }
    void queue() {
        if(terminal_||queued_)return;
        queued_=true;
        const QPointer<Entry> weak(this);
        if(!QMetaObject::invokeMethod(this,[weak]{if(weak){weak->queued_=false;weak->check();}},Qt::QueuedConnection))fail(QStringLiteral("queue-refused"));
    }
    void check() {
        if(!permitted()){fail(QStringLiteral("request-disarmed"));return;}
        if(consumed_||sampling_)return;
        if(++checks_>32){fail(QStringLiteral("request-check-cap"));return;}
        if(absent("selected-request"))return;
        if(!read("selected-request",config_.nonce+' '+identity_,&requestStat_)){fail(QStringLiteral("request-refused"));return;}
        consumed_=true;begin_=now();end_=qMin(reader_selected::SetupWindowMs,begin_+30000);operation_.start(int(end_-now()));
        window_=qobject_cast<QQuickWindow*>(QGuiApplication::focusWindow());engine_=window_?qmlEngine(window_):nullptr;
        if(!permitted()||!sameWindow()){fail(QStringLiteral("window-refused"));return;}
        const QPointer<Entry> weak(this);sampling_=true;
        const auto result=reader_selected::openFixture(window_,config_.navigation,[weak]{return weak&&weak->permitted()&&weak->sameWindow();},int(qMin<qint64>(20000,end_-now())));
        if(!weak)return;
        sampling_=false;navigationAttempted_=result.attempted;navigationDispatched_=result.dispatched;
        if(!failure_.isEmpty()){finish();return;}
        if(!permitted()||!sameWindow()||!result.dispatched||!result.refusal.isEmpty()||end_-now()<=5000){fail(result.refusal.isEmpty()?QStringLiteral("bootstrap-refused"):result.refusal);return;}
        opportunity_.start(5000);
    }
    bool qualified(const reader_selected::Observation&o)const {
        return o.observed&&o.creationJoinObserved&&o.mapCoherent&&o.ready&&!o.loading&&!o.pageError&&!o.documentError&&!o.awaitingTiles&&!o.pendingTiles&&!o.working&&
            o.documentId==config_.navigation.documentId&&o.pageId==config_.pageIds[1]&&o.pageIndex==config_.navigation.pageIndex&&o.pageCount==config_.pageCount;
    }
    void sample() {
        // FIRST/LAST: a token is installed inside the Sample's synchronous action.
        std::unique_ptr<reader_worker_quarantine::Frame> nativeFrame;
        if(terminal_||sampled_)return;
        sampled_=true;
        if(!permitted()||!sameWindow()){fail(QStringLiteral("source-window-or-deadline"));return;}
        const QPointer<Entry> weak(this);sampling_=true;sampleBegin_=now();
        {
            reader_selected::ValidatedWorkerAction action=[weak,&nativeFrame](QObject *controller,const QVariant&id,QObject *worker,
                    const reader_selected::Observation&before,const std::function<bool()>&guard){
                if(weak)weak->submit(controller,id,worker,before,guard,nativeFrame);
            };
            const auto observation=reader_selected::observeCreationJoin(window_,[weak]{return weak&&weak->permitted()&&weak->sameWindow();},int(qMin<qint64>(20000,end_-now())),std::move(action));
            if(!weak)return;
            before_=observation;
        }
        sampling_=false;sampleEnd_=now();
        if(!failure_.isEmpty()){finish();return;}
        if(!qualified(before_)||!token_||!submission_.accepted){fail(before_.refusal.isEmpty()?QStringLiteral("worker-source-or-submit-refused"):before_.refusal);return;}
        queueOutcome();
    }
    void submit(QObject *controller,const QVariant&id,QObject *worker,const reader_selected::Observation&before,
                const std::function<bool()>&guard,std::unique_ptr<reader_worker_quarantine::Frame>&nativeFrame) {
        const QPointer<Entry> weak(this);
        const bool admitted=guard&&guard();
        if(!weak)return;
        if(!admitted||!qualified(before)||!worker){fail(QStringLiteral("worker-source-refused"));return;}
        QMetaMethod completion;int matches=0;const auto *meta=worker->metaObject();
        if(!weak)return;
        if(!meta||meta->methodCount()>512){fail(QStringLiteral("completion-contract"));return;}
        for(int i=0;i<meta->methodCount();++i)if(meta->method(i).name()=="jobCompleted"){completion=meta->method(i);++matches;}
        const bool completionAdmitted=guard();
        if(!weak)return;
        if(matches!=1||!completion.enclosingMetaObject()||qstrcmp(completion.enclosingMetaObject()->className(),"DocumentWorker")!=0||!completionAdmitted){
            fail(QStringLiteral("completion-contract"));return;}
        auto frozen=reader_worker_order::freeze(config_.navigation.pageIndex,config_.pageCount,config_.pageIds);
        if(!frozen.frozen){fail(frozen.refusal);return;}
        reader_worker_order::Loading loading;
#ifdef READER_WORKER_ORDER_ENTRY_TESTING
        if(firstTest_&&secondTest_)loading=reader_worker_order::testing::load(firstTest_,secondTest_);
        else
#endif
        loading=reader_worker_order::loadPinned(guard);
        if(!weak)return;
        if(!loading.accessors){fail(loading.refusal);return;}
        auto context=std::make_shared<WorkState>();context->accessors=std::move(loading.accessors);context->frozen=std::move(frozen.frozen);
        reader_worker_quarantine::Lease lease;
        std::unique_ptr<LeaseBox> box;
#ifdef READER_WORKER_ORDER_ENTRY_TESTING
        const auto prepareTest=prepareTest_;
        if(prepareTest)lease=prepareTest(worker);
        else
#endif
        {
            if(!loading.nativeBridge){fail(QStringLiteral("native-bridge-missing"));return;}
            auto preparing=loading.nativeBridge->prepare(controller,id,worker,guard);
            if(!weak)return;
            if(!preparing.prepared){submission_.unknown=preparing.unknown;fail(preparing.refusal);return;}
            box=std::make_unique<LeaseBox>();box->prepared=std::move(preparing.prepared);
            lease={box.get(),worker,&LeaseBox::close,&LeaseBox::destroy};
        }
        if(!weak)return;
        const bool installAdmitted=guard();
        if(!weak)return;
        if(!installAdmitted||!permitted()||!sameWindow()){
#ifdef READER_WORKER_ORDER_ENTRY_TESTING
            if(!box&&lease.object){const bool closed=lease.closeOnGui(lease.object);if(closed)lease.destroyClosedOnGui(lease.object);}
#endif
            fail(QStringLiteral("before-install-disarmed"));return;
        }
        auto *submittedBox=box.get();
        const auto installed=pool_.install(worker,completion,lease,{&WorkState::run,context},int(end_-now()));
        if(installed)box.release(); // Transfer before any post-install facade access.
        if(!weak)return;
        if(!installed){
#ifdef READER_WORKER_ORDER_ENTRY_TESTING
            if(!box&&lease.object){const bool closed=lease.closeOnGui(lease.object);if(closed)lease.destroyClosedOnGui(lease.object);}
#endif
            fail(QStringLiteral("quarantine-admission-refused"));return;
        }
        token_=installed;
        work_=std::move(context);nativeFrame=pool_.hold(token_);
        if(!nativeFrame||!nativeFrame->valid()){fail(QStringLiteral("quarantine-frame-refused"));return;}
        pool_.submit(token_,[weak,submittedBox,&guard](const std::function<void()>&callback)->qulonglong {
            if(!weak)return 0;
            const bool allowed=guard();
            if(!weak||!allowed)return 0;
#ifdef READER_WORKER_ORDER_ENTRY_TESTING
            if(weak->submitTest_){const auto submit=weak->submitTest_;weak->submission_.attempted=true;const auto job=submit(callback);if(weak){weak->submission_.accepted=job!=0;weak->submission_.jobId=job;}return job;}
#endif
            const auto submitted=submittedBox->prepared->submit(callback,guard);
            if(weak)weak->submission_=submitted;
            return submitted.accepted?submitted.jobId:0;
        });
    }
    void queueOutcome() {
        if(terminal_||outcomeQueued_)return;
        outcomeQueued_=true;const QPointer<Entry> weak(this);
        QMetaObject::invokeMethod(this,[weak]{if(weak){weak->outcomeQueued_=false;weak->outcome();}},Qt::QueuedConnection);
    }
    void outcome() {
        if(terminal_||sampling_)return;
        if(!permitted()||!sameWindow()){fail(QStringLiteral("worker-window-or-deadline"));return;}
        const auto snapshot=pool_.snapshot(token_);
        if(snapshot.phase==reader_worker_quarantine::Phase::Quarantined){fail(snapshot.refusal.isEmpty()?QStringLiteral("quarantined"):snapshot.refusal);return;}
        if(snapshot.phase!=reader_worker_quarantine::Phase::Released||!snapshot.settled||snapshot.depth)return;
        if(!work_||!work_->ready.load(std::memory_order_acquire)||!work_->result.observed){fail(QStringLiteral("worker-tuple-refused"));return;}
        finish();
    }
    void fail(QString reason) {
        if(terminal_)return;
        if(failure_.isEmpty())failure_=std::move(reason);
        const QPointer<Entry> weak(this);pool_.cancel(token_,failure_);if(!weak)return;
        if(!sampling_)finish();
    }
    static QJsonObject facts(const reader_selected::Observation&o) {
        return {{"observed",o.observed},{"creation_join_observed",o.creationJoinObserved},{"document_id",o.documentId},{"page_id",o.pageId},{"page_index",o.pageIndex},{"page_count",o.pageCount},
            {"map_coherent",o.mapCoherent},{"status",o.status},{"page_status",o.pageStatus},{"loading",o.loading},{"page_error",o.pageError},{"document_error",o.documentError},
            {"awaiting_tiles",o.awaitingTiles},{"pending_tiles",o.pendingTiles},{"working",o.working},{"ready",o.ready},{"refusal",o.refusal}};
    }
    void finish() {
        if(terminal_||sampling_)return;
        const auto snapshot=pool_.snapshot(token_);
        const bool tupleReady=work_&&work_->ready.load(std::memory_order_acquire);
        const auto tuple=tupleReady?work_->result:reader_worker_order::Result{};
        const bool success=failure_.isEmpty()&&permitted()&&sameWindow()&&qualified(before_)&&submission_.accepted&&
            snapshot.phase==reader_worker_quarantine::Phase::Released&&snapshot.settled&&!snapshot.depth&&snapshot.entered==1&&snapshot.returning==1&&snapshot.offGui==1&&
            !snapshot.duplicates&&!snapshot.skipped&&snapshot.completions==1&&snapshot.closed&&snapshot.destroyed&&tupleReady&&tuple.observed&&tuple.calls==6;
        terminal_=true;setup_.stop();operation_.stop();opportunity_.stop();
        if(suppressed_||!trusted())return;
        const auto identity=identity_.trimmed().split(' ');if(identity.size()!=2)return;
        QJsonArray entries;for(const auto&e:tuple.entries)entries.append(QJsonObject{{"index",e.index},{"inverse",e.inverse},{"page_id",QString::fromLatin1(e.id.data())}});
        QJsonObject worker{{"generation",qint64(snapshot.generation)},{"job_id",snapshot.jobId},{"phase",int(snapshot.phase)},{"attempted",snapshot.attempted},{"submission_returned",snapshot.submissionReturned},
            {"native_attempted",submission_.attempted},{"native_accepted",submission_.accepted},{"native_unknown",submission_.unknown},{"native_refusal",submission_.refusal},
            {"entered",int(snapshot.entered)},{"returning",int(snapshot.returning)},{"off_gui",int(snapshot.offGui)},{"duplicates",int(snapshot.duplicates)},{"skipped",int(snapshot.skipped)},
            {"completion_count",int(snapshot.completions)},{"owners_closed",snapshot.closed},{"owners_destroyed",snapshot.destroyed},{"settled",snapshot.settled},{"depth",int(snapshot.depth)},{"refusal",snapshot.refusal}};
        QJsonObject json{{"schema",1},{"diagnostic","worker-order-tuple"},{"nonce",QString::fromLatin1(config_.nonce)},{"pid",QString::fromLatin1(identity[0])},{"start",QString::fromLatin1(identity[1])},
            {"stage",success?"worker-order-observed":"worker-order-refused"},{"refusal",success?QString():failure_.isEmpty()?QStringLiteral("incomplete"):failure_},{"before_worker",facts(before_)},{"worker",worker},
            {"tuple_ready",tupleReady},{"tuple_observed",tuple.observed},{"tuple_calls",int(tuple.calls)},{"tuple_refusal",int(tuple.refusal)},{"entries",entries},
            {"navigation_attempted",navigationAttempted_},{"navigation_dispatched",navigationDispatched_},{"same_process",reader_selected::selfIdentity()==identity_},{"same_window_engine",sameWindow()},
            {"plan_begin_ms",begin_},{"source_begin_ms",sampleBegin_},{"source_end_ms",sampleEnd_},{"plan_end_ms",now()},{"native_authority",false},{"render_authority",false},{"durable_authority",false}};
        publish("selected-result",".pending-result",QJsonDocument(json).toJson(QJsonDocument::Compact)+'\n',success);
    }
    QPointer<QGuiApplication> app_;QString path_;Config config_;reader_worker_quarantine::Pool pool_;reader_worker_quarantine::Token token_;
    std::shared_ptr<WorkState>work_;reader_worker_native::Submission submission_;reader_selected::Observation before_;
    QPointer<QQuickWindow>window_;QPointer<QQmlEngine>engine_;QElapsedTimer clock_;QFileSystemWatcher watcher_;QTimer setup_,operation_,opportunity_;
    QByteArray identity_;struct stat rootStat_{},requestStat_{};int root_=-1,checks_=0;qint64 begin_=-1,end_=-1,sampleBegin_=-1,sampleEnd_=-1;
    bool started_=false,terminal_=false,consumed_=false,queued_=false,outcomeQueued_=false,sampling_=false,sampled_=false,suppressed_=false,navigationAttempted_=false,navigationDispatched_=false;QString failure_;
#ifdef READER_WORKER_ORDER_ENTRY_TESTING
    qint64 offset_=0;reader_worker_order::detail::IdAt firstTest_=nullptr;reader_worker_order::detail::IndexFor secondTest_=nullptr;
    PrepareTest prepareTest_;reader_worker_quarantine::Pool::Submit submitTest_;std::function<void()>postLink_;
#endif
};
} // namespace reader_worker_order_entry
