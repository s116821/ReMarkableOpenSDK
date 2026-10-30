/* Owned host application only; preload the separately built probe. */
#include <QCoreApplication>
#include <QTimer>
#include <cassert>
#include <cstring>
#include <fstream>
#include <string>
#include <unistd.h>

#ifndef QT_PROBE_NONCE
#error Define the same frozen QT_PROBE_NONCE as the preloaded library.
#endif

int main(int argc, char **argv) {
    const char *path = "/run/rmb-qt-probe-" QT_PROBE_NONCE "/callback.json";
    const bool preserve = argc == 2 && std::strcmp(argv[1], "preserve") == 0;
    const bool refuse = argc == 2 && std::strcmp(argv[1], "refuse") == 0;
    if (!preserve) assert(access(path, F_OK) != 0);
    if (argc == 2 && std::strcmp(argv[1], "no-app") == 0) return 0;
    {
        QCoreApplication app(argc, argv);
        if (!preserve) assert(access(path, F_OK) != 0); // No synchronous receipt.
        QTimer::singleShot(20, &app, &QCoreApplication::quit);
        assert(app.exec() == 0);
        if (refuse) { assert(access(path, F_OK) != 0); return 0; }
        std::ifstream input(path);
        std::string result((std::istreambuf_iterator<char>(input)), {});
        if (preserve) { assert(result == "keep\n"); return 0; }
        assert(result == "{\"nonce\":\"" QT_PROBE_NONCE "\",\"application_thread\":true}\n");
    }
    assert(unlink(path) == 0);
    {
        QCoreApplication second(argc, argv);
        QTimer::singleShot(20, &second, &QCoreApplication::quit);
        assert(second.exec() == 0);
        assert(access(path, F_OK) != 0); // No second callback after app recreation.
    }
}
