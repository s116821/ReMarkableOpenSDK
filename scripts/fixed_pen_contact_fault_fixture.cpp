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
[[maybe_unused]] void replace(const std::string &path,const QByteArray &bytes,mode_t mode) {
    const auto temporary=path+".new";QFile f(QString::fromStdString(temporary));require(f.open(QIODevice::WriteOnly|QIODevice::NewOnly)&&f.write(bytes)==bytes.size(),"owned replacement bytes");f.close();
    require(::chmod(temporary.c_str(),mode)==0&&::rename(temporary.c_str(),path.c_str())==0,"owned named inode replacement");
}
}
int main(int argc,char **argv) {
    if(argc!=2)return 2;
    ::alarm(10);
    struct sigaction ignore{};ignore.sa_handler=SIG_IGN;::sigemptyset(&ignore.sa_mask);if(::sigaction(SIGPIPE,&ignore,nullptr))return 2;
    try {
        Threads background;
        scenario([&]{SpawnIO io(argv[1]);require(bool(io.image),"separate pinned fault image available");
            const auto budget=reader_fixed_pen::reserve(now());const auto r=run(io,budget);
            require(!r.accepted&&!r.diagnostic,"killed child cannot be accepted as a normal complete writer");
            require(r.childStarted&&r.childGone&&r.childStatusKnown&&r.childSignal==SIGKILL&&r.childExit==-1,"actual spawned child reaped with SIGKILL9");
            require(!r.childResultKnown&&!r.dispatch.complete&&r.dispatch.unknown&&r.dispatch.contactPossible,"no invented complete child record after termination");
            require(r.childStart>0&&io.launches==1&&io.parentWriterClosed,"single actual exec with parent writer descriptor closed");
            require(r.armed&&io.releases==1&&r.recovery.attempted&&r.recovery.written&&r.recovery.observed&&!r.recovery.unknown,"independent descriptor release written and observed once");
            require(io.contactSeen&&r.stream.contactFrames==6&&r.stream.contactIntervals==1,"actual FIFO contact prefix before release");
            require(r.stream.records==37&&r.stream.frames==9&&r.stream.startEndpointObserved&&!r.stream.endEndpointObserved&&!r.stream.strokeObserved,"eight written frames and one parent release are a partial stroke");
            require(reader_fixed_pen_stream::released(r.snapshot)&&r.stream.finalReleased&&io.quietConfirmed()&&io.snapshotMatched(),"released final record state agrees with modeled snapshot");
            require(r.deviceFdsClosed&&r.ipcFdsClosed&&r.traceClosed&&io.deviceFdsClosed()&&io.traceClosed(),"actual owned descriptors closed before publication");
            require(r.finishedMs>=budget.began&&r.finishedMs<budget.recoveryEnd,"original recovery budget retained");
            const auto b=binding(budget);exportWire("contact-fault",normalWire(b,r,io.evidence()),io.stored,expectations("contact-fault",b,false));
        });
        std::printf("PASS contact fault cases=%u checks=%u actual_exec=true self_signal=9 complete_child_message=false independent_release=true device_snapshot_context_models=true native_io=false\n",cases,checks);return 0;
    }catch(const std::exception&e){std::fprintf(stderr,"FAIL contact fault %s cases=%u checks=%u\n",e.what(),cases,checks);return 1;}
}
