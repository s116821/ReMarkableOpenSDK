// Owned signal plumbing only; no tablet/native application or production nonce.
#define QT_SHUTDOWN_NONCE "00000000000000000000000000000000"
#include "qt_shutdown_trace_startup.cpp"
#include <cassert>
#include <QFile>
#include <QTemporaryDir>

static void put(const QByteArray &path,const QByteArray &bytes){
    int fd=::open(path.constData(),O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC,0600);
    assert(fd>=0);assert(::write(fd,bytes.constData(),size_t(bytes.size()))==bytes.size());::close(fd);
}
int main(int argc,char **argv){
    constexpr auto root="/run/rmb-qt-shutdown-" QT_SHUTDOWN_NONCE;
    assert(::mkdir(root,0700)==0);
    const auto identity=QByteArray::number(::getpid())+' '+shutdown_trace::processStart()+'\n';
    put(QByteArray(root)+"/owner",QT_SHUTDOWN_NONCE);
    put(QByteArray(root)+"/attempt.identity",identity);
    {
        QGuiApplication app(argc,argv);
        QCoreApplication::processEvents();
        assert(!shutdown_trace::Recorder::open(root,QT_SHUTDOWN_NONCE)); // exclusive output
        auto *window=new QQuickWindow;
        assert(QMetaObject::invokeMethod(&app,"focusWindowChanged",Qt::DirectConnection,Q_ARG(QWindow *,window)));
        for(int i=0;i<40;++i){
            assert(QMetaObject::invokeMethod(window,"beforeRendering",Qt::DirectConnection));
            assert(QMetaObject::invokeMethod(window,"afterRendering",Qt::DirectConnection));
        }
        assert(QMetaObject::invokeMethod(&app,"aboutToQuit",Qt::DirectConnection));
        for(int i=0;i<2;++i){
            assert(QMetaObject::invokeMethod(window,"beforeRendering",Qt::DirectConnection));
            assert(QMetaObject::invokeMethod(window,"afterRendering",Qt::DirectConnection));
        }
        delete window;
    }
    QFile file(QByteArray(root)+"/shutdown-trace.log");assert(file.open(QIODevice::ReadOnly));
    const auto bytes=file.readAll();const auto lines=bytes.trimmed().split('\n');
    assert(lines.size()==71); // 2 startup/window +64 frame +quit+late pair+2 destruction
    unsigned seq=0;for(const auto &line:lines){
        auto f=line.split(' ');assert(f.size()==10 && f[0]=="v1");
        assert(f[1].toUInt()==++seq && f[8]=="0" && line.size()<192);
    }
    assert(bytes.count(" before-render ")==32 && bytes.count(" after-render ")==32);
    assert(bytes.count(" late-before-render ")==1 && bytes.count(" late-after-render ")==1);
    assert(lines.back().contains(" application-destroyed ") && lines.back().endsWith(" 1 0 42"));
    QTemporaryDir bad;assert(bad.isValid());
    put(bad.path().toUtf8()+"/owner","wrong");put(bad.path().toUtf8()+"/attempt.identity",identity);
    assert(!shutdown_trace::Recorder::open(bad.path().toUtf8().constData(),QT_SHUTDOWN_NONCE));
    assert(!QFile::exists(bad.path()+"/shutdown-trace.log"));
    QTemporaryDir wrongIdentity;assert(wrongIdentity.isValid());
    put(wrongIdentity.path().toUtf8()+"/owner",QT_SHUTDOWN_NONCE);
    put(wrongIdentity.path().toUtf8()+"/attempt.identity","1 1\n");
    assert(!shutdown_trace::Recorder::open(wrongIdentity.path().toUtf8().constData(),QT_SHUTDOWN_NONCE));
    shutdown_trace::Recorder failed;
    failed.writing.test_and_set();failed.record(shutdown_trace::Event::Startup);
    assert(failed.dropped==1 && !failed.failed);
    failed.writing.clear();failed.record(shutdown_trace::Event::Startup);
    assert(failed.failed); // invalid fd is a latched write failure, never retried
    const auto failedSequence=failed.sequence;failed.record(shutdown_trace::Event::Startup);
    assert(failed.sequence==failedSequence);
    std::puts("PASS owned lifecycle signals, 32-pair cap, post-quit markers, destruction, exclusive output, wrong owner/identity, contention and I/O failure");
}
