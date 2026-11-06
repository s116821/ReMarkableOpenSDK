// Owned Qt/socket regression. No Admission/Facilities or target facts are minted.
#include "../rm2_3_28_0_172/read_only_backend.h"
#include "../rm1_observation/read_only_backend.h"
#include <QElapsedTimer>
#include <QTemporaryDir>
#include <cassert>
#include <cstdio>
#include <type_traits>
#ifdef NDEBUG
#error Dispatcher fixture assertions must remain enabled.
#endif
using namespace remarkable::resident;
static_assert(!std::is_constructible_v<ReadOnlyDispatch,QObject*>);
template<class F> bool until(F predicate) {
    QElapsedTimer clock;clock.start();
    while(!predicate()&&clock.elapsed()<1500){QCoreApplication::processEvents(QEventLoop::AllEvents,5);QThread::msleep(1);}
    return predicate();
}
static std::optional<Frame> receive(QLocalSocket &socket) {
    Decoder decoder;std::optional<Frame> result;
    until([&]{if(socket.bytesAvailable()){
        const auto read=decoder.feed(socket.read(MaxFrame+4));assert(!read.error);if(read.frame)result=read.frame;
    }return bool(result)||socket.state()==QLocalSocket::UnconnectedState;});
    return result;
}
int main(int argc,char **argv) {
    QGuiApplication app(argc,argv);
    QTemporaryDir directory;assert(directory.isValid());
    EndpointConfig config;config.directory=directory.path();config.peerUid=::geteuid();config.peerPid=::getpid();
    config.peerStart=processStart(::getpid());config.operationBudgetMs=1000;
    detail::Writer runtime;runtime.bytes=QByteArray(32,1)+QByteArray(32,2)+QByteArray(32,3)+bootIdentity();
    runtime.number<quint32>(quint32(::getpid()));runtime.number<quint64>(config.peerStart);runtime.bytes+=Id(16,4);
    config.runtime=runtime.bytes;
    remarkable::rm2_3_28_0_172::ReadOnlyBackend rm2(nullptr);
    remarkable::rm1_observation::ReadOnlyBackend rm1(nullptr);
    unsigned submissions=0;
    Endpoint endpoint(config,[&](Endpoint::Lease lease){++submissions;rm2.submit(std::move(lease));});
    assert(!rm1.attach(&endpoint)); // No sealed RM1 record, no attached route.
    assert(rm2.attach(&endpoint));assert(!rm2.attach(&endpoint));assert(endpoint.start());
    QLocalSocket socket;socket.connectToServer(config.directory+"/sdk.sock");
    assert(until([&]{return socket.state()==QLocalSocket::ConnectedState;}));
    Frame hello;hello.runtime=config.runtime;hello.client=Id(16,5);hello.server=Id(16,0);hello.operation=Id(16,6);
    const auto encoded=encode(hello);assert(!encoded.isEmpty()&&socket.write(encoded)==encoded.size());
    auto ready=receive(socket);assert(ready&&ready->message==1);
    for(unsigned char kind=0;kind<=6;++kind){
        Frame request=*ready;request.message=2;request.sequence=quint64(kind)+1;request.kind=kind;
        request.facts={Id(16,7),Id(16,8),{Id(16,8)}};
        request.target=kind<=2?Id(16,9):request.facts.page;
        const auto bytes=encode(request);assert(!bytes.isEmpty()&&socket.write(bytes)==bytes.size());
        const auto response=receive(socket);
        assert(response&&response->kind==3&&response->rejection==0&&response->requestDigest==fingerprint(request));
        assert(response->facts.order.empty()&&response->capture.png.isEmpty());
    }
    assert(submissions==7);
    std::puts("PASS real endpoint rejects all commands without original RM2 Facilities; null RM1 admission cannot attach");
}
