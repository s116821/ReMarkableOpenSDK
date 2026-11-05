#pragma once
#include "admission.h"
#include "../resident/read_only_dispatch.h"

namespace remarkable::rm1_observation {
class ReadOnlyBackend final:public QObject {
    const Admission *admission_;
    resident::ReadOnlyDispatch dispatch_;
public:
    explicit ReadOnlyBackend(const Admission *admission,QObject *parent=nullptr)
        :QObject(parent),admission_(admission),dispatch_(this) {}
    bool attach(resident::Endpoint *endpoint) {
        return admission_&&admission_->current()&&dispatch_.attach(endpoint);
    }
    void submit(resident::Endpoint::Lease lease) {
        const auto *admission=admission_;
        const std::function<bool()> current=[admission]{return admission&&admission->current();};
        dispatch_.submit(std::move(lease),current,current);
    }
};
}
