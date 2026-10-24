#pragma once
// One-use bootstrap admission. This does not itself verify the native profile.
#include "endpoint.h"
#include <QCryptographicHash>
#include <array>
#include <time.h>

namespace remarkable::resident {
inline quint64 bootMilliseconds() {
    timespec now{};
    if(::clock_gettime(CLOCK_BOOTTIME,&now)||now.tv_sec<0||now.tv_nsec<0||now.tv_nsec>=1000000000)return 0;
    const auto seconds=quint64(now.tv_sec);
    if(seconds>(std::numeric_limits<quint64>::max()-999)/1000)return 0;
    return seconds*1000+quint64(now.tv_nsec)/1000000;
}
struct ArtifactDigests {
    QByteArray profile,executable,adapter;
    bool valid()const {
        const auto hash=[](const QByteArray &value){return value.size()==32&&value!=QByteArray(32,0);};
        return hash(profile)&&hash(executable)&&hash(adapter);
    }
};
class Activation final {
    int root_=-1;
    struct stat rootStat_{};
    explicit Activation(QString directory):directory_(std::move(directory)) {}
    QString directory_;
    EndpointConfig config_;
    static bool same(const struct stat &a,const struct stat &b) {
        return a.st_dev==b.st_dev&&a.st_ino==b.st_ino&&a.st_uid==b.st_uid&&a.st_mode==b.st_mode;
    }
    static bool sameFile(const struct stat &a,const struct stat &b) {
        return same(a,b)&&a.st_size==b.st_size&&a.st_mtim.tv_sec==b.st_mtim.tv_sec&&
            a.st_mtim.tv_nsec==b.st_mtim.tv_nsec;
    }
public:
    Activation(const Activation&)=delete;
    Activation&operator=(const Activation&)=delete;
    ~Activation(){if(root_>=0)::close(root_);}
    const EndpointConfig &config()const{return config_;}
    bool directoryCurrent()const {
        struct stat current{},named{};
        const auto name=QFile::encodeName(directory_);
        return root_>=0&&!::fstat(root_,&current)&&same(rootStat_,current)&&
            !::lstat(name.constData(),&named)&&same(current,named);
    }
    // Digests must come from independent bootstrap checks, never this file or
    // a peer's echo. Only the direct supervisor parent may become the peer.
    // The directory owner is trusted, as for Endpoint; no same-UID isolation.
    static std::unique_ptr<Activation> claim(QString directory,const ArtifactDigests &expected) {
        if(!expected.valid())return {};
        const auto name=QFile::encodeName(directory);
        if(name.isEmpty()||!name.startsWith('/')||name.contains('\0')||name.endsWith('/')||
           name.size()+9>=qsizetype(sizeof(sockaddr_un::sun_path)))return {};
        auto result=std::unique_ptr<Activation>(new Activation(std::move(directory)));
        result->root_=::open(name.constData(),O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);
        if(result->root_<0||::fstat(result->root_,&result->rootStat_)||
           !S_ISDIR(result->rootStat_.st_mode)||(result->rootStat_.st_mode&07777)!=0700||
           result->rootStat_.st_uid!=::geteuid()||!result->directoryCurrent())return {};
        const int root=result->root_;
        struct stat absent{};
        if(!::fstatat(root,"sdk.sock",&absent,AT_SYMLINK_NOFOLLOW)||errno!=ENOENT)return {};
        const int file=::openat(root,"admission.bin",O_RDONLY|O_NONBLOCK|O_NOFOLLOW|O_CLOEXEC);
        if(file<0)return {};
        struct FileGuard {int fd;~FileGuard(){::close(fd);}} guard{file};
        struct stat before{},after{};
        if(::fstat(file,&before)||!S_ISREG(before.st_mode)||before.st_uid!=::geteuid()||
           (before.st_mode&07777)!=0600||before.st_nlink!=1||before.st_size!=168)return {};
        QByteArray bytes(168,0);qsizetype received=0;
        while(received<bytes.size()) {
            const auto count=::read(file,bytes.data()+received,std::size_t(bytes.size()-received));
            if(count<0&&errno==EINTR)continue;
            if(count<=0)return {};
            received+=count;
        }
        char extra=0;
        if(::read(file,&extra,1)!=0||::fstat(file,&after)||!sameFile(before,after)||
           after.st_nlink!=1||bytes.left(8)!="RSDKACT1"||bytes.mid(8,32)!=expected.profile||
           bytes.mid(40,32)!=expected.executable||bytes.mid(72,32)!=expected.adapter||
           bytes.mid(104,16)!=bootIdentity()||!validId(bytes.mid(120,16))||bytes.mid(164,4)!=QByteArray(4,0))return {};
        const auto u32=[&](int offset){return qFromBigEndian<quint32>(reinterpret_cast<const uchar*>(bytes.constData()+offset));};
        const auto u64=[&](int offset){return qFromBigEndian<quint64>(reinterpret_cast<const uchar*>(bytes.constData()+offset));};
        const auto parent=::getppid();
        const auto now=bootMilliseconds(),expires=u64(152),parentStart=processStart(parent),selfStart=processStart(::getpid());
        if(parent<=1||u32(136)!=quint32(::geteuid())||u32(140)!=quint32(parent)||
           !parentStart||u64(144)!=parentStart||!selfStart||!now||expires<=now||expires-now>60000||
           u32(160)<1||u32(160)>30000)return {};
        // O_EXCL-equivalent hard-link creation consumes this directory's one
        // admission. Keep both names as evidence; never reset/delete a claim.
        if(!result->directoryCurrent()||::linkat(root,"admission.bin",root,"admission.consumed",0))return {};
        struct stat consumed{},original{};
        const auto claimedAt=bootMilliseconds();
        if(::fstatat(root,"admission.consumed",&consumed,AT_SYMLINK_NOFOLLOW)||
           ::fstatat(root,"admission.bin",&original,AT_SYMLINK_NOFOLLOW)||
           ::fstat(file,&after)||!sameFile(before,consumed)||!sameFile(before,original)||
           !sameFile(before,after)||consumed.st_nlink!=2||after.st_nlink!=2||
           !result->directoryCurrent()||::getppid()!=parent||processStart(parent)!=parentStart||
           !claimedAt||claimedAt>=expires)return {};
        auto &config=result->config_;
        config.directory=result->directory_;
        config.peerUid=::geteuid();config.peerPid=parent;config.peerStart=parentStart;
        config.operationBudgetMs=int(u32(160));
        config.pinDirectory=true;config.directoryDevice=result->rootStat_.st_dev;config.directoryInode=result->rootStat_.st_ino;
        config.runtime=expected.profile+expected.executable+expected.adapter+bytes.mid(104,16);
        std::array<uchar,12> process{};
        qToBigEndian<quint32>(quint32(::getpid()),process.data());
        qToBigEndian<quint64>(selfStart,process.data()+4);
        config.runtime.append(reinterpret_cast<const char*>(process.data()),qsizetype(process.size()));
        config.runtime.append(bytes.mid(120,16));
        return result;
    }
};
}
