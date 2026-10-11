#include "qt_creation_deadline.h"
#include <cassert>
#include <cstdio>
using qml_access::CreationDeadline;
int main() {
    qint64 now = 1000;
    CreationDeadline setup([&] { return now; });
    assert(setup.startSetup(40000) && setup.setupDeadlineMs() == 41000);
    assert(!setup.startSetup(40000)); // No readiness event renews the origin.
    now = 40999; assert(setup.open());
    now = 41000; assert(!setup.open() && !setup.accept(5000));

    now = 1000;
    CreationDeadline access([&] { return now; });
    assert(access.startSetup(40000));
    now = 40999; assert(access.accept(5000));
    now = 41000; assert(access.open()); // Accepted access can cross setup expiry.
    assert(!access.accept(5000)); // No duplicate arm/LibraryReady renewal.
    now = 45998; assert(access.open() && access.remainingMs() == 1);
    now = 45999; assert(!access.open());

    now = 1000;
    CreationDeadline suspend([&] { return now; });
    assert(suspend.startSetup(40000) && suspend.accept(5000));
    now = 7000; assert(!suspend.open()); // Boot clock advances while Qt time can pause.

    now = -1;
    CreationDeadline failure([&] { return now; });
    assert(!failure.startSetup(40000) && !failure.open());
    now = 1000; assert(!failure.startSetup(40000) && !failure.open());
    CreationDeadline laterFailure([&] { return now; });
    assert(laterFailure.startSetup(40000));
    now = -1; assert(!laterFailure.open() && !laterFailure.accept(5000));
    now = 1000;
    CreationDeadline acceptedFailure([&] { return now; });
    assert(acceptedFailure.startSetup(40000) && acceptedFailure.accept(5000));
    now = -1; assert(!acceptedFailure.open());
    now = std::numeric_limits<qint64>::max();
    CreationDeadline overflow([&] { return now; });
    assert(!overflow.startSetup(40000));
    puts("creation boot-clock setup/access boundaries, suspend, failure and no-renewal PASS");
}
