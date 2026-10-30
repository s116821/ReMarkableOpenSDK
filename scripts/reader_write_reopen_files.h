#pragma once
// Bounded private research files. No process/unit/native admission by itself.
#include <QByteArray>
#include <QFile>
#include <QString>
#include <memory>
#include <atomic>
#include <utility>
#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace reader_write_reopen_files {
struct Read {bool observed=false,missing=false;QByteArray bytes;QString refusal;};
struct Publish {bool attempted=false,linked=false,committed=false;QString refusal;};
inline bool same(const struct stat &a,const struct stat &b){
    return a.st_dev==b.st_dev&&a.st_ino==b.st_ino&&a.st_mode==b.st_mode&&a.st_uid==b.st_uid&&a.st_gid==b.st_gid&&
        a.st_nlink==b.st_nlink&&a.st_size==b.st_size&&a.st_mtim.tv_sec==b.st_mtim.tv_sec&&a.st_mtim.tv_nsec==b.st_mtim.tv_nsec&&
        a.st_ctim.tv_sec==b.st_ctim.tv_sec&&a.st_ctim.tv_nsec==b.st_ctim.tv_nsec;
}
inline bool dataSame(const struct stat &a,const struct stat &b){
    return a.st_dev==b.st_dev&&a.st_ino==b.st_ino&&a.st_mode==b.st_mode&&a.st_uid==b.st_uid&&a.st_gid==b.st_gid&&
        a.st_size==b.st_size&&a.st_mtim.tv_sec==b.st_mtim.tv_sec&&a.st_mtim.tv_nsec==b.st_mtim.tv_nsec;
}
inline bool name(const char *s){
    if(!s||!*s||!qstrcmp(s,".")||!qstrcmp(s,".."))return false;
    unsigned count=0;for(;*s;++s){if(++count>64||!((*s>='a'&&*s<='z')||(*s>='0'&&*s<='9')||*s=='-'||*s=='.'))return false;}return true;
}
class Directory final:public std::enable_shared_from_this<Directory> {
    struct Fd {
        int value=-1;
        explicit Fd(int n=-1):value(n){}~Fd(){if(value>=0)::close(value);}
        Fd(const Fd&)=delete;Fd&operator=(const Fd&)=delete;
        bool close(){const int n=std::exchange(value,-1);return n<0||::close(n)==0;}
    };
    struct Operation {
        Directory &owner;bool entered;
        explicit Operation(Directory &d):owner(d),entered(!d.busy_.exchange(true)){if(!entered)d.revoked_=true;}
        ~Operation(){if(entered)owner.busy_=false;}
    };
public:
    // Serialized caller only. Retain shared Directory, never close this borrowed
    // FD, and do not retain the number across retirement or a caller callout.
    int borrowedFd()const{return root_;}
    Directory(const Directory&)=delete;Directory&operator=(const Directory&)=delete;
    ~Directory(){if(root_>=0)::close(root_);}
    static std::shared_ptr<Directory> open(const QString &path){
        Fd fd(::open(QFile::encodeName(path).constData(),O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC));
        if(fd.value<0)return {};
        auto self=std::shared_ptr<Directory>(new Directory(path,std::exchange(fd.value,-1)));
        if(::fstat(self->root_,&self->stat_)||!S_ISDIR(self->stat_.st_mode)||(self->stat_.st_mode&07777)!=0700||self->stat_.st_uid!=::geteuid()||!self->bound())return {};
        return self;
    }
    bool bound()const{
        struct stat current{},named{};
        return !revoked_&&root_>=0&&!::fstat(root_,&current)&&S_ISDIR(current.st_mode)&&current.st_dev==stat_.st_dev&&current.st_ino==stat_.st_ino&&
            current.st_mode==stat_.st_mode&&current.st_uid==stat_.st_uid&&current.st_gid==stat_.st_gid&&
            !::lstat(QFile::encodeName(path_).constData(),&named)&&named.st_dev==current.st_dev&&named.st_ino==current.st_ino&&
            named.st_mode==current.st_mode&&named.st_uid==current.st_uid&&named.st_gid==current.st_gid;
    }
    bool absent(const char *file)const{
        if(!name(file)||!bound())return false;
        struct stat s{};return ::fstatat(root_,file,&s,AT_SYMLINK_NOFOLLOW)&&errno==ENOENT;
    }
    Read read(const char *file,qint64 cap,bool allowEmpty=false){
        const auto self=shared_from_this();Operation operation(*self);Read r;
        const auto no=[&](const char *why){r.refusal=QString::fromLatin1(why);return r;};
        if(!operation.entered||!name(file)||cap<0||cap>131072||!bound())return no("read-admission");
        Fd fd(::openat(root_,file,O_RDONLY|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC));
        if(fd.value<0){r.missing=errno==ENOENT;return no(r.missing?"missing":"read-open");}
        struct stat before{},after{},named{};
        if(::fstat(fd.value,&before)||!regular(before,cap)||(!allowEmpty&&!before.st_size))return no("read-file");
        r.bytes.reserve(int(before.st_size));
        char data[4096];off_t left=before.st_size;
        while(left>0){
            if(!bound())return no("read-root");
            const auto want=size_t(qMin<off_t>(left,sizeof data));const auto n=::read(fd.value,data,want);
            if(n<1||size_t(n)>want)return no("read-size");
            r.bytes.append(data,int(n));left-=n;
        }
        if(::read(fd.value,data,1)!=0||::fstat(fd.value,&after)||!same(before,after)||
           ::fstatat(root_,file,&named,AT_SYMLINK_NOFOLLOW)||!same(after,named)||!bound())return no("read-binding");
        Fd pin(::openat(root_,file,O_PATH|O_NOFOLLOW|O_CLOEXEC));struct stat pinned{};
        if(pin.value<0||::fstat(pin.value,&pinned)||!same(after,pinned)||!fd.close()||
           ::fstatat(root_,file,&named,AT_SYMLINK_NOFOLLOW)||!same(pinned,named)||!bound())return no("read-close-binding");
        if(!pin.close())return no("read-close");
        r.observed=true;return r;
    }
    Publish publish(const char *file,const char *pending,const QByteArray &bytes){
        const auto self=shared_from_this();Operation operation(*self);Publish r;
        const auto no=[&](const char *why){r.refusal=QString::fromLatin1(why);return r;};
        if(!operation.entered||!name(file)||!name(pending)||!qstrcmp(file,pending)||bytes.isEmpty()||bytes.size()>4096||!absent(file)||!absent(pending))return no("publish-admission");
        r.attempted=true;
        Fd fd(::openat(root_,pending,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600));
        if(fd.value<0)return no("publish-open");
        struct stat initial{},sealed{},pinned{},named{},linked{};
        if(::fstat(fd.value,&initial)||!regular(initial,0)||initial.st_size)return no("publish-empty");
        if(::write(fd.value,bytes.constData(),size_t(bytes.size()))!=bytes.size()||::fstat(fd.value,&sealed)||!regular(sealed,4096)||
           sealed.st_size!=bytes.size()||sealed.st_dev!=initial.st_dev||sealed.st_ino!=initial.st_ino)return no("publish-write");
        Fd pin(::openat(root_,pending,O_PATH|O_NOFOLLOW|O_CLOEXEC));
        if(pin.value<0||::fstat(pin.value,&pinned)||!same(sealed,pinned)||!fd.close()||!bound()||
           ::fstatat(root_,pending,&named,AT_SYMLINK_NOFOLLOW)||!same(sealed,named)||!absent(file))return no("publish-binding");
        if(::linkat(root_,pending,root_,file,0))return no("publish-link");
        r.linked=true;
        if(::fstat(pin.value,&pinned)||pinned.st_nlink!=2||!dataSame(sealed,pinned)||::fstatat(root_,pending,&named,AT_SYMLINK_NOFOLLOW)||
           !same(pinned,named)||::fstatat(root_,file,&linked,AT_SYMLINK_NOFOLLOW)||!same(pinned,linked)||!bound())return no("publish-linked");
        if(::unlinkat(root_,pending,0)||::fstat(pin.value,&pinned)||pinned.st_nlink!=1||!dataSame(sealed,pinned)||
           ::fstatat(root_,file,&named,AT_SYMLINK_NOFOLLOW)||!same(pinned,named)||!absent(pending)||!bound()||!pin.close())return no("publish-final");
        r.committed=true;return r;
    }
    // Caller owns this new descriptor and must check close before advancing.
    int makeCapture(const char *directory){
        const auto self=shared_from_this();Operation operation(*self);
        if(!operation.entered||!name(directory)||!absent(directory)||::mkdirat(root_,directory,0700))return -1;
        Fd fd(::openat(root_,directory,O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC));struct stat s{},named{};
        if(fd.value<0||::fstat(fd.value,&s)||!S_ISDIR(s.st_mode)||(s.st_mode&07777)!=0700||s.st_uid!=stat_.st_uid||
           ::fstatat(root_,directory,&named,AT_SYMLINK_NOFOLLOW)||!same(s,named)||!bound())return -1;
        return std::exchange(fd.value,-1);
    }
    bool retire(){
        const auto self=shared_from_this();Operation operation(*self);
        if(!operation.entered||retired_)return false;
        retired_=true;revoked_=true;const int fd=std::exchange(root_,-1);return fd>=0&&::close(fd)==0;
    }
private:
    Directory(QString path,int fd):path_(std::move(path)),root_(fd){}
    bool regular(const struct stat &s,qint64 cap)const{return S_ISREG(s.st_mode)&&(s.st_mode&07777)==0600&&s.st_uid==stat_.st_uid&&s.st_gid==stat_.st_gid&&s.st_nlink==1&&s.st_size>=0&&s.st_size<=cap;}
    QString path_;int root_=-1;struct stat stat_{};
    std::atomic<bool> busy_{false},revoked_{false};bool retired_=false;
};
} // namespace reader_write_reopen_files
