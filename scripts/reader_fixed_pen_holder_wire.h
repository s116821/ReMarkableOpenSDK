#pragma once
// Parent-only serialization. Never call Qt or this header's helpers in child.
#include "reader_fixed_pen_holder.h"
#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace reader_fixed_pen_holder {
struct Binding {
    QByteArray nonce,boot;std::uint64_t candidatePid=0,candidateStart=0,holderPid=0,holderStart=0;
    std::uint64_t setupEnd=0,requestMs=0,beganMs=0;
    bool goClaimed=false;
};
struct Evidence {
    std::uint64_t traceBytes=0;QByteArray traceSha256;
    bool traceComplete=false,quiet=false,snapshotMatch=false,descriptorIdentity=false;
};
inline QString decimal(std::uint64_t n){return QString::number(qulonglong(n));}
inline QJsonObject snapshotWire(const Snapshot&k) {
    return {{"x",k.x},{"y",k.y},{"pressure",k.pressure},{"distance",k.distance},{"tilt_x",k.tiltX},{"tilt_y",k.tiltY},
        {"pen",k.pen},{"touch",k.touch},{"rubber",k.rubber},{"stylus",k.stylus},{"stylus2",k.stylus2}};
}
inline QJsonObject wireBinding(const Binding&b,const Result&r) {
    return {{"nonce",QString::fromLatin1(b.nonce)},{"candidate_pid",decimal(b.candidatePid)},{"candidate_start",decimal(b.candidateStart)},
        {"boot_id",QString::fromLatin1(b.boot)},{"holder_pid",decimal(b.holderPid)},{"holder_start",decimal(b.holderStart)},
        {"setup_end_ms",decimal(b.setupEnd)},{"request_ms",decimal(b.requestMs)},{"began_ms",decimal(b.beganMs)},
        {"stroke_end_ms",decimal(b.beganMs?b.beganMs+1000:0)},{"recovery_end_ms",decimal(b.beganMs?b.beganMs+3000:0)},
        {"child_pid",decimal(r.childPid>0?std::uint64_t(r.childPid):0)},{"child_start",decimal(r.childStart)}};
}
inline QByteArray normalWire(const Binding&b,const Result&r,const Evidence&e) {
    if(r.diagnostic)return {};
    auto o=wireBinding(b,r);const auto&d=r.dispatch;const auto&s=r.stream;const auto&k=r.snapshot;const auto&v=r.recovery;
    o.insert("schema",QStringLiteral("fixed-pen-holder/1"));o.insert("finished_ms",decimal(r.finishedMs));
    o.insert("child_started",r.childStarted);o.insert("child_gone",r.childGone);o.insert("child_status_known",r.childStatusKnown);
    o.insert("child_exit",r.childExit);o.insert("child_signal",r.childSignal);o.insert("child_result_known",r.childResultKnown);
    o.insert("dispatch",QJsonObject{{"complete",d.complete},{"attempted",d.attempted},{"contact_possible",d.contactPossible},{"unknown",d.unknown},
        {"calls",int(d.calls)},{"frames",int(d.frames)},{"accepted_records",int(d.acceptedRecords)},{"refusal",int(d.refusal)}});
    o.insert("stream",QJsonObject{{"valid",s.valid},{"finalized",s.finalized},{"partial_frame",s.partialFrame},{"raw_contact_possible",s.rawContactPossible},
        {"complete_contact",s.completeContact},{"final_released",s.finalReleased},{"start_endpoint_observed",s.startEndpointObserved},
        {"end_endpoint_observed",s.endEndpointObserved},{"stroke_observed",s.strokeObserved},{"records",int(s.records)},{"frames",int(s.frames)},
        {"contact_frames",int(s.contactFrames)},{"contact_intervals",int(s.contactIntervals)},{"refusal",int(s.refusal)}});
    o.insert("initial_snapshot",snapshotWire(r.initialSnapshot));o.insert("snapshot",snapshotWire(k));
    o.insert("trace",QJsonObject{{"bytes",int(e.traceBytes)},{"sha256",QString::fromLatin1(e.traceSha256)},{"complete",e.traceComplete}});
    o.insert("recovery",QJsonObject{{"armed",r.armed},{"attempted",v.attempted},{"written",v.written},{"observed",v.observed},{"unknown",v.unknown},{"refusal",int(v.refusal)}});
    o.insert("checks",QJsonObject{{"go_claimed",b.goClaimed},{"initial_snapshots_match",r.initialSnapshotsMatch},{"initial_queue_empty",r.initialQueueEmpty},
        {"quiet20ms",e.quiet},{"final_snapshot_match",e.snapshotMatch},{"descriptor_identity",e.descriptorIdentity},
        {"device_fds_closed",r.deviceFdsClosed},{"ipc_fds_closed",r.ipcFdsClosed},{"trace_closed",r.traceClosed},{"context_live",r.contextLive}});
    o.insert("accepted",r.accepted);o.insert("refusal",QString::fromLatin1(r.refusal));
    for(const char*n:{"native_authority","render_authority","durable_authority","atomic_authority"})o.insert(n,false);
    auto bytes=QJsonDocument(o).toJson(QJsonDocument::Compact)+'\n';return bytes.size()<=4096?bytes:QByteArray{};
}
inline QByteArray diagnosticWire(const Binding&b,const Result&r) {
    if(!r.diagnostic||r.childGone)return {};
    auto o=wireBinding(b,r);o.insert("schema",QStringLiteral("fixed-pen-holder-diagnostic/1"));o.insert("at_ms",decimal(r.finishedMs));
    o.insert("child_gone",false);o.insert("release_observed",false);o.insert("unknown",true);o.insert("refusal",QString::fromLatin1(r.refusal));
    for(const char*n:{"native_authority","render_authority","durable_authority","atomic_authority"})o.insert(n,false);
    auto bytes=QJsonDocument(o).toJson(QJsonDocument::Compact)+'\n';return bytes.size()<=4096?bytes:QByteArray{};
}
} // namespace reader_fixed_pen_holder
