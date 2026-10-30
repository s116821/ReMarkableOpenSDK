// Actual posix_spawn/exec, fixed descriptor roles, FIFO/pipe/wait/recovery.
// Reuses original owned IO; its device/snapshot/context facts stay explicit models.
#define main reader_fixed_pen_legacy_fixture_main
#include "reader_fixed_pen_holder_fixture.cpp"
#undef main
#include "reader_fixed_pen_spawn.h"
#include <thread>
#include <atomic>

namespace {
struct Threads {
    std::atomic<bool> live{true};std::thread a,b;
    Threads():a([this]{while(live.load()){pollfd p{};::poll(&p,0,1);}}),b([this]{while(live.load()){pollfd p{};::poll(&p,0,1);}}){}
    ~Threads(){live.store(false);a.join();b.join();}
};
struct SpawnIO:IO {
    std::unique_ptr<reader_fixed_pen_spawn::Image> image;
    bool mismatch=false;unsigned launches=0;
    SpawnIO(const char *program,Fault fault=Fault::None):IO(fault) {
        const QString target=QString::fromStdString(folder)+"/fixed-pen-writer";
        require(QFile::copy(QString::fromLocal8Bit(program),target),"owned worker copied");
        require(::chmod(target.toLocal8Bit().constData(),0700)==0,"owned worker mode");
        QFile file(target);require(file.open(QIODevice::ReadOnly),"owned worker hash read");const auto data=file.readAll();file.close();
        QFile hash(QString::fromStdString(folder)+"/worker.sha256");require(hash.open(QIODevice::WriteOnly|QIODevice::NewOnly),"owned digest created");
        const auto digest=QCryptographicHash::hash(data,QCryptographicHash::Sha256).toHex()+'\n';require(hash.write(digest)==65,"owned digest exact65");hash.close();
        require(::chmod(hash.fileName().toLocal8Bit().constData(),0600)==0,"owned digest mode");
        openImage();
    }
    ~SpawnIO(){image.reset();for(const char *name:{"fixed-pen-writer","worker.sha256","replacement","writer.old","hash.old"})::unlink((folder+'/'+name).c_str());}
    void openImage(){
        const int root=::open(folder.c_str(),O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);require(root>=0,"owned root descriptor");
        image=reader_fixed_pen_spawn::Image::open(root,detail::processStart(::getpid()));const bool closed=::close(root)==0;require(closed,"caller root descriptor closed");
    }
    pid_t spawnChild(int start,int cancel,int report,pid_t parent,const reader_fixed_pen::Budget &budget)noexcept {
        ++launches;if(!image)return -1;struct stat st{};if(::fstat(writer,&st))return -1;
        if(mismatch)++st.st_ino;
        return image->spawn(writer,start,cancel,report,parent,budget,st);
    }
    bool afterForkParent()noexcept {parentWriterClosed=closeFd(writer);return parentWriterClosed&&image&&image->postSpawnOkay();}
};
void replace(const std::string &path,const QByteArray &bytes,mode_t mode) {
    const auto temporary=path+".new";QFile f(QString::fromStdString(temporary));require(f.open(QIODevice::WriteOnly|QIODevice::NewOnly)&&f.write(bytes)==bytes.size(),"owned replacement bytes");f.close();
    require(::chmod(temporary.c_str(),mode)==0&&::rename(temporary.c_str(),path.c_str())==0,"owned named inode replacement");
}
}
int main(int argc,char **argv) {
    if(argc!=2)return 2;
    sigset_t alarmMask;struct sigaction alarmDefault{};alarmDefault.sa_handler=SIG_DFL;
    if(::sigemptyset(&alarmMask)||::sigaddset(&alarmMask,SIGALRM)||::sigprocmask(SIG_UNBLOCK,&alarmMask,nullptr)||
       ::sigemptyset(&alarmDefault.sa_mask)||::sigaction(SIGALRM,&alarmDefault,nullptr))return 2;
    ::alarm(10); // independent bound, including native owned-file qualification
    struct sigaction ignore{};ignore.sa_handler=SIG_IGN;::sigemptyset(&ignore.sa_mask);if(::sigaction(SIGPIPE,&ignore,nullptr))return 2;
    try{
        Threads background;
        scenario([&]{SpawnIO io(argv[1]);require(bool(io.image),"verified image available");
            const int extra=::open("/dev/null",O_RDONLY);require(extra>=0,"extra parent descriptor");const int held=::fcntl(extra,F_DUPFD,200);require(held>=200&&::close(extra)==0,"noncloexec parent descriptor at200");
            require(::setenv("LD_PRELOAD","OWNED_NOT_A_LIBRARY",1)==0,"owned parent inherited injection marker");
            const auto budget=reader_fixed_pen::reserve(now());const auto r=run(io,budget);::unsetenv("LD_PRELOAD");
            require(r.accepted&&r.childGone&&r.childExit==0&&r.childResultKnown&&r.dispatch.calls==56&&r.dispatch.acceptedRecords==178,"fresh child complete fixed stroke accepted");
            require(io.launches==1&&io.releases==1&&r.recovery.observed&&r.stream.strokeObserved,"single spawn/fenced child/one recovery");
            require(::fcntl(held,F_GETFD)>=0&&::close(held)==0,"parent unknown descriptor retained while child closed extras");
            struct stat st{};require(io.image->spawn(-1,-1,-1,-1,::getpid(),budget,st)<0,"used image never spawns again");
            const auto b=binding(budget);exportWire("spawn-positive",normalWire(b,r,io.evidence()),io.stored,expectations("spawn-positive",b,true));
        });
        scenario([&]{SpawnIO io(argv[1],Fault::CancelContact);const auto r=run(io,reader_fixed_pen::reserve(now()));
            require(!r.accepted&&r.childGone&&io.launches==1&&io.releases==1&&r.recovery.attempted,"canceled spawned child fenced before one release attempt");});
        scenario([&]{SpawnIO io(argv[1]);io.mismatch=true;const auto r=run(io,reader_fixed_pen::reserve(now()));
            require(!r.accepted&&r.childGone&&r.childExit==2&&io.releases==1,"worker mismatched descriptor refuses without fake positive");});
        scenario([&]{SpawnIO io(argv[1]);const auto r=run(io,reader_fixed_pen::reserve(now()-4000));require(!r.accepted&&!r.childStarted&&!r.armed&&io.launches==0,"expired original budget never spawns");});
        scenario([&]{SpawnIO io(argv[1]);replace(io.folder+"/fixed-pen-writer","not an executable\n",0700);const auto r=run(io,reader_fixed_pen::reserve(now()));
            require(!r.accepted&&r.childPid==0&&io.launches==1&&!r.childStarted&&!r.dispatch.attempted,"image replacement refuses before GO");});
        scenario([&]{SpawnIO io(argv[1]);replace(io.folder+"/worker.sha256",QByteArray(64,'0')+'\n',0600);const auto r=run(io,reader_fixed_pen::reserve(now()));
            require(!r.accepted&&!r.childStarted&&r.childPid==0,"digest inode replacement refuses before GO");});
        scenario([&]{SpawnIO io(argv[1]);io.image.reset();replace(io.folder+"/fixed-pen-writer","not an executable\n",0700);replace(io.folder+"/worker.sha256",QCryptographicHash::hash("not an executable\n",QCryptographicHash::Sha256).toHex()+'\n',0600);io.openImage();
            require(bool(io.image),"source-authenticated malformed executable model pinned");const auto r=run(io,reader_fixed_pen::reserve(now()));
            if(r.accepted||r.childStarted||r.childPid!=0||io.launches!=1)std::fprintf(stderr,"exec-format details accepted=%d started=%d child=%ld gone=%d exit=%d signal=%d error=%d attempted=%d refusal=%s\n",int(r.accepted),int(r.childStarted),static_cast<long>(r.childPid),int(r.childGone),r.childExit,r.childSignal,io.image->error(),int(r.dispatch.attempted),r.refusal);
            require(!r.accepted&&!r.childStarted&&r.childPid==0&&io.launches==1,"exec format failure never falls back to fork");});
        for(unsigned variant=0;variant<4;++variant)scenario([&]{SpawnIO io(argv[1]);io.image.reset();const auto worker=io.folder+"/fixed-pen-writer",digest=io.folder+"/worker.sha256";
            if(variant==0)require(::chmod(worker.c_str(),0600)==0,"wrong worker mode model");
            if(variant==1)require(::link(worker.c_str(),(io.folder+"/writer.old").c_str())==0,"linked worker model");
            if(variant==2)replace(digest,QByteArray(64,'0')+'\n',0600);
            if(variant==3)replace(digest,QByteArray(65,'z'),0600);
            io.openImage();require(!io.image,"malformed mode/link/hash rejected before ready");
        });
        scenario([&]{SpawnIO io(argv[1]);struct stat st{};require(!::fstat(io.writer,&st),"owned writer stat");const auto b=reader_fixed_pen::reserve(now());
            require(io.image->spawn(io.writer,-1,-1,-1,::getpid(),b,st)<0,"invalid source descriptor refuses");
            require(io.image->spawn(io.writer,0,0,1,::getpid(),b,st)<0,"failed launch still consumes one-use image");});
        static_assert(detail::HasSpawn<SpawnIO>::value&&!detail::HasSpawn<IO>::value,"native-style and historical owned routes distinct");
        std::printf("PASS fixed pen spawn cases=%u checks=%u actual_exec=true parent_background_threads=2 device_snapshot_context_models=true native_io=false\n",cases,checks);return 0;
    }catch(const std::exception &e){std::fprintf(stderr,"FAIL fixed pen spawn %s cases=%u checks=%u\n",e.what(),cases,checks);return 1;}
}
