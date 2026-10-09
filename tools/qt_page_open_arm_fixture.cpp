#include "qt_page_open_arm.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QElapsedTimer>
#include <cstdio>
using namespace qml_access;
int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    if (argc != 2) return 2;
    QByteArray mode(argv[1]);
    const bool creation = mode.startsWith("creation-");
    if (creation) mode.remove(0, 9);
    const QString arm = creation ? "creation-arm" : "open-arm";
    const QString waiting = creation ? "creation-waiting" : "open-waiting";
    const QString temporary = creation ? "creation-arm.tmp" : "open-arm.tmp";
    const QString accepted = creation ? "creation-arm-accepted" : "open-arm-accepted";
    QTemporaryDir root;
    if (!root.isValid()) return 3;
    ::chmod(QFile::encodeName(root.path()).constData(), 0700);
    const QString nonce = QStringLiteral("0123456789abcdef0123456789abcdef");
    const auto writeFile = [&](const QString &name, const QByteArray &bytes, mode_t permissions=0600) {
        QFile file(root.filePath(name));
        if (!file.open(QIODevice::WriteOnly | QIODevice::NewOnly) || file.write(bytes) != bytes.size()) return false;
        file.close();
        return ::chmod(QFile::encodeName(root.filePath(name)).constData(), permissions) == 0;
    };
    if (mode == "stale") writeFile(arm, "old");
    if (mode == "stale-marker") writeFile(waiting, "old");
    if (mode == "stale-temp") writeFile(temporary, "old");
    if (mode == "closed") writeFile("entry.closed", "old");
    if (mode == "stopping") writeFile("stop.request", "old");
    if (mode == "restoring") writeFile("restore.claim", "old");
    if (mode == "directory-mode") ::chmod(QFile::encodeName(root.path()).constData(), 0755);
    bool progress = true;
    int completions = 0;
    QString stage;
    QElapsedTimer clock; clock.start();
    PageOpenArmGate gate(&app, root.path(), nonce, [&] { return progress && clock.elapsed() < 100; },
        [&](const char *value) {
            ++completions; stage=QString::fromLatin1(value);
            if (mode == "closed-before-release") writeFile("entry.closed", "closed");
            if (mode != "duplicate") app.quit();
        }, creation ? PageOpenArmGate::Operation::Creation : PageOpenArmGate::Operation::PageOpen, creation ? 123456 : 0);
    gate.begin(); gate.begin(); // A duplicate begin cannot publish/reopen the gate.
    bool markerValid = !creation || (!QFile::exists(root.filePath("open-waiting")) &&
        gate.armBytes().endsWith(" create 123456\n"));
    if (QFile::exists(root.filePath(waiting)) && mode != "stale-marker") {
        QFile marker(root.filePath(waiting)); if (!marker.open(QIODevice::ReadOnly)) return 4;
        markerValid = marker.readAll() == gate.waitingBytes();
    }
    const auto publish = [&] {
        QByteArray bytes = gate.armBytes();
        if (mode == "wrong") bytes[0] = 'f';
        if (mode == "wrong-process") bytes.replace(PageOpenArmGate::processIdentity(), "123 456");
        if (mode == "partial") bytes.chop(1);
        if (mode == "oversized") bytes.append(200, 'x');
        if (mode == "symlink") {
            writeFile("other", bytes);
            ::symlink(QFile::encodeName(root.filePath("other")).constData(), QFile::encodeName(root.filePath(arm)).constData());
        } else writeFile(arm, bytes, mode == "token-mode" ? 0644 : 0600);
    };
    if (mode == "immediate") publish();
    else if (mode == "good" || mode == "closed-before-release" || mode == "duplicate" || mode == "wrong" || mode == "wrong-process" || mode == "partial" || mode == "oversized" || mode == "token-mode" || mode == "symlink")
        QTimer::singleShot(10, &app, publish);
    else if (mode == "unrelated") QTimer::singleShot(10, &app, [&] { writeFile("unrelated", "ignored"); });
    else if (mode == "cancel") { progress=false; gate.disarm(); publish(); }
    else if (mode == "cutoff") { progress=false; publish(); }
    else if (mode == "late") QTimer::singleShot(120, &app, publish);
    else if (mode == "closed-after") { writeFile("entry.closed", "closed"); publish(); }
    else if (mode == "restore-after") { writeFile("restore.claim", "closed"); publish(); }
    else if (mode == "replacement") {
        QDir().rename(root.path(), root.path()+"-old"); QDir().mkdir(root.path());
        ::chmod(QFile::encodeName(root.path()).constData(), 0700); publish();
    }
    if (mode == "duplicate") QTimer::singleShot(30, &app, [&] { writeFile("unrelated", "event"); gate.begin(); });
    QTimer::singleShot(180, &app, &QCoreApplication::quit);
    app.exec();
    const bool good = mode == "good" || mode == "closed-before-release" || mode == "immediate" || mode == "duplicate";
    bool ok = markerValid && (good ? completions == 1 && stage == accepted :
        mode == "none" || mode == "cancel" || mode == "unrelated" ? completions == 0 : completions == 1 && stage != accepted);
    if (good) ok = ok && gate.releaseAllowed() == (mode != "closed-before-release" && clock.elapsed() < 100);
    if (mode == "replacement") QDir(root.path()+"-old").removeRecursively();
    std::printf("%s stage=%s completions=%d %s\n", mode.constData(), qPrintable(stage), completions, ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
