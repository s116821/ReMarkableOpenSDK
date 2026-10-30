#include "reader_insertion_child_environment.h"
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <dlfcn.h>
#include <link.h>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;
namespace {
int checks = 0;
void check(bool yes, const char *name) {
    if (!yes) { std::fprintf(stderr, "FAIL %s\n", name); std::exit(1); }
    ++checks;
}
bool absent() { return !std::getenv("LD_PRELOAD") && !std::getenv("LD_BIND_NOW"); }
struct Image { const char *name; void *base; unsigned matches = 0; };
int image(struct dl_phdr_info *info, std::size_t, void *data) {
    auto &expected = *static_cast<Image *>(data);
    if (std::strcmp(info->dlpi_name, expected.name) == 0 && reinterpret_cast<void *>(info->dlpi_addr) == expected.base)
        ++expected.matches;
    return 0;
}
bool mapped(const char *name, void *base) {
    Image expected{name, base}; ::dl_iterate_phdr(image, &expected); return expected.matches == 1;
}
int child(const char *path, bool custom, const char *preload, std::string &captured) {
    // Prepare the explicit environment before fork. Reintroduction is a negative
    // control, not an environment the clearing helper claims to constrain.
    std::string library = std::string("LD_PRELOAD=") + preload;
    std::string owner = "OWNED_EXPECTED_PARENT_PID=" + std::to_string(::getpid());
    char bind[] = "LD_BIND_NOW=1";
    char *customEnvironment[] = {library.data(), owner.data(), bind, nullptr};
    char *args[] = {const_cast<char *>(path), nullptr};
    int pipe[2]; check(::pipe(pipe) == 0, "capture pipe");
    const pid_t pid = ::fork(); check(pid >= 0, "real fork");
    if (pid == 0) {
        ::close(pipe[0]);
        if (::dup2(pipe[1], STDOUT_FILENO) < 0 || ::dup2(pipe[1], STDERR_FILENO) < 0) std::_Exit(93);
        ::close(pipe[1]);
        ::execve(path, args, custom ? customEnvironment : environ);
        std::_Exit(94);
    }
    check(::close(pipe[1]) == 0, "parent writer close");
    char bytes[256];
    for (;;) {
        const ssize_t count = ::read(pipe[0], bytes, sizeof(bytes));
        if (count < 0 && errno == EINTR) continue;
        check(count >= 0, "capture read");
        if (count == 0) break;
        check(captured.size() + std::size_t(count) <= 4096, "capture bounded");
        captured.append(bytes, std::size_t(count));
    }
    check(::close(pipe[0]) == 0, "capture close");
    int status = 0; pid_t waited;
    do { waited = ::waitpid(pid, &status, 0); } while (waited < 0 && errno == EINTR);
    check(waited == pid && WIFEXITED(status), "owned child normal exit status");
    return WEXITSTATUS(status);
}
}
int main(int argc, char **argv) {
    check(argc == 4, "fixed mode child library arguments");
    const char *mode = argv[1], *childPath = argv[2], *libraryPath = argv[3];
    const bool bare = std::strcmp(mode, "bare-controls") == 0;
    const bool inherited = std::strcmp(mode, "inherit") == 0;
    const bool custom = std::strcmp(mode, "custom-env") == 0;
    const bool constructor = std::strcmp(mode, "constructor-clear") == 0;
    check(bare || inherited || custom || constructor || !std::strcmp(mode, "parent-clear") || !std::strcmp(mode, "already-clear"), "known mode");
    void *const original = ::dlsym(RTLD_DEFAULT, "owned_injection_cookie");
    using Cookie = std::uint64_t (*)() noexcept;
    using Cleared = int (*)() noexcept;
    const Cookie cookie = reinterpret_cast<Cookie>(original);
    Dl_info location{};
    if (bare) {
        check(!original && absent(), "bare process has no preload or injection environment");
        for (unsigned mask = 0; mask != 4; ++mask) {
            if (mask & 1) check(::setenv("LD_PRELOAD", "owned-not-loaded", 1) == 0, "set current process preload");
            if (mask & 2) check(::setenv("LD_BIND_NOW", "1", 1) == 0, "set current process bind variable");
            check(reader_child_environment::clearInheritedInjection() && absent(), "clear all current environment combinations");
            check(!::dlsym(RTLD_DEFAULT, "owned_injection_cookie"), "environment setting alone did not load DSO");
        }
    } else {
        check(cookie && cookie() == UINT64_C(0x5e1ec7edc001d00d), "parent constructor qualification and callable cookie");
        check(::dladdr(original, &location) && !std::strcmp(location.dli_fname, libraryPath) && mapped(libraryPath, location.dli_fbase), "parent exact DSO mapping before clear");
        const auto cleared = reinterpret_cast<Cleared>(::dlsym(RTLD_DEFAULT, "owned_injection_cleared_in_constructor"));
        check(cleared && cleared() == (constructor ? 1 : 0), "constructor clear ordering recorded");
        if (constructor) check(absent(), "constructor consumed qualification then cleared before parent main");
        else check(std::getenv("LD_PRELOAD") && std::getenv("LD_BIND_NOW"), "original injection environment present in parent main");
        if (!inherited) {
            check(reader_child_environment::clearInheritedInjection() && absent(), "current parent clears both variables");
            if (!std::strcmp(mode, "already-clear")) check(reader_child_environment::clearInheritedInjection() && absent(), "clearing already absent variables is idempotent");
        }
        check(::dlsym(RTLD_DEFAULT, "owned_injection_cookie") == original && cookie() == UINT64_C(0x5e1ec7edc001d00d) &&
              mapped(libraryPath, location.dli_fbase), "parent loaded DSO remains callable after environment change");
    }
    std::string output;
    const int code = child(childPath, custom, libraryPath, output);
    const bool denied = inherited || custom;
    check(code == (denied ? 119 : 0), "real exec child outcome");
    check(output == (denied ? "OWNED_PRELOAD_CHILD_DENIED\n" : "CHILD_MAIN_OK env_absent=1 preload_absent=1\n"),
          "child constructor denial precedes main or clean main runs without preload");
    if (bare) check(absent() && !::dlsym(RTLD_DEFAULT, "owned_injection_cookie"), "bare parent remains unaffected");
    else {
        check(::dlsym(RTLD_DEFAULT, "owned_injection_cookie") == original && cookie() == UINT64_C(0x5e1ec7edc001d00d) &&
              mapped(libraryPath, location.dli_fbase), "parent original DSO remains resident and callable after child exec");
        check(inherited ? !absent() : absent(), "child explicit environment did not modify parent environment");
    }
    std::printf("CHILD_CAPTURE_BEGIN\n%sCHILD_CAPTURE_END\n", output.c_str());
    std::printf("PASS mode=%s checks=%d child_exit=%d parent_resident=%d\n", mode, checks, code, bare ? 0 : 1);
    return 0;
}
