// Original owned ARM fixture. No proprietary implementation or device input.
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <thread>
#include <unistd.h>

struct Contact { int32_t id, x, y, major, pressure; uint8_t state; uint8_t pad[3]; };
struct Handler {
    uint8_t prefix[0x40]; int32_t slot; uint8_t gap[0x14];
    int32_t xmin, xmax, ymin, ymax; uint8_t gap2[0x28];
    double matrix[9]; uint16_t type; uint8_t gap3[6]; double output[2];
};
static_assert(offsetof(Contact, state) == 20);
static_assert(offsetof(Handler, xmin) == 0x58);
static_assert(offsetof(Handler, matrix) == 0x90);
static_assert(offsetof(Handler, type) == 0xd8);
static_assert(offsetof(Handler, output) == 0xe0);
extern "C" void fixture_current_caller(Handler*, Contact*);
extern "C" void fixture_other_caller(Handler*, Contact*);
static bool missing_post;
static unsigned type_calls;

extern "C" int fixture_fractions(Handler* h, Contact* c, double* out) {
    out[0] = double(c->x - h->xmin) / double(h->xmax - h->xmin);
    out[1] = double(c->y - h->ymin) / double(h->ymax - h->ymin);
    return h->type;
}
extern "C" int fixture_matrix_type(double* matrix) { ++type_calls; return matrix[0] != 1.0; }
extern "C" void fixture_transform(Handler* h, double* p) {
    if (missing_post) _exit(0);
    const double x = p[0], y = p[1];
    const double w = h->matrix[2]*x + h->matrix[5]*y + h->matrix[8];
    p[0] = (h->matrix[0]*x + h->matrix[3]*y + h->matrix[6]) / w;
    p[1] = (h->matrix[1]*x + h->matrix[4]*y + h->matrix[7]) / w;
    h->type = 1; // Cache mutation is permitted, not a same-call mismatch.
}

int main(int argc, char** argv) {
    if (argc != 3) return 2;
    const char* mode = argv[1];
    if (std::strcmp(mode,"identity") && std::strcmp(mode,"rotate") &&
        std::strcmp(mode,"nonfinite") && std::strcmp(mode,"wrong-first") &&
        std::strcmp(mode,"wrong-caller") && std::strcmp(mode,"no-call") &&
        std::strcmp(mode,"missing-post")) return 2;
    Handler h{};
    h.xmax = 1403; h.ymax = 1871;
    h.matrix[0] = h.matrix[4] = h.matrix[8] = 1.0;
    Contact c{1,164,1396,8,100,1,{0,0,0}};
    if (!std::strcmp(mode,"rotate") || !std::strcmp(mode,"missing-post")) {
        const double rotation[9] = {0,1,0,-1,0,0,1,0,1};
        std::memcpy(h.matrix, rotation, sizeof rotation); h.type = 1;
    }
    if (!std::strcmp(mode,"nonfinite")) h.xmax = h.xmin;
    if (!std::strcmp(mode,"wrong-first")) c.id = 2;
    missing_post = !std::strcmp(mode,"missing-post");
    std::atomic<bool> done{false};
    std::thread worker([&]{ while (!done.load()) std::this_thread::sleep_for(std::chrono::milliseconds(2)); });
    // Main can release an attached dummy using one private file, never touch input.
    const auto start = std::chrono::steady_clock::now();
    while (std::strcmp(argv[2],"-") && access(argv[2],F_OK)) {
        if (std::chrono::steady_clock::now()-start > std::chrono::seconds(30)) {
            done = true; worker.join(); return 3;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    if (std::strcmp(mode,"no-call")) {
        if (!std::strcmp(mode,"wrong-caller")) fixture_other_caller(&h,&c);
        else fixture_current_caller(&h,&c);
        if (!std::strcmp(mode,"wrong-first")) { c.id=1; fixture_current_caller(&h,&c); }
    }
    done = true; worker.join();
    const unsigned expected_calls = h.type ? 1 : 0;
    if (type_calls != expected_calls) return 4; // PRE branch flags survived debugger.
    std::puts("owned fixture finished; no native authority");
    return 0;
}
