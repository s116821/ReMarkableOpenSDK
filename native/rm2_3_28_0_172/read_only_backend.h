#pragma once
#include "../resident/read_only_dispatch.h"
#include "reader_insertion_guard_native_profile.h"

namespace remarkable::rm2_3_28_0_172 {
using resident::observationReply;
// Preserve the exact RM2 Facilities gate; shared dispatch confers no admission.
class ReadOnlyBackend final:public QObject {
    const reader_insertion_native_profile::Facilities *profile_;
    resident::ReadOnlyDispatch dispatch_;
public:
    explicit ReadOnlyBackend(const reader_insertion_native_profile::Facilities *profile,QObject *parent=nullptr)
        :QObject(parent),profile_(profile),dispatch_(this) {}
    bool attach(resident::Endpoint *endpoint) { return dispatch_.attach(endpoint); }
    void submit(resident::Endpoint::Lease lease) {
        const auto *profile=profile_;
        dispatch_.submit(std::move(lease),[profile]{
            return profile&&profile->core&&profile->applicationAbsent&&profile->baseAdmission.profileAndProviders;
        });
    }
};
}
