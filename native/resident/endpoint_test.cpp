#include "endpoint.h"
#include <QElapsedTimer>
#include <QTemporaryDir>
#include <QProcess>
#include <cstdio>
#include <cstdlib>
using namespace remarkable::resident;
static unsigned checks=0;
static void need(bool value){++checks;if(!value){std::fprintf(stderr,"endpoint check %u failed\n",checks);std::abort();}}
template<class F>static bool until(F predicate,int budget=1500) {
    QElapsedTimer clock;clock.start();
    while(!predicate()&&clock.elapsed()<budget){QCoreApplication::processEvents(QEventLoop::AllEvents,5);QThread::msleep(1);}
    return predicate();
}
static Frame vector(const QString &root,int index) {
    QFile file(root+'/'+QStringLiteral("%1.bin").arg(index,2,10,QChar('0')));need(file.open(QIODevice::ReadOnly));
    auto value=decode(file.readAll().mid(4));need(bool(value));return *value;
}
static EndpointConfig config(const QString &directory) {
    EndpointConfig value;value.directory=directory;value.peerUid=::geteuid();value.peerPid=::getpid();
    value.peerStart=processStart(::getpid());value.operationBudgetMs=100;
    detail::Writer runtime;runtime.bytes=QByteArray(32,1)+QByteArray(32,2)+QByteArray(32,3)+bootIdentity();
    runtime.number<quint32>(quint32(::getpid()));runtime.number<quint64>(value.peerStart);runtime.bytes+=Id(16,4);
    value.runtime=runtime.bytes;return value;
}
static std::optional<Frame> receive(QLocalSocket &socket,int budget=1500) {
    Decoder decoder;std::optional<Frame> value;
    until([&] {
        if(socket.bytesAvailable()) {const auto result=decoder.feed(socket.read(MaxFrame+4));need(!result.error);if(result.frame)value=result.frame;}
        return bool(value)||socket.state()==QLocalSocket::UnconnectedState;
    },budget);
    return value;
}
static Frame handshake(QLocalSocket &socket,const EndpointConfig &c,Frame hello) {
    socket.connectToServer(c.directory+"/sdk.sock");need(until([&]{return socket.state()==QLocalSocket::ConnectedState;}));
    hello.runtime=c.runtime;need(socket.write(encode(hello))>0);
    auto ready=receive(socket);need(bool(ready));need(ready->message==1&&validId(ready->server));return *ready;
}
static Frame request(Frame value,const Frame &ready,quint64 sequence) {
    value.runtime=ready.runtime;value.client=ready.client;value.server=ready.server;value.sequence=sequence;return value;
}
int main(int argc,char **argv) {
    QCoreApplication app(argc,argv);
    if(argc==4&&QByteArray(argv[1])=="--rust-server") {
        QTemporaryDir directory;need(directory.isValid());auto c=config(directory.path());
        bool ok=false;c.peerPid=QString::fromLocal8Bit(argv[2]).toInt(&ok);need(ok&&c.peerPid>0);c.peerStart=processStart(c.peerPid);
        const QString vectors=QString::fromLocal8Bit(argv[3]);const auto facts=vector(vectors,5);
        Endpoint *endpoint=nullptr;unsigned submitted=0;
        Endpoint server(c,[&](Endpoint::Lease lease){++submitted;if(lease->request().kind==0)need(endpoint->finish(lease,facts));});endpoint=&server;need(server.start());
        auto hello=vector(vectors,0);hello.runtime=c.runtime;const auto encoded=encode(hello);
        std::printf("%s\n",QFile::encodeName(directory.path()).constData());std::fflush(stdout);
        need(::write(STDOUT_FILENO,encoded.constData(),std::size_t(encoded.size()))==encoded.size());
        QSocketNotifier shutdown(STDIN_FILENO,QSocketNotifier::Read,&app);
        QObject::connect(&shutdown,&QSocketNotifier::activated,&app,[&]{char byte=0;need(::read(STDIN_FILENO,&byte,1)==1&&byte=='Q');std::printf("submissions=%u\n",submitted);std::fflush(stdout);app.quit();});
        QTimer::singleShot(10000,&app,&QCoreApplication::quit);return app.exec();
    }
    if(argc==5&&QByteArray(argv[1])=="--client") {
        char go=0;need(::read(STDIN_FILENO,&go,1)==1&&go=='G');
        auto c=config(QString::fromLocal8Bit(argv[2]));c.runtime=QByteArray::fromHex(argv[3]);
        const QString vectors=QString::fromLocal8Bit(argv[4]);QLocalSocket peer;
        const auto ready=handshake(peer,c,vector(vectors,0));const auto value=request(vector(vectors,2),ready,1);
        need(peer.write(encode(value))>0);const auto result=receive(peer);
        need(bool(result)&&result->kind==0&&result->requestDigest==fingerprint(value));
        std::printf("PASS: separate process peer credentials and exchange\n");return 0;
    }
    need(argc==2);const QString vectors=QString::fromLocal8Bit(argv[1]);
    const auto hello=vector(vectors,0),observe=vector(vectors,2),insert=vector(vectors,3),facts=vector(vectors,5);
    {
        QTemporaryDir directory;auto c=config(directory.path());QProcess child;
        QString program=QCoreApplication::applicationFilePath();
        QStringList arguments={"--client",c.directory,QString::fromLatin1(c.runtime.toHex()),vectors};
        const auto qemu=qEnvironmentVariable("RESIDENT_QEMU");
        if(!qemu.isEmpty()){arguments.prepend(program);arguments.prepend(qEnvironmentVariable("RESIDENT_QEMU_SYSROOT"));arguments.prepend("-L");program=qemu;}
        child.start(program,arguments);need(child.waitForStarted(2000));c.peerPid=pid_t(child.processId());c.peerStart=processStart(c.peerPid);need(c.peerStart!=0);
        Endpoint *endpoint=nullptr;unsigned submitted=0;
        Endpoint server(c,[&](Endpoint::Lease lease){++submitted;need(endpoint->finish(lease,facts));});endpoint=&server;need(server.start());
        need(child.write("G")==1);need(until([&]{return child.state()==QProcess::NotRunning;},5000));
        need(child.exitStatus()==QProcess::NormalExit&&child.exitCode()==0);need(submitted==1);
        need(child.readAllStandardOutput().contains("PASS: separate process"));
    }
    {
        QTemporaryDir directory;need(directory.isValid());auto c=config(directory.path());unsigned submitted=0;
        Endpoint *endpoint=nullptr;Endpoint server(c,[&](Endpoint::Lease lease){++submitted;need(lease->active());need(endpoint->finish(lease,facts));need(!lease->active());});endpoint=&server;
        need(server.start());need(!server.start());struct stat socketStat{};
        need(!::lstat(QFile::encodeName(c.directory+"/sdk.sock").constData(),&socketStat)&&(socketStat.st_mode&07777)==0600);
        QLocalSocket peer;auto ready=handshake(peer,c,hello);
        for(unsigned index=1;index<=200;++index) {
            const auto operation=request(observe,ready,index);const auto bytes=encode(operation);
            // Real socket fragmentation without waiting for the partial frame.
            need(peer.write(bytes.left(3))==3);peer.flush();QCoreApplication::processEvents();
            need(peer.write(bytes.mid(3))==bytes.size()-3);auto result=receive(peer);need(bool(result));
            need(result->sequence==index&&result->requestDigest==fingerprint(operation)&&result->kind==0);
        }
        need(submitted==200);
        const auto stale=request(observe,ready,200);need(peer.write(encode(stale))>0);
        need(until([&]{return peer.state()==QLocalSocket::UnconnectedState;}));need(submitted==200);
        QLocalSocket next;const auto nextReady=handshake(next,c,hello);need(nextReady.server!=ready.server);
        need(next.write(encode(request(observe,ready,1)))>0);
        need(until([&]{return next.state()==QLocalSocket::UnconnectedState;}));need(submitted==200);
    }
    {
        QTemporaryDir directory;auto c=config(directory.path());Endpoint::Lease retained;unsigned submitted=0;
        Endpoint server(c,[&](Endpoint::Lease lease){++submitted;retained=std::move(lease);});need(server.start());
        QLocalSocket peer;auto ready=handshake(peer,c,hello);need(peer.write(encode(request(insert,ready,1)))>0);
        need(until([&]{return bool(retained);}));need(retained->active());
        need(until([&]{return peer.state()==QLocalSocket::UnconnectedState;}));need(!retained->active());
        need(!server.finish(retained,facts));need(submitted==1);
        QLocalSocket next;auto nextReady=handshake(next,c,hello);need(nextReady.server!=ready.server);
        need(next.write(encode(request(observe,nextReady,1)))>0);need(until([&]{return submitted==2;}));
        next.abort();need(until([&]{return !retained->active();}));need(!server.finish(retained,facts));
    }
    {
        QTemporaryDir directory;auto c=config(directory.path());++c.peerStart;unsigned submitted=0;
        Endpoint server(c,[&](Endpoint::Lease){++submitted;});need(server.start());
        QLocalSocket peer;peer.connectToServer(c.directory+"/sdk.sock");
        need(until([&]{return peer.state()==QLocalSocket::UnconnectedState;}));need(submitted==0);
    }
    {
        QTemporaryDir directory;auto c=config(directory.path());unsigned submitted=0;
        Endpoint server(c,[&](Endpoint::Lease){++submitted;throw 1;});need(server.start());
        QLocalSocket peer;auto ready=handshake(peer,c,hello);need(peer.write(encode(request(insert,ready,1)))>0);
        need(until([&]{return peer.state()==QLocalSocket::UnconnectedState;}));need(submitted==1);
    }
    // Retire the submitting connection inside a nested event loop while the
    // outer read still holds a coalesced Hello. Reentry cannot admit a new peer.
    for(bool throws:{false,true}) {
        QTemporaryDir directory;auto c=config(directory.path());unsigned submitted=0;
        QLocalSocket peer,reentrant;Endpoint *endpoint=nullptr;
        Endpoint server(c,[&](Endpoint::Lease lease){
            ++submitted;
            if(submitted>1){need(endpoint->finish(lease,facts));return;}
            peer.abort();need(until([&]{return !lease->active();}));
            reentrant.connectToServer(c.directory+"/sdk.sock");
            need(until([&]{return reentrant.state()==QLocalSocket::UnconnectedState;}));
            if(throws)throw 1;
        });endpoint=&server;need(server.start());
        const auto ready=handshake(peer,c,hello);auto oldHello=hello;oldHello.runtime=c.runtime;
        need(peer.write(encode(request(insert,ready,1))+encode(oldHello))>0);
        need(until([&]{return submitted==1;}));
        QLocalSocket next;const auto nextReady=handshake(next,c,hello);need(nextReady.server!=ready.server);
        const auto operation=request(observe,nextReady,1);need(next.write(encode(operation))>0);
        const auto result=receive(next);need(bool(result)&&result->requestDigest==fingerprint(operation));need(submitted==2);
    }
    // Startup permission restriction must reject both ordinary-file and symlink
    // replacement without chmod of the replacement/target or identity adoption.
    for(bool symlink:{false,true}) {
        QTemporaryDir directory;const auto path=QFile::encodeName(directory.path()+"/sdk.sock");
        const int root=::open(QFile::encodeName(directory.path()).constData(),O_RDONLY|O_DIRECTORY|O_CLOEXEC);need(root>=0);
        const int listener=::socket(AF_UNIX,SOCK_STREAM|SOCK_CLOEXEC,0);need(listener>=0);
        sockaddr_un address{};address.sun_family=AF_UNIX;need(path.size()<qsizetype(sizeof address.sun_path));
        std::memcpy(address.sun_path,path.constData(),std::size_t(path.size()));
        need(!::bind(listener,reinterpret_cast<sockaddr*>(&address),sizeof address));
        struct stat bound{},restricted{};need(!::lstat(path.constData(),&bound));need(!::unlink(path.constData()));
        const auto target=symlink?path+".target":path;QFile replacement(QString::fromLocal8Bit(target));
        need(replacement.open(QIODevice::WriteOnly));need(replacement.write("retained")==8);replacement.close();need(!::chmod(target.constData(),0640));
        if(symlink)need(!::symlink(target.constData(),path.constData()));
        need(!restrictSocket(root,bound,restricted));need(restricted.st_ino==0);
        struct stat after{};need(!::stat(target.constData(),&after)&&(after.st_mode&07777)==0640);
        need(replacement.open(QIODevice::ReadOnly));need(replacement.readAll()=="retained");
        ::close(listener);::close(root);
    }
    {
        QTemporaryDir directory;auto c=config(directory.path());auto server=std::make_unique<Endpoint>(c,Endpoint::Submit{});need(server->start());
        QLocalSocket peer;const auto ready=handshake(peer,c,hello);need(peer.write(encode(request(observe,ready,1)))>0);
        const auto result=receive(peer);need(bool(result)&&result->kind==3&&result->rejection==0);
        const auto path=c.directory+"/sdk.sock";need(QFile::remove(path));QFile replacement(path);
        need(replacement.open(QIODevice::WriteOnly));need(replacement.write("retained")==8);replacement.close();
        server.reset();need(replacement.open(QIODevice::ReadOnly));need(replacement.readAll()=="retained");
    }
    std::printf("PASS: owned local socket endpoint, %u checks\n",checks);
}
