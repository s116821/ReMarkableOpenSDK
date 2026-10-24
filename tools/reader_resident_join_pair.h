#pragma once
// Fixed two-read-only-request research plan. Not a production IPC service.
#include "reader_selected_view_entry.h"
#include "reader_request_retirement.h"
#include <array>
#include <stdexcept>

namespace reader_resident {
struct Config {
    QByteArray activation, first, second;
    reader_selected::FixtureNavigation navigation;
    QString pageId;
    int pageCount=-1;
    bool valid() const {
        const auto nonce=[](const QByteArray &s) {
            if(s.size()!=32)return false;
            for(char c:s)if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')))return false;
            return true;
        };
        return nonce(activation)&&nonce(first)&&nonce(second)&&activation!=first&&activation!=second&&first!=second&&
            navigation.valid()&&navigation.legacyComposition&&pageCount>navigation.pageIndex&&pageCount<=32&&
            QRegularExpression(QStringLiteral("^[0-9a-f]{8}(-[0-9a-f]{4}){3}-[0-9a-f]{12}$")).match(pageId).hasMatch();
    }
};

class Pair final : public QObject {
public:
    Pair(QGuiApplication *app,QString path,Config config,QObject *parent=nullptr)
        :QObject(parent?parent:app),app_(app),path_(std::move(path)),config_(std::move(config)),fence_(this) {clock_.start();}
    ~Pair() override {if(requestFd_>=0)::close(requestFd_);if(root_>=0)::close(root_);}
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
        connect(&opportunity_,&QTimer::timeout,this,[this]{beginRequest(0);});
        connect(&fence_,&reader_retirement::Fence::retired,this,[this](quint64 generation,const QByteArray &nonce){retired(generation,nonce);});
        connect(&fence_,&reader_retirement::Fence::retirementFailed,this,[this](quint64,const QByteArray&){fail(QStringLiteral("retirement-failed"));});
        connect(app_,&QCoreApplication::aboutToQuit,this,[this]{suppressed_=true;fail(QStringLiteral("application-closing"));});
        setup_.setSingleShot(true);opportunity_.setSingleShot(true);opportunity_.setTimerType(Qt::PreciseTimer);
        if(!watcher_.addPath(path_))return terminal_=true,false;
        setup_.start(int(qMax<qint64>(1,reader_selected::SetupWindowMs-now())));
        const QPointer<Pair> weak(this);
        if(!publish("selected-ready",".pending-ready",config_.activation+' '+identity_)) {if(weak)fail(QStringLiteral("ready-publication-refused"));return false;}
        queue();return true;
    }
    bool terminal() const {return terminal_;}
#ifdef READER_RESIDENT_TESTING
    void checkForTest(){queue();}
    void opportunityForTest(){opportunity_.stop();beginRequest(0);}
    bool opportunityPendingForTest()const{return opportunity_.isActive();}
    void advanceForTest(qint64 amount){offset_+=amount;}
    void postLinkForTest(std::function<void()> hook){postLink_=std::move(hook);}
    void beforeRequestForTest(std::function<void(int)> hook){beforeRequest_=std::move(hook);}
#endif
private:
    qint64 now()const {
#ifdef READER_RESIDENT_TESTING
        return clock_.elapsed()+offset_;
#else
        return clock_.elapsed();
#endif
    }
    bool absent(const char *name)const {struct stat s{};if(!::fstatat(root_,name,&s,AT_SYMLINK_NOFOLLOW))return false;return errno==ENOENT;}
    bool read(const char *name,const QByteArray &expected,struct stat *identity=nullptr)const {
        if(expected.size()>75)return false;
        const int fd=::openat(root_,name,O_RDONLY|O_NOFOLLOW|O_CLOEXEC|O_NONBLOCK);if(fd<0)return false;
        struct stat a{},b{},named{};char bytes[76];
        bool ok=!::fstat(fd,&a)&&S_ISREG(a.st_mode)&&(a.st_mode&07777)==0600&&a.st_uid==rootStat_.st_uid&&a.st_nlink==1&&a.st_size==expected.size();
        const ssize_t n=ok?::read(fd,bytes,sizeof bytes):-1;
        ok=ok&&n==expected.size()&&QByteArray(bytes,n)==expected&&!::fstat(fd,&b)&&same(a,b)&&
            !::fstatat(root_,name,&named,AT_SYMLINK_NOFOLLOW)&&same(b,named);
        const bool closed=::close(fd)==0;
        if(ok&&closed&&identity)*identity=b;
        return ok&&closed;
    }
    static bool same(const struct stat&a,const struct stat&b) {
        return a.st_dev==b.st_dev&&a.st_ino==b.st_ino&&a.st_mode==b.st_mode&&a.st_uid==b.st_uid&&a.st_nlink==b.st_nlink&&a.st_size==b.st_size&&
            a.st_mtim.tv_sec==b.st_mtim.tv_sec&&a.st_mtim.tv_nsec==b.st_mtim.tv_nsec&&a.st_ctim.tv_sec==b.st_ctim.tv_sec&&a.st_ctim.tv_nsec==b.st_ctim.tv_nsec;
    }
    bool trusted()const {
        if(root_<0||!app_||QCoreApplication::closingDown())return false;
        const int fresh=::open(path_.toLocal8Bit().constData(),O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);if(fresh<0)return false;
        struct stat s{};const bool identical=!::fstat(fresh,&s)&&s.st_dev==rootStat_.st_dev&&s.st_ino==rootStat_.st_ino&&s.st_uid==rootStat_.st_uid&&(s.st_mode&07777)==0700;
        const bool closed=::close(fresh)==0;
        if(!identical||!closed||!absent("selected-cancel")||!absent("restore.claim")||!read("owner",config_.activation)||!read("attempt.identity",identity_))return false;
        if(consumed_){struct stat request{};return read("selected-request",config_.activation+' '+identity_,&request)&&same(request,requestStat_);}
        return true;
    }
    bool permitted()const {return !terminal_&&pendingFailure_.isEmpty()&&trusted()&&now()<reader_selected::SetupWindowMs&&(operationEnd_<0||now()<operationEnd_);}
    bool sameWindow()const {return window_&&engine_&&QGuiApplication::focusWindow()==window_&&qmlEngine(window_)==engine_;}
    bool publish(const char *name,const char *pending,const QByteArray &bytes,bool requireOperation=false)const {
        const auto admitted=[this,requireOperation]{return trusted()&&now()<reader_selected::SetupWindowMs&&(!requireOperation||(operationEnd_>=0&&now()<operationEnd_));};
        if(bytes.isEmpty()||bytes.size()>(QByteArray(name)=="selected-result"?4096:2048)||!admitted())return false;
        const int fd=::openat(root_,pending,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);if(fd<0)return false;
        struct stat s{};const bool valid=!::fstat(fd,&s)&&S_ISREG(s.st_mode)&&(s.st_mode&07777)==0600&&s.st_uid==rootStat_.st_uid&&s.st_nlink==1&&s.st_size==0;
        const ssize_t n=valid?::write(fd,bytes.constData(),size_t(bytes.size())):-1;
        const bool closed=::close(fd)==0;
        if(n!=bytes.size()||!closed||!admitted()||::linkat(root_,pending,root_,name,0))return false;
        const QPointer<const Pair> weak(this);
#ifdef READER_RESIDENT_TESTING
        if(postLink_&&QByteArray(name)=="selected-result")postLink_();
#endif
        if(!weak||!admitted()||::unlinkat(root_,pending,0))return false;
        return true;
    }
    void queue() {
        if(terminal_||queued_)return;
        queued_=true;
        const QPointer<Pair> weak(this);
        if(!QMetaObject::invokeMethod(this,[weak]{if(weak){weak->queued_=false;weak->check();}},Qt::QueuedConnection))fail(QStringLiteral("queue-refused"));
    }
    void check() {
        if(!permitted()){fail(QStringLiteral("request-disarmed"));return;}
        if(consumed_||sampling_)return;
        if(++checks_>32){fail(QStringLiteral("request-check-cap"));return;}
        if(absent("selected-request"))return;
        if(!read("selected-request",config_.activation+' '+identity_,&requestStat_)){fail(QStringLiteral("request-refused"));return;}
        consumed_=true;begin_=now();operationEnd_=qMin(reader_selected::SetupWindowMs,begin_+30000);
        window_=qobject_cast<QQuickWindow*>(QGuiApplication::focusWindow());engine_=window_?qmlEngine(window_):nullptr;
        if(!permitted()||!sameWindow()){fail(QStringLiteral("window-refused"));return;}
        const QPointer<Pair> weak(this);sampling_=true;
        const auto result=reader_selected::openFixture(window_,config_.navigation,[weak]{return weak&&weak->permitted()&&weak->sameWindow();},int(qMin<qint64>(20000,operationEnd_-now())));
        if(!weak)return;
        sampling_=false;navigationAttempted_=result.attempted;navigationDispatched_=result.dispatched;
        if(!pendingFailure_.isEmpty()){finish(pendingFailure_);return;}
        if(!permitted()||!sameWindow()||!result.dispatched||!result.refusal.isEmpty()||operationEnd_-now()<=5000){fail(result.refusal.isEmpty()?QStringLiteral("bootstrap-refused"):result.refusal);return;}
        opportunity_.start(5000);
    }
    void beginRequest(int index) {
        if(index<0||index>1||terminal_||records_[index].attempted)return;
        if(!permitted()||!sameWindow()){fail(QStringLiteral("request-window-or-deadline"));return;}
#ifdef READER_RESIDENT_TESTING
        const QPointer<Pair> weak(this);if(beforeRequest_)beforeRequest_(index);if(!weak)return;
#endif
        if(!permitted()||!sameWindow()){fail(QStringLiteral("request-window-or-deadline"));return;}
        if(index==1){
            const auto previous=fence_.snapshot(token_);
            if(!records_[0].retired||!records_[0].cleanup||previous.phase!=reader_retirement::Phase::Retired||previous.depth!=0){fail(QStringLiteral("previous-request-not-retired"));return;}
            firstRetiredBeforeSecond_=true;
        }
        token_=fence_.begin(index==0?config_.first:config_.second,reader_retirement::Kind::ReadOnly);
        if(!token_){fail(QStringLiteral("request-admission-refused"));return;}
        active_=index;records_[index].attempted=true;records_[index].generation=token_.generation();
        requestFd_=::fcntl(root_,F_DUPFD_CLOEXEC,0);
        if(requestFd_<0){fail(QStringLiteral("request-descriptor-refused"));return;}
        sampleRequest();
    }
    void sampleRequest() {
        auto frame=fence_.enter(token_); // FIRST; all reentrant local destruction precedes its exit.
        if(!frame||!fence_.accepts(token_)||!permitted()||!sameWindow()){fail(QStringLiteral("request-disarmed"));return;}
        const QPointer<Pair> weak(this);const auto token=token_;const int index=active_;
        sampling_=true;records_[index].begin=now();
        const auto observation=reader_selected::observeCreationJoin(window_,[weak,token]{return weak&&weak->fence_.accepts(token)&&weak->permitted()&&weak->sameWindow();},int(qMin<qint64>(20000,operationEnd_-now())));
        if(!weak)return;
        sampling_=false;records_[index].end=now();records_[index].observation=observation;
        if(!pendingFailure_.isEmpty()){finish(pendingFailure_);return;}
        if(!permitted()||!sameWindow()||!qualified(observation)){fail(observation.refusal.isEmpty()?QStringLiteral("joined-observation-refused"):observation.refusal);return;}
        if(!fence_.requestRetire(token,[weak,index]{
            if(!weak)return;
            const int fd=weak->requestFd_;weak->requestFd_=-1;
            weak->records_[index].cleanup=fd>=0&&::close(fd)==0;
            if(!weak->records_[index].cleanup)throw std::runtime_error("request descriptor cleanup failed");
        },reader_retirement::Effect::NoMutationAttempted))fail(QStringLiteral("retirement-refused"));
    }
    bool qualified(const reader_selected::Observation&o)const {
        return o.observed&&o.creationJoinObserved&&o.mapCoherent&&o.ready&&!o.loading&&!o.pageError&&!o.documentError&&!o.awaitingTiles&&!o.pendingTiles&&!o.working&&
            o.documentId==config_.navigation.documentId&&o.pageId==config_.pageId&&o.pageIndex==config_.navigation.pageIndex&&o.pageCount==config_.pageCount;
    }
    void retired(quint64 generation,const QByteArray &nonce) {
        if(active_<0||active_>1||generation!=records_[active_].generation||nonce!=(active_==0?config_.first:config_.second))return;
        records_[active_].retired=true;
        if(terminal_)return;
        const QPointer<Pair> weak(this);const int next=active_+1;
        if(!QMetaObject::invokeMethod(this,[weak,next]{if(!weak||weak->terminal_)return;if(next==1)weak->beginRequest(1);else weak->finish(QStringLiteral("resident-pair-observed"));},Qt::QueuedConnection))fail(QStringLiteral("queue-refused"));
    }
    void fail(QString reason) {
        if(terminal_)return;
        if(pendingFailure_.isEmpty())pendingFailure_=std::move(reason);
        setup_.stop();opportunity_.stop();
        const QPointer<Pair> weak(this);
        if(token_&&fence_.accepts(token_))fence_.requestRetire(token_,[weak]{if(weak&&weak->requestFd_>=0){const int fd=weak->requestFd_;weak->requestFd_=-1;::close(fd);}},reader_retirement::Effect::NoMutationAttempted);
        if(!sampling_)finish(pendingFailure_);
    }
    static QJsonObject facts(const reader_selected::Observation&o) {
        return {{"observed",o.observed},{"creation_join_observed",o.creationJoinObserved},{"document_id",o.documentId},{"page_id",o.pageId},{"page_index",o.pageIndex},{"page_count",o.pageCount},
            {"map_coherent",o.mapCoherent},{"status",o.status},{"page_status",o.pageStatus},{"loading",o.loading},{"page_error",o.pageError},{"document_error",o.documentError},
            {"awaiting_tiles",o.awaitingTiles},{"pending_tiles",o.pendingTiles},{"working",o.working},{"ready",o.ready},{"refusal",o.refusal}};
    }
    void finish(const QString &stage) {
        if(terminal_)return;
        terminal_=true;setup_.stop();opportunity_.stop();const auto paths=watcher_.directories();if(!paths.isEmpty())watcher_.removePaths(paths);
        if(suppressed_||!trusted())return;
        const auto identity=identity_.trimmed().split(' ');if(identity.size()!=2)return;
        QJsonArray requests;
        for(int i=0;i<2;++i){auto item=facts(records_[i].observation);item.insert("nonce",QString::fromLatin1(i==0?config_.first:config_.second));item.insert("generation",qint64(records_[i].generation));
            item.insert("attempted",records_[i].attempted);item.insert("retired",records_[i].retired);item.insert("cleanup_ok",records_[i].cleanup);item.insert("sample_begin_ms",records_[i].begin);item.insert("sample_end_ms",records_[i].end);requests.append(item);}
        const bool success=stage==QStringLiteral("resident-pair-observed")&&sameWindow()&&firstRetiredBeforeSecond_&&records_[0].retired&&records_[1].retired&&records_[0].cleanup&&records_[1].cleanup&&qualified(records_[0].observation)&&qualified(records_[1].observation)&&now()<operationEnd_;
        QJsonObject json{{"schema",1},{"diagnostic","resident-readonly-pair"},{"nonce",QString::fromLatin1(config_.activation)},{"pid",QString::fromLatin1(identity[0])},{"start",QString::fromLatin1(identity[1])},
            {"stage",success?stage:QStringLiteral("resident-pair-refused")},{"refusal",success?QString():stage},{"requests",requests},{"same_process",reader_selected::selfIdentity()==identity_},{"same_window_engine",sameWindow()},
            {"navigation_attempted",navigationAttempted_},{"navigation_dispatched",navigationDispatched_},{"first_retired_before_second",firstRetiredBeforeSecond_},{"sample_begin_ms",begin_},{"sample_end_ms",now()},
            {"native_authority",false},{"render_authority",false},{"durable_authority",false}};
        publish("selected-result",".pending-result",QJsonDocument(json).toJson(QJsonDocument::Compact)+'\n',success);
    }
    struct Record{reader_selected::Observation observation;quint64 generation=0;qint64 begin=-1,end=-1;bool attempted=false,retired=false,cleanup=false;};
    QPointer<QGuiApplication> app_;QString path_;Config config_;reader_retirement::Fence fence_;reader_retirement::Token token_;
    QPointer<QQuickWindow> window_;QPointer<QQmlEngine> engine_;std::array<Record,2> records_{};
    QElapsedTimer clock_;QFileSystemWatcher watcher_;QTimer setup_,opportunity_;QByteArray identity_;struct stat rootStat_{},requestStat_{};
    int root_=-1,requestFd_=-1,checks_=0,active_=-1;qint64 begin_=-1,operationEnd_=-1;
    bool started_=false,terminal_=false,consumed_=false,queued_=false,sampling_=false,suppressed_=false,navigationAttempted_=false,navigationDispatched_=false,firstRetiredBeforeSecond_=false;QString pendingFailure_;
#ifdef READER_RESIDENT_TESTING
    qint64 offset_=0;std::function<void()> postLink_;std::function<void(int)> beforeRequest_;
#endif
};
} // namespace reader_resident
