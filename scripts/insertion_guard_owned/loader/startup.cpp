// A distinct object deliberately linked after archive selection.
#include <cstdio>
extern "C" void owned_install_from_constructor();
__attribute__((constructor)) static void owned_payload_startup() {
    std::fprintf(stderr,"DSO_CONSTRUCTOR_ENTER\n");
    owned_install_from_constructor();
    std::fprintf(stderr,"DSO_CONSTRUCTOR_LEAVE\n");
}
