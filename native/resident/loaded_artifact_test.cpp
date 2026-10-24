#include "loaded_artifact.h"
#include <QCoreApplication>
#include <cstdio>
#include <cstdlib>
using namespace remarkable::resident;
namespace {
unsigned checks=0;
void check(bool value){++checks;if(!value){std::fprintf(stderr,"artifact check %u failed\n",checks);std::abort();}}
}
int main(int argc,char **argv) {
    QCoreApplication app(argc,argv);check(argc==2);
    const QByteArray path=argv[1];
    void *library=::dlopen(path.constData(),RTLD_NOW|RTLD_LOCAL);check(library!=nullptr);
    void *anchor=::dlsym(library,"owned_artifact_anchor");check(anchor!=nullptr);
    QFile file(QString::fromLocal8Bit(path));check(file.open(QIODevice::ReadOnly));
    const auto originalBytes=file.readAll();
    const auto expected=QCryptographicHash::hash(originalBytes,QCryptographicHash::Sha256);file.close();
    check(loadedArtifactDigest(anchor)==expected);
    check(loadedArtifactDigest(nullptr).isEmpty());
    int stack=0;check(loadedArtifactDigest(&stack).isEmpty());
    check(::chmod(path.constData(),0664)==0);check(loadedArtifactDigest(anchor).isEmpty());
    check(::chmod(path.constData(),0644)==0);check(loadedArtifactDigest(anchor)==expected);
    const auto moved=path+".moved";
    check(::rename(path.constData(),moved.constData())==0);
    check(file.open(QIODevice::WriteOnly|QIODevice::NewOnly));
    check(file.write(originalBytes)==originalBytes.size());file.close();
    check(loadedArtifactDigest(anchor).isEmpty()); // Same bytes, different mapped inode.
    check(::dlclose(library)==0); // Owned no-op library only, never the native adapter.
    std::printf("{\"passed\":true,\"checks\":%u,\"native_activation\":false}\n",checks);
}
