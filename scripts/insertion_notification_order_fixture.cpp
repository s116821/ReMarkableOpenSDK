// Owned Qt experiment: emitter order is not a native insertion receipt.
#include <QCoreApplication>
#include <QEventLoop>
#include <QThread>
#include <QTimer>
#include <QStringList>
#include <QTextStream>
#include <cstdlib>

class Emitter final : public QThread {
    Q_OBJECT
public:
    bool withPage = true;
    void deliver() { if (withPage) emit pageAdded(42, 3); emit jobCompleted(42); }
    void run() override { deliver(); }
signals:
    void pageAdded(quint64 job, int index);
    void jobCompleted(quint64 job);
};

static void require(bool ok, const char *reason) {
    if (!ok) { QTextStream(stderr) << "FAIL " << reason << '\n'; std::exit(1); }
}

static void exercise(bool workerThread, bool nested, bool withPage, quint64 expectedJob,
                     const QStringList &expected) {
    Emitter emitter; // QObject affinity stays on the application thread.
    emitter.withPage = withPage;
    QObject task;
    QStringList seen;
    bool rightThread = true;
    QObject::connect(&emitter, &Emitter::pageAdded, &task, [&](quint64 job, int index) {
        rightThread &= QThread::currentThread() == task.thread();
        if (job != expectedJob) return;
        require(index == 3, "matching result index");
        seen.append(nested ? "page-enter" : "page");
        if (nested) {
            QEventLoop loop;
            QTimer::singleShot(0, &loop, &QEventLoop::quit);
            loop.exec();
            seen.append("page-exit");
        }
    }, Qt::QueuedConnection);
    QObject::connect(&emitter, &Emitter::jobCompleted, &task, [&](quint64 job) {
        rightThread &= QThread::currentThread() == task.thread();
        if (job == expectedJob) seen.append("finished");
    }, Qt::AutoConnection);
    if (workerThread) {
        emitter.start();
        require(emitter.wait(5000), "owned worker completion deadline");
    } else emitter.deliver();
    // Only this owned fixture pumps events, never the proposed sampler.
    QCoreApplication::sendPostedEvents(&task, QEvent::MetaCall);
    require(rightThread && seen == expected, "observed Qt notification order");
}

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    exercise(true, false, true, 42, {"page", "finished"});
    exercise(false, false, true, 42, {"finished", "page"});
    exercise(true, true, true, 42, {"page-enter", "finished", "page-exit"});
    exercise(true, false, false, 42, {"finished"});
    exercise(true, false, true, 99, {});
    QTextStream(stdout) << "PASS notification ordering: worker affinity, GUI direct completion, nested delivery, skipped page and foreign job; synthetic only\n";
}
#include "insertion_notification_order_fixture.moc"
