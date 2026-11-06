// Host-only build preflight. Never loads an adapter or supplies target facts.
#include "profile.h"
#include <cstdio>
int main() {
    const auto *profile=remarkable::rm1_observation::detail::reviewedProfile();
    if(!profile||!remarkable::rm1_observation::detail::complete(*profile)) {
        std::fputs("unsupported: no reviewed RM1 observation profile; required SDK/provider/binding evidence is incomplete\n",stderr);
        return 78;
    }
    std::printf("%s\n%s\n",profile->manifestSha256,profile->qtVersion);
}
