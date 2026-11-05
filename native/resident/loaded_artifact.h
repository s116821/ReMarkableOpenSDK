#pragma once
#include <QByteArray>
#include <QCryptographicHash>
#include <QFile>
#include <dlfcn.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#include <cerrno>
#include <cstdint>

namespace remarkable::resident {
// Hash the stable regular file backing an actual executable mapping containing
// this artifact's anchor. This is file/mapping correspondence, not a signature
// or protection against trusted same-UID code modifying its own process.
inline QByteArray loadedArtifactDigest(const void *anchor) {
    Dl_info info{};
    if(!anchor||!::dladdr(anchor,&info)||!info.dli_fname||!*info.dli_fname||!info.dli_fbase)return {};
    const int fd=::open(info.dli_fname,O_RDONLY|O_NOFOLLOW|O_CLOEXEC);
    if(fd<0)return {};
    struct Guard{int fd;~Guard(){::close(fd);}}guard{fd};
    struct stat before{},after{};
    if(::fstat(fd,&before)||!S_ISREG(before.st_mode)||before.st_uid!=::geteuid()||
       (before.st_mode&0022)||before.st_size<52||before.st_size>32*1024*1024)return {};
    QFile maps(QStringLiteral("/proc/self/maps"));
    if(!maps.open(QIODevice::ReadOnly))return {};
    const auto contents=maps.read(4*1024*1024);
    if(contents.size()>=4*1024*1024||!maps.atEnd())return {};
    unsigned matches=0;
    for(const auto &line:contents.split('\n')) {
        unsigned long long begin=0,end=0,offset=0,inode=0;unsigned deviceMajor=0,deviceMinor=0;
        char permission[5]{};
        if(std::sscanf(line.constData(),"%llx-%llx %4s %llx %x:%x %llu",&begin,&end,permission,&offset,&deviceMajor,&deviceMinor,&inode)!=7)continue;
        const auto address=reinterpret_cast<std::uintptr_t>(anchor);
        if(begin<=address&&address<end) {
            if(permission[0]!='r'||permission[1]=='w'||permission[2]!='x'||
               deviceMajor!=::major(before.st_dev)||deviceMinor!=::minor(before.st_dev)||inode!=before.st_ino)return {};
            ++matches;
        }
    }
    if(matches!=1)return {};
    QCryptographicHash hash(QCryptographicHash::Sha256);
    QByteArray block(65536,0);off_t bytes=0;
    while(bytes<before.st_size) {
        const auto count=::read(fd,block.data(),std::size_t(qMin<off_t>(block.size(),before.st_size-bytes)));
        if(count<0&&errno==EINTR)continue;
        if(count<=0)return {};
        hash.addData(QByteArrayView(block.constData(),qsizetype(count)));bytes+=count;
    }
    char extra=0;
    if(::read(fd,&extra,1)!=0||::fstat(fd,&after)||before.st_dev!=after.st_dev||before.st_ino!=after.st_ino||
       before.st_mode!=after.st_mode||before.st_uid!=after.st_uid||before.st_size!=after.st_size||
       before.st_mtim.tv_sec!=after.st_mtim.tv_sec||before.st_mtim.tv_nsec!=after.st_mtim.tv_nsec||
       before.st_ctim.tv_sec!=after.st_ctim.tv_sec||before.st_ctim.tv_nsec!=after.st_ctim.tv_nsec)return {};
    return hash.result();
}
}
