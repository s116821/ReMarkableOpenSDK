#include "activation.h"
#include <QTemporaryDir>
#include <QDir>
#include <cstdio>
#include <cstdlib>
#include <functional>
using namespace remarkable::resident;
namespace {
unsigned checks=0;
void check(bool yes){++checks;if(!yes){std::fprintf(stderr,"activation check %u failed\n",checks);std::abort();}}
ArtifactDigests hashes(){return {QByteArray(32,1),QByteArray(32,2),QByteArray(32,3)};}
void u32(QByteArray &bytes,int at,quint32 n){qToBigEndian<quint32>(n,reinterpret_cast<uchar*>(bytes.data()+at));}
void u64(QByteArray &bytes,int at,quint64 n){qToBigEndian<quint64>(n,reinterpret_cast<uchar*>(bytes.data()+at));}
QByteArray admission() {
    const auto expected=hashes();
    QByteArray bytes="RSDKACT1";bytes+=expected.profile;bytes+=expected.executable;bytes+=expected.adapter;
    bytes+=bootIdentity();bytes+=QByteArray(16,7);bytes+=QByteArray(32,0);
    check(bytes.size()==168);
    u32(bytes,136,quint32(::geteuid()));u32(bytes,140,quint32(::getppid()));
    u64(bytes,144,processStart(::getppid()));u64(bytes,152,bootMilliseconds()+59000);u32(bytes,160,30000);
    return bytes;
}
struct Fixture {
    QTemporaryDir root;
    Fixture(){check(root.isValid());check(::chmod(QFile::encodeName(root.path()).constData(),0700)==0);}
    QString path()const{return root.path()+QStringLiteral("/admission.bin");}
    void write(const QByteArray &bytes){QFile file(path());check(file.open(QIODevice::WriteOnly|QIODevice::Truncate));check(file.write(bytes)==bytes.size());file.close();check(::chmod(QFile::encodeName(path()).constData(),0600)==0);}
    bool consumed()const{return QFile::exists(root.path()+QStringLiteral("/admission.consumed"));}
};
void refused(const std::function<void(QByteArray&)> &edit) {
    Fixture fixture;auto bytes=admission();edit(bytes);fixture.write(bytes);
    check(!Activation::claim(fixture.root.path(),hashes()));check(!fixture.consumed());
}
}
int main(int argc,char **argv) {
    QCoreApplication app(argc,argv);
    check(::getppid()>1&&processStart(::getppid())>0);
    {
        Fixture fixture;fixture.write(admission());auto activation=Activation::claim(fixture.root.path(),hashes());
        check(bool(activation));check(fixture.consumed());check(activation->directoryCurrent());
        const auto &config=activation->config();
        check(config.runtime.size()==140&&config.runtime.left(96)==hashes().profile+hashes().executable+hashes().adapter);
        check(config.peerPid==::getppid()&&config.peerStart==processStart(::getppid())&&config.pinDirectory);
        check(!Activation::claim(fixture.root.path(),hashes()));
        Endpoint endpoint(config,{});check(endpoint.start());
    }
    for(const int at:{0,7,8,39,40,71,72,103,104,119,136,140,144,164,167})
        refused([at](QByteArray &bytes){bytes[at]=char(uchar(bytes[at])^0xffU);});
    refused([](QByteArray &bytes){bytes.replace(120,16,QByteArray(16,0));});
    refused([](QByteArray &bytes){u64(bytes,152,bootMilliseconds());});
    refused([](QByteArray &bytes){u64(bytes,152,bootMilliseconds()+120000);});
    refused([](QByteArray &bytes){u32(bytes,160,0);});
    refused([](QByteArray &bytes){u32(bytes,160,30001);});
    refused([](QByteArray &bytes){bytes.chop(1);});
    refused([](QByteArray &bytes){bytes.append('x');});
    {
        Fixture fixture;fixture.write(admission());auto expected=hashes();expected.adapter=QByteArray(32,0);
        check(!Activation::claim(fixture.root.path(),expected));check(!fixture.consumed());
        check(::chmod(QFile::encodeName(fixture.path()).constData(),0644)==0);
        check(!Activation::claim(fixture.root.path(),hashes()));check(!fixture.consumed());
    }
    {
        Fixture fixture;fixture.write(admission());check(::chmod(QFile::encodeName(fixture.root.path()).constData(),0755)==0);
        check(!Activation::claim(fixture.root.path(),hashes()));check(!fixture.consumed());
    }
    {
        Fixture fixture;fixture.write(admission());const auto other=fixture.root.path()+QStringLiteral("/original");
        check(QFile::rename(fixture.path(),other));check(::symlink(QFile::encodeName(other).constData(),QFile::encodeName(fixture.path()).constData())==0);
        check(!Activation::claim(fixture.root.path(),hashes()));check(!fixture.consumed());
    }
    {
        Fixture fixture;fixture.write(admission());const auto alias=fixture.root.path()+QStringLiteral("-link");
        check(::symlink(QFile::encodeName(fixture.root.path()).constData(),QFile::encodeName(alias).constData())==0);
        check(!Activation::claim(alias,hashes()));check(!fixture.consumed());check(::unlink(QFile::encodeName(alias).constData())==0);
    }
    {
        Fixture fixture;fixture.write(admission());const auto used=fixture.root.path()+QStringLiteral("/admission.consumed");
        QFile file(used);check(file.open(QIODevice::WriteOnly));file.close();
        check(!Activation::claim(fixture.root.path(),hashes()));check(QFile::exists(used));
    }
    {
        Fixture fixture;fixture.write(admission());QFile file(fixture.root.path()+QStringLiteral("/sdk.sock"));
        check(file.open(QIODevice::WriteOnly));file.close();check(!Activation::claim(fixture.root.path(),hashes()));check(!fixture.consumed());
    }
    {
        Fixture fixture;fixture.write(admission());auto activation=Activation::claim(fixture.root.path(),hashes());check(bool(activation));
        const auto original=fixture.root.path(),moved=original+QStringLiteral("-moved");
        check(QDir().rename(original,moved));check(QDir().mkdir(original));check(::chmod(QFile::encodeName(original).constData(),0700)==0);
        check(!activation->directoryCurrent());Endpoint endpoint(activation->config(),{});check(!endpoint.start());
        check(QDir(moved).removeRecursively());
    }
    std::printf("{\"passed\":true,\"checks\":%u,\"native_activation\":false}\n",checks);
}
