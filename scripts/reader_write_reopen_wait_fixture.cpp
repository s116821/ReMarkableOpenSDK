#include "reader_write_reopen_wait.h"
#include <cstdio>
#include <stdexcept>
namespace {unsigned checks=0;void check(bool value,const char *why){++checks;if(!value)throw std::runtime_error(why);}}
int main(){try{
    using reader_write_reopen::StartupWait;
    StartupWait wait;
    wait.note(QStringLiteral("holder-not-ready"));wait.beginComposition();
    wait.note(QStringLiteral("composition/owned-refusal"));
    wait.beginComposition();wait.beginComposition();
    wait.note(QStringLiteral("composition/canceled"));
    const auto stopped=wait.terminal("startup-admission-or-cap");
    check(stopped.contains(QStringLiteral("|last_wait=composition/canceled")),"terminal cancellation retained");
    check(stopped.contains(QStringLiteral("|previous_wait=composition/owned-refusal")),"pending inspections preserve preceding refusal");
    wait.note(QStringLiteral("composition/canceled"));wait.note(QStringLiteral("composition/canceled"));
    check(wait.terminal("startup-admission-or-cap")==stopped,"repeated cancellation does not replace prior distinct reason");
    StartupWait early;early.beginComposition();early.note(QStringLiteral("composition/canceled"));
    check(early.terminal("stop").contains(QStringLiteral("|previous_wait=composition-not-sampled")),"no completed sample remains explicit");
    StartupWait repeated;repeated.note(QStringLiteral("holder-not-ready"));repeated.note(QStringLiteral("holder-not-ready"));
    check(repeated.terminal("stop").endsWith(QStringLiteral("|previous_wait=not-polled")),"repeated pending is not a new distinct phase");
    StartupWait bounded;bounded.note(QString(1000,QChar('a')));bounded.note(QString(1000,QChar('b')));
    const auto text=bounded.terminal("stop");
    check(text.size()<=352&&text.contains(QString(160,QChar('a')))&&text.contains(QString(160,QChar('b'))),"both reasons bounded independently");
    std::printf("PASS startup wait cancellation regression cases=4 checks=%u native_execution=false\n",checks);return 0;
}catch(const std::exception&e){std::fprintf(stderr,"FAIL wait diagnostic %s\n",e.what());return 1;}}

