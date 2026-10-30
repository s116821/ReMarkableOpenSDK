#pragma once
// Parent-only fresh-image launcher. No device opening or input in this helper.
#include "reader_fixed_pen_holder.h"
#include <QByteArray>
#include <QCryptographicHash>
#include <array>
#include <memory>
#include <spawn.h>
#include <sys/stat.h>

namespace reader_fixed_pen_spawn {
class Image final {
    struct Fd {
        int value=-1;
        ~Fd(){if(value>=0)::close(value);}
        bool close(){const int n=std::exchange(value,-1);return n<0||::close(n)==0;}
    };
    static bool same(const struct stat&a,const struct stat&b)noexcept {
        return a.st_dev==b.st_dev&&a.st_ino==b.st_ino&&a.st_mode==b.st_mode&&a.st_uid==b.st_uid&&a.st_gid==b.st_gid&&
            a.st_nlink==b.st_nlink&&a.st_size==b.st_size&&a.st_mtim.tv_sec==b.st_mtim.tv_sec&&a.st_mtim.tv_nsec==b.st_mtim.tv_nsec&&
            a.st_ctim.tv_sec==b.st_ctim.tv_sec&&a.st_ctim.tv_nsec==b.st_ctim.tv_nsec;
    }
    static bool regular(const struct stat&s,mode_t mode,off_t limit)noexcept {
        return S_ISREG(s.st_mode)&&(s.st_mode&07777)==mode&&s.st_uid==::geteuid()&&s.st_nlink==1&&s.st_size>0&&s.st_size<=limit;
    }
public:
    static std::unique_ptr<Image> open(int directory,std::uint64_t parentStart) {
        auto self=std::unique_ptr<Image>(new Image);
        self->parent_=::getpid();self->parentStart_=parentStart;
        if(!parentStart||reader_fixed_pen_holder::detail::processStart(self->parent_)!=parentStart)return {};
        self->root_.value=::fcntl(directory,F_DUPFD_CLOEXEC,16);
        if(self->root_.value<0||::fstat(self->root_.value,&self->rootStat_)||!S_ISDIR(self->rootStat_.st_mode)||
           (self->rootStat_.st_mode&07777)!=0700||self->rootStat_.st_uid!=::geteuid())return {};
        self->hash_.value=::openat(self->root_.value,"worker.sha256",O_RDONLY|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC);
        self->image_.value=::openat(self->root_.value,"fixed-pen-writer",O_RDONLY|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC);
        if(self->hash_.value<0||self->image_.value<0||::fstat(self->hash_.value,&self->hashStat_)||!regular(self->hashStat_,0600,65)||
           self->hashStat_.st_size!=65||::fstat(self->image_.value,&self->imageStat_)||!regular(self->imageStat_,0700,4194304))return {};
        char hash[66];const auto count=::pread(self->hash_.value,hash,sizeof hash,0);
        if(count!=65||hash[64]!='\n')return {};
        for(unsigned i=0;i<64;++i)if(!((hash[i]>='0'&&hash[i]<='9')||(hash[i]>='a'&&hash[i]<='f')))return {};
        self->expectedHash_=QByteArray(hash,64);QCryptographicHash digest(QCryptographicHash::Sha256);
        std::array<char,4096> bytes{};off_t at=0;
        while(at<self->imageStat_.st_size){const auto amount=std::size_t(qMin<off_t>(off_t(bytes.size()),self->imageStat_.st_size-at));
            const auto n=::pread(self->image_.value,bytes.data(),amount,at);if(n!=ssize_t(amount))return {};
            digest.addData(QByteArrayView(bytes.data(),n));at+=n;}
        if(digest.result().toHex()!=self->expectedHash_||!self->unchanged())return {};
        return self;
    }
    bool unchanged()const noexcept {
        struct stat root{},image{},hash{},named{};char bytes[66];
        return root_.value>=0&&image_.value>=0&&hash_.value>=0&&!::fstat(root_.value,&root)&&root.st_dev==rootStat_.st_dev&&root.st_ino==rootStat_.st_ino&&
            root.st_mode==rootStat_.st_mode&&root.st_uid==rootStat_.st_uid&&!::fstat(image_.value,&image)&&same(image,imageStat_)&&
            !::fstatat(root_.value,"fixed-pen-writer",&named,AT_SYMLINK_NOFOLLOW)&&same(named,imageStat_)&&
            !::fstat(hash_.value,&hash)&&same(hash,hashStat_)&&!::fstatat(root_.value,"worker.sha256",&named,AT_SYMLINK_NOFOLLOW)&&same(named,hashStat_)&&
            ::pread(hash_.value,bytes,sizeof bytes,0)==65&&QByteArray(bytes,65)==expectedHash_+'\n';
    }
    bool postSpawnOkay()const noexcept{return spawned_&&postSpawnOkay_;}
    int error()const noexcept{return error_;}
    pid_t spawn(int writer,int start,int cancel,int report,pid_t parent,const reader_fixed_pen::Budget&budget,const struct stat&device)noexcept {
        if(spawned_)return -1;
        spawned_=true; // one use before descriptor/library callouts
        if(parent!=parent_||!budget.valid()||!unchanged()||reader_fixed_pen_holder::detail::processStart(parent_)!=parentStart_){error_=EINVAL;return -1;}
        std::array<Fd,4> copies{};const int originals[]={writer,start,cancel,report};
        for(unsigned i=0;i<4;++i){copies[i].value=::fcntl(originals[i],F_DUPFD_CLOEXEC,16);if(copies[i].value<0){error_=errno;return -1;}}
        char path[96];const int pathBytes=std::snprintf(path,sizeof path,"/proc/%ld/fd/%d",static_cast<long>(parent_),image_.value);
        if(pathBytes<=0||std::size_t(pathBytes)>=sizeof path){error_=EINVAL;return -1;}
        std::array<std::array<char,32>,6> arguments{};
        const std::uint64_t values[]={std::uint64_t(parent_),parentStart_,budget.began,std::uint64_t(device.st_dev),std::uint64_t(device.st_ino),std::uint64_t(device.st_rdev)};
        for(unsigned i=0;i<6;++i){const int n=std::snprintf(arguments[i].data(),arguments[i].size(),"%llu",static_cast<unsigned long long>(values[i]));if(n<1||std::size_t(n)>=arguments[i].size()){error_=EINVAL;return -1;}}
        char name[]="fixed-pen-writer",mode[]="--writer";
        char *argv[]={name,mode,arguments[0].data(),arguments[1].data(),arguments[2].data(),arguments[3].data(),arguments[4].data(),arguments[5].data(),nullptr};
        char *environment[]={nullptr};posix_spawn_file_actions_t actions;posix_spawnattr_t attributes;
        error_=::posix_spawn_file_actions_init(&actions);if(error_)return -1;
        const int initialized=::posix_spawnattr_init(&attributes);if(initialized){::posix_spawn_file_actions_destroy(&actions);error_=initialized;return -1;}
        const auto add=[&](int value){if(!error_&&value)error_=value;};
        add(::posix_spawn_file_actions_addopen(&actions,0,"/dev/null",O_RDWR,0));
        add(::posix_spawn_file_actions_adddup2(&actions,0,1));add(::posix_spawn_file_actions_adddup2(&actions,0,2));
        for(unsigned i=0;i<4;++i)add(::posix_spawn_file_actions_adddup2(&actions,copies[i].value,int(i)+3));
        add(::posix_spawn_file_actions_addclosefrom_np(&actions,7));
        sigset_t mask,defaults;
        if(::sigemptyset(&mask)||::sigemptyset(&defaults))add(errno?errno:EINVAL);
        for(int signal:{SIGPIPE,SIGCHLD,SIGTERM,SIGINT,SIGHUP,SIGALRM})if(::sigaddset(&defaults,signal))add(errno?errno:EINVAL);
        add(::posix_spawnattr_setsigmask(&attributes,&mask));add(::posix_spawnattr_setsigdefault(&attributes,&defaults));
        add(::posix_spawnattr_setflags(&attributes,POSIX_SPAWN_SETSIGMASK|POSIX_SPAWN_SETSIGDEF));
        pid_t child=0;if(!error_)error_=::posix_spawn(&child,path,&actions,&attributes,argv,environment);
        bool destroyed=::posix_spawn_file_actions_destroy(&actions)==0;
        destroyed=(::posix_spawnattr_destroy(&attributes)==0)&&destroyed;
        bool closed=true;for(auto &copy:copies)closed=copy.close()&&closed;
        if(error_)return -1;
        postSpawnOkay_=destroyed&&closed&&unchanged();
        return child; // never discard a known spawned PID, even on postcheck failure
    }
private:
    Image()=default;
    Fd root_,image_,hash_;struct stat rootStat_{},imageStat_{},hashStat_{};
    QByteArray expectedHash_;pid_t parent_=0;std::uint64_t parentStart_=0;
    bool spawned_=false,postSpawnOkay_=false;int error_=0;
};
} // namespace reader_fixed_pen_spawn
