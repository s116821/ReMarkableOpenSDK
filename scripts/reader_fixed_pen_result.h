#pragma once
// Original-byte research consumer. Caller owns file/process/clock admission.
#pragma push_macro("emit")
#undef emit
#include "reader_fixed_pen_stream.h"
#pragma pop_macro("emit")
#include <QByteArray>
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QtEndian>
#include <atomic>
#include <cmath>
#include <limits>

namespace reader_fixed_pen_result {
struct Expected {
    QString nonce,boot,candidatePid,candidateStart,holderPid,holderStart;
    quint64 setupEnd=0,requestMs=0;
};
// These are observations from the independent outer actor, not flags copied
// from the holder JSON. A production caller must authenticate/pin their source.
struct Completion {
    bool exited=false,statusKnown=false,rootPinned=false,noPending=false,candidateLive=false;
    int exitCode=-1;
    QString holderPid,holderStart;
    quint64 observedMs=0,nowMs=0;
    QByteArray originalStdout;
};
struct Claim {std::atomic<bool> consumed{false};};
struct Result {
    bool claimed=false,accepted=false;QString refusal;
    Expected binding;
    quint64 beganMs=0,strokeEndMs=0,recoveryEndMs=0,finishedMs=0,completedAtMs=0;
    QByteArray resultSha256,traceSha256;
    reader_fixed_pen_stream::Report replay;
    static constexpr bool nativeAuthority=false,renderAuthority=false,durableAuthority=false,atomicAuthority=false;
};
namespace detail {
inline bool keys(const QJsonObject &o,const char *names){
    const auto expected=QString::fromLatin1(names).split(' ',Qt::SkipEmptyParts);
    if(o.size()!=expected.size())return false;
    for(const auto &name:expected)if(!o.contains(name))return false;
    return true;
}
inline bool truth(const QJsonObject &o,const char *name,bool expected){const auto v=o.value(QString::fromLatin1(name));return v.isBool()&&v.toBool()==expected;}
inline bool integer(const QJsonObject &o,const char *name,int minimum,int maximum,int *output=nullptr){
    const auto v=o.value(QString::fromLatin1(name));
    if(!v.isDouble())return false;
    const double n=v.toDouble();
    if(!std::isfinite(n)||std::floor(n)!=n||n<minimum||n>maximum)return false;
    if(output)*output=int(n);
    return true;
}
inline bool decimal(const QString &s,quint64 *out=nullptr,bool nonzero=true){
    if(s.isEmpty()||s.size()>20||(s.size()>1&&s[0]=='0'))return false;
    for(const QChar ch:s)if(ch<'0'||ch>'9')return false;
    bool ok=false;const auto value=s.toULongLong(&ok);
    if(!ok||(nonzero&&!value))return false;
    if(out)*out=value;
    return true;
}
inline bool clock(const QJsonObject &o,const char *name,quint64 &out){const auto v=o.value(QString::fromLatin1(name));return v.isString()&&decimal(v.toString(),&out);}
inline bool pid(const QString &s){quint64 n=0;return decimal(s,&n)&&n<=2147483647;}
inline bool equal(const QJsonObject &o,const char *name,const QString &value){const auto v=o.value(QString::fromLatin1(name));return v.isString()&&v.toString()==value;}
inline bool snapshot(const QJsonObject &o,reader_fixed_pen_stream::Snapshot &s){
    if(!keys(o,"x y pressure distance tilt_x tilt_y pen touch rubber stylus stylus2"))return false;
    if(!integer(o,"x",0,20966,&s.x)||!integer(o,"y",0,15725,&s.y)||!integer(o,"pressure",0,4095,&s.pressure)||
       !integer(o,"distance",0,255,&s.distance)||!integer(o,"tilt_x",-9000,9000,&s.tiltX)||!integer(o,"tilt_y",-9000,9000,&s.tiltY))return false;
    bool *values[]={&s.pen,&s.touch,&s.rubber,&s.stylus,&s.stylus2};
    const char *names[]={"pen","touch","rubber","stylus","stylus2"};
    for(int i=0;i<5;++i){const auto v=o.value(QString::fromLatin1(names[i]));if(!v.isBool())return false;*values[i]=v.toBool();}
    return true;
}
inline bool streamMatches(const QJsonObject &s,const reader_fixed_pen_stream::Report &r){
    return keys(s,"valid finalized partial_frame raw_contact_possible complete_contact final_released start_endpoint_observed end_endpoint_observed stroke_observed records frames contact_frames contact_intervals refusal")&&
        truth(s,"valid",r.valid)&&truth(s,"finalized",r.finalized)&&truth(s,"partial_frame",r.partialFrame)&&
        truth(s,"raw_contact_possible",r.rawContactPossible)&&truth(s,"complete_contact",r.completeContact)&&truth(s,"final_released",r.finalReleased)&&
        truth(s,"start_endpoint_observed",r.startEndpointObserved)&&truth(s,"end_endpoint_observed",r.endEndpointObserved)&&truth(s,"stroke_observed",r.strokeObserved)&&
        integer(s,"records",int(r.records),int(r.records))&&integer(s,"frames",int(r.frames),int(r.frames))&&
        integer(s,"contact_frames",int(r.contactFrames),int(r.contactFrames))&&integer(s,"contact_intervals",int(r.contactIntervals),int(r.contactIntervals))&&
        integer(s,"refusal",int(r.refusal),int(r.refusal));
}
}
inline Result consume(const QByteArray &bytes,const QByteArray &trace,const Expected &e,const Completion &c,Claim &claim){
    Result r;
    if(claim.consumed.exchange(true)){r.refusal=QStringLiteral("claim-spent");return r;}r.claimed=true;
    const auto no=[&](const char *s){r.refusal=QString::fromLatin1(s);return r;};
    static const QRegularExpression nonce(QStringLiteral("\\A[0-9a-f]{32}\\z"));
    static const QRegularExpression boot(QStringLiteral("\\A[0-9a-f]{8}(-[0-9a-f]{4}){3}-[0-9a-f]{12}\\z"));
    if(e.nonce.size()!=32||e.boot.size()!=36||!nonce.match(e.nonce).hasMatch()||!boot.match(e.boot).hasMatch()||!detail::pid(e.candidatePid)||!detail::decimal(e.candidateStart)||
       !detail::pid(e.holderPid)||!detail::decimal(e.holderStart)||e.holderPid==e.candidatePid||!e.requestMs||e.setupEnd<=e.requestMs)return no("expected");
    if(bytes.isEmpty()||bytes.size()>4096||bytes!=c.originalStdout)return no("original-bytes");
    if(!c.exited||!c.statusKnown||c.exitCode||!c.rootPinned||!c.noPending||!c.candidateLive||c.holderPid!=e.holderPid||c.holderStart!=e.holderStart||
       c.observedMs<e.requestMs)return no("actor-completion");
    QJsonParseError error;const auto doc=QJsonDocument::fromJson(bytes,&error);
    if(error.error!=QJsonParseError::NoError||!doc.isObject()||doc.toJson(QJsonDocument::Compact)+'\n'!=bytes)return no("canonical-json");
    const auto o=doc.object();
    if(!detail::keys(o,"schema nonce candidate_pid candidate_start boot_id holder_pid holder_start setup_end_ms request_ms began_ms stroke_end_ms recovery_end_ms finished_ms child_pid child_start child_started child_gone child_status_known child_exit child_signal child_result_known dispatch stream initial_snapshot snapshot trace recovery checks accepted refusal native_authority render_authority durable_authority atomic_authority"))return no("schema-keys");
    if(!detail::equal(o,"schema",QStringLiteral("fixed-pen-holder/1"))||!detail::equal(o,"nonce",e.nonce)||!detail::equal(o,"boot_id",e.boot)||
       !detail::equal(o,"candidate_pid",e.candidatePid)||!detail::equal(o,"candidate_start",e.candidateStart)||
       !detail::equal(o,"holder_pid",e.holderPid)||!detail::equal(o,"holder_start",e.holderStart))return no("binding");
    quint64 setup=0,request=0,began=0,stroke=0,recovery=0,finished=0;
    if(!detail::clock(o,"setup_end_ms",setup)||!detail::clock(o,"request_ms",request)||!detail::clock(o,"began_ms",began)||
       !detail::clock(o,"stroke_end_ms",stroke)||!detail::clock(o,"recovery_end_ms",recovery)||!detail::clock(o,"finished_ms",finished)||
       setup!=e.setupEnd||request!=e.requestMs||began<request||began-request>250||began>std::numeric_limits<quint64>::max()-3000||
       stroke!=began+1000||recovery!=began+3000||recovery>setup||finished<began||finished>=recovery||
       c.observedMs<finished||c.nowMs<c.observedMs||c.nowMs>=setup)return no("original-clocks");
    if(!o.value("child_pid").isString()||!detail::pid(o.value("child_pid").toString())||o.value("child_pid").toString()==e.holderPid||
       o.value("child_pid").toString()==e.candidatePid||!o.value("child_start").isString()||!detail::decimal(o.value("child_start").toString()))return no("child-binding");
    for(const char *name:{"child_started","child_gone","child_status_known","child_result_known","accepted"})if(!detail::truth(o,name,true))return no("positive-facts");
    for(const char *name:{"native_authority","render_authority","durable_authority","atomic_authority"})if(!detail::truth(o,name,false))return no("authority");
    if(!detail::integer(o,"child_exit",0,0)||!detail::integer(o,"child_signal",0,0)||!o.value("refusal").isString()||!o.value("refusal").toString().isEmpty())return no("child-or-refusal");
    const auto dispatch=o.value("dispatch").toObject();
    if(!detail::keys(dispatch,"complete attempted contact_possible unknown calls frames accepted_records refusal")||!detail::truth(dispatch,"complete",true)||
       !detail::truth(dispatch,"attempted",true)||!detail::truth(dispatch,"contact_possible",true)||!detail::truth(dispatch,"unknown",false)||
       !detail::integer(dispatch,"calls",56,56)||!detail::integer(dispatch,"frames",56,56)||!detail::integer(dispatch,"accepted_records",178,178)||!detail::integer(dispatch,"refusal",0,0))return no("dispatch");
    const auto release=o.value("recovery").toObject();
    if(!detail::keys(release,"armed attempted written observed unknown refusal"))return no("recovery-keys");
    for(const char *name:{"armed","attempted","written","observed"})if(!detail::truth(release,name,true))return no("recovery");
    if(!detail::truth(release,"unknown",false)||!detail::integer(release,"refusal",0,0))return no("recovery");
    const auto checks=o.value("checks").toObject();
    if(!detail::keys(checks,"go_claimed initial_snapshots_match initial_queue_empty quiet20ms final_snapshot_match descriptor_identity device_fds_closed ipc_fds_closed trace_closed context_live"))return no("check-keys");
    for(const auto &v:checks)if(!v.isBool()||!v.toBool())return no("check-failed");
    reader_fixed_pen_stream::Snapshot initial,final;
    if(!detail::snapshot(o.value("initial_snapshot").toObject(),initial)||!detail::snapshot(o.value("snapshot").toObject(),final)||
       !reader_fixed_pen_stream::released(initial)||initial.tiltX||initial.tiltY||!reader_fixed_pen_stream::released(final)||
       final.x!=11056||final.y!=7616||final.distance!=100||final.tiltX||final.tiltY)return no("snapshots");
    const auto raw=o.value("trace").toObject();
    if(!detail::keys(raw,"bytes sha256 complete")||trace.isEmpty()||trace.size()>131072||trace.size()%16||
       !detail::integer(raw,"bytes",int(trace.size()),int(trace.size()))||!detail::truth(raw,"complete",true))return no("trace-shape");
    const auto traceHash=QCryptographicHash::hash(trace,QCryptographicHash::Sha256).toHex();
    if(!detail::equal(raw,"sha256",QString::fromLatin1(traceHash)))return no("trace-hash");
    reader_fixed_pen_stream::Stream replay(initial);
    for(qsizetype n=0;n<trace.size();n+=16){
        const auto p=reinterpret_cast<const uchar *>(trace.constData()+n);
        if(!replay.accept({qFromLittleEndian<quint16>(p+8),qFromLittleEndian<quint16>(p+10),qFromLittleEndian<qint32>(p+12)}))return no("trace-replay");
    }
    if(!replay.releasedMatches(final))return no("trace-final-snapshot");
    r.replay=replay.report();
    if(!r.replay.strokeObserved||!detail::streamMatches(o.value("stream").toObject(),r.replay))return no("reported-stream");
    r.resultSha256=QCryptographicHash::hash(bytes,QCryptographicHash::Sha256).toHex();r.traceSha256=traceHash;
    r.binding=e;r.beganMs=began;r.strokeEndMs=stroke;r.recoveryEndMs=recovery;r.finishedMs=finished;r.completedAtMs=c.observedMs;
    r.accepted=true;return r;
}
} // namespace reader_fixed_pen_result
