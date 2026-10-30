// Executed by the real host kernel. Never emulates an ARM exec/binfmt path.
#include <cstdio>
#include <cstdlib>
#include <dlfcn.h>

int main() {
    if (std::getenv("LD_PRELOAD") || std::getenv("LD_BIND_NOW") ||
        ::dlsym(RTLD_DEFAULT, "owned_injection_cookie")) return 92;
    std::puts("CHILD_MAIN_OK env_absent=1 preload_absent=1");
    return 0;
}
