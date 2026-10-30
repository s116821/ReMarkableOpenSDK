// Original producer bytes, synthetic ARM records, modeled outer actor observations.
#include <QJsonDocument> // Deliberately exercise Qt-first include order.
#include "reader_fixed_pen_holder_wire.h"
#include "reader_fixed_pen_result.h"
#include <cstdio>
#include <functional>
#include <stdexcept>

namespace {
using namespace reader_fixed_pen_result;
unsigned cases=0,checks=0;
void require(bool b,const char *s){++checks;if(!b)throw std::runtime_error(s);}
template<class F>void scenario(F f){++cases;f();}
struct Fixture {
    Expected expected{QString(32,'a'),"11111111-2222-3333-4444-555555555555","123","1000","124","1001",20000,10000};
    Completion completion{true,true,true,true,true,0,"124","1001",10500,10600,{}};
    QByteArray bytes,trace;
    Fixture(){
        reader_fixed_pen_holder::Binding binding{expected.nonce.toLatin1(),expected.boot.toLatin1(),123,1000,124,1001,20000,10000,10010,true};
        reader_fixed_pen_holder::Result result;
        result.accepted=result.childStarted=result.childGone=result.childStatusKnown=result.childResultKnown=true;
        result.armed=result.initialSnapshotsMatch=result.initialQueueEmpty=result.contextLive=true;
        result.deviceFdsClosed=result.ipcFdsClosed=result.traceClosed=true;
        result.childPid=125;result.childStart=1002;result.finishedMs=10400;result.childExit=0;result.childSignal=0;result.refusal="";
        result.dispatch.complete=result.dispatch.attempted=result.dispatch.contactPossible=true;
        result.dispatch.calls=result.dispatch.frames=56;result.dispatch.acceptedRecords=178;
        result.recovery.attempted=result.recovery.written=result.recovery.observed=true;
        reader_fixed_pen_stream::Snapshot state;
        result.initialSnapshot=state;reader_fixed_pen_stream::Stream stream(state);
        for(const auto &frame:reader_fixed_pen::strokeFrames())for(std::size_t i=0;i<frame.count;++i){
            const auto event=frame.events[i];char record[16]{};
            qToLittleEndian<quint16>(event.type,reinterpret_cast<uchar *>(record+8));
            qToLittleEndian<quint16>(event.code,reinterpret_cast<uchar *>(record+10));
            qToLittleEndian<qint32>(event.value,reinterpret_cast<uchar *>(record+12));trace.append(record,16);
            require(stream.accept(event),"original fixed records replay");
        }
        state=stream.report().state;require(stream.releasedMatches(state),"synthetic final snapshot");
        result.snapshot=state;result.stream=stream.report();
        reader_fixed_pen_holder::Evidence evidence{quint64(trace.size()),QCryptographicHash::hash(trace,QCryptographicHash::Sha256).toHex(),true,true,true,true};
        bytes=reader_fixed_pen_holder::normalWire(binding,result,evidence);completion.originalStdout=bytes;
        require(!bytes.isEmpty()&&bytes.size()<=4096,"actual producer bound");
    }
    Result run(Claim &claim){return consume(bytes,trace,expected,completion,claim);}
    Result run(){Claim claim;return run(claim);}
    void object(const std::function<void(QJsonObject&)> &change){auto o=QJsonDocument::fromJson(bytes).object();change(o);bytes=QJsonDocument(o).toJson(QJsonDocument::Compact)+'\n';completion.originalStdout=bytes;}
    void reportTrace(){
        reader_fixed_pen_stream::Snapshot initial;
        reader_fixed_pen_stream::Stream replay(initial);
        for(qsizetype n=0;n<trace.size();n+=16){auto p=reinterpret_cast<const uchar *>(trace.constData()+n);require(replay.accept({qFromLittleEndian<quint16>(p+8),qFromLittleEndian<quint16>(p+10),qFromLittleEndian<qint32>(p+12)}),"changed positive trace replay");}
        require(replay.releasedMatches(replay.report().state),"changed trace final match");
        reader_fixed_pen_holder::Result modeled;modeled.stream=replay.report();
        const auto stream=QJsonDocument::fromJson(reader_fixed_pen_holder::normalWire({},modeled,{})).object().value("stream");
        object([&](QJsonObject &o){o.insert("stream",stream);auto t=o.value("trace").toObject();t.insert("bytes",trace.size());t.insert("sha256",QString::fromLatin1(QCryptographicHash::hash(trace,QCryptographicHash::Sha256).toHex()));o.insert("trace",t);});
    }
};
}
int main(){try{
    scenario([]{Fixture f;Claim claim;const auto r=f.run(claim);if(!r.accepted)std::fprintf(stderr,"baseline refusal %s\n",qPrintable(r.refusal));require(r.claimed&&r.accepted&&r.refusal.isEmpty()&&r.replay.strokeObserved,"actual producer original bytes accepted with independent completion");require(r.replay.records==178,"independent raw16 framing replay");const auto again=f.run(claim);require(!again.accepted&&!again.claimed&&again.refusal=="claim-spent","whole consumer admission spent");});
    const char *flat[]={"nonce","candidate_pid","candidate_start","boot_id","holder_pid","holder_start","setup_end_ms","request_ms","began_ms","stroke_end_ms","recovery_end_ms","finished_ms","child_pid","child_start"};
    for(const auto field:flat)scenario([=]{Fixture f;f.object([=](QJsonObject&o){o.insert(field,"0");});require(!f.run().accepted,"wrong or noncanonical binding/clock");});
    for(const char *field:{"child_started","child_gone","child_status_known","child_result_known","accepted"})scenario([=]{Fixture f;f.object([=](QJsonObject&o){o.insert(field,false);});require(!f.run().accepted,"missing positive lifecycle fact");});
    for(const char *field:{"native_authority","render_authority","durable_authority","atomic_authority"})scenario([=]{Fixture f;f.object([=](QJsonObject&o){o.insert(field,true);});require(!f.run().accepted,"authority remains false");});
    for(int mode=0;mode<13;++mode)scenario([=]{Fixture f;
        switch(mode){
        case 0:f.completion.exited=false;break;case 1:f.completion.statusKnown=false;break;case 2:f.completion.exitCode=1;break;
        case 3:f.completion.rootPinned=false;break;case 4:f.completion.noPending=false;break;case 5:f.completion.candidateLive=false;break;
        case 6:f.completion.holderPid="125";break;case 7:f.completion.holderStart="0";break;case 8:f.completion.observedMs=10399;break;
        case 9:f.completion.nowMs=10499;break;case 10:f.completion.nowMs=20000;break;case 11:f.completion.originalStdout+=' ';break;
        default:f.completion.observedMs=0;}
        require(!f.run().accepted,"outer completion independently required");
    });
    for(int mode=0;mode<8;++mode)scenario([=]{Fixture f;f.object([=](QJsonObject&o){
        switch(mode){case 0:o.insert("extra",true);break;case 1:o.remove("stream");break;case 2:o.insert("schema","fixed-pen-holder-diagnostic/1");break;
        case 3:o.insert("child_exit",0.5);break;case 4:o.insert("child_exit","0");break;case 5:o.insert("accepted",1);break;
        case 6:o.insert("refusal","unknown");break;default:o.insert("candidate_pid",123);}
    });require(!f.run().accepted,"closed schema and strict types");});
    for(int mode=0;mode<4;++mode)scenario([=]{Fixture f;
        if(mode==0)f.bytes.insert(1,"\"accepted\":true,");
        if(mode==1)f.bytes.prepend(' ');
        if(mode==2)f.bytes.chop(1);
        if(mode==3)f.bytes=QByteArray(4097,' ');
        f.completion.originalStdout=f.bytes;require(!f.run().accepted,"duplicate/whitespace/noncanonical/overlong original refuses");
    });
    const char *nested[]={"dispatch","stream","snapshot","initial_snapshot","trace","recovery","checks"};
    for(const char *field:nested)scenario([=]{Fixture f;f.object([=](QJsonObject&o){auto inner=o.value(field).toObject();inner.insert("unexpected",true);o.insert(field,inner);});require(!f.run().accepted,"all nested schemas closed");});
    for(const char *field:{"complete","attempted","contact_possible","unknown","calls","frames","accepted_records","refusal"})scenario([=]{Fixture f;f.object([=](QJsonObject&o){auto inner=o.value("dispatch").toObject();const auto old=inner.value(field);inner.insert(field,old.isBool()?QJsonValue(!old.toBool()):QJsonValue(old.toInt()+1));o.insert("dispatch",inner);});require(!f.run().accepted,"dispatch conjunction");});
    for(const char *field:{"armed","attempted","written","observed","unknown","refusal"})scenario([=]{Fixture f;f.object([=](QJsonObject&o){auto inner=o.value("recovery").toObject();const auto old=inner.value(field);inner.insert(field,old.isBool()?QJsonValue(!old.toBool()):QJsonValue(1));o.insert("recovery",inner);});require(!f.run().accepted,"release conjunction");});
    for(const char *field:{"go_claimed","initial_snapshots_match","initial_queue_empty","quiet20ms","final_snapshot_match","descriptor_identity","device_fds_closed","ipc_fds_closed","trace_closed","context_live"})scenario([=]{Fixture f;f.object([=](QJsonObject&o){auto inner=o.value("checks").toObject();inner.insert(field,false);o.insert("checks",inner);});require(!f.run().accepted,"every closed resource/source check required");});
    for(int mode=0;mode<7;++mode)scenario([=]{Fixture f;
        if(mode==0)f.trace[12]=char(1);
        if(mode==1)f.trace.chop(1);
        if(mode==2)f.trace.clear();
        if(mode==3)f.trace=QByteArray(131088,'\0');
        if(mode==4)f.object([](QJsonObject&o){auto t=o.value("trace").toObject();t.insert("complete",false);o.insert("trace",t);});
        if(mode==5)f.object([](QJsonObject&o){auto s=o.value("stream").toObject();s.insert("records",177);o.insert("stream",s);});
        if(mode==6){f.trace[8]=char(99);f.object([&](QJsonObject&o){auto t=o.value("trace").toObject();t.insert("sha256",QString::fromLatin1(QCryptographicHash::hash(f.trace,QCryptographicHash::Sha256).toHex()));o.insert("trace",t);});}
        require(!f.run().accepted,"trace bytes or independent replay mismatch");
    });
    scenario([]{Fixture f;Claim claim;f.completion.noPending=false;require(!f.run(claim).accepted,"first incomplete observation refuses");f.completion.noPending=true;require(!f.run(claim).accepted,"later completion does not renew admission");});
    scenario([]{Fixture f;QByteArray filtered,frame;int x=0,y=0,pen=0,touch=0,pressure=0,distance=100;
        for(qsizetype n=0;n<f.trace.size();n+=16){const auto p=reinterpret_cast<const uchar *>(f.trace.constData()+n);const auto type=qFromLittleEndian<quint16>(p+8),code=qFromLittleEndian<quint16>(p+10);const auto value=qFromLittleEndian<qint32>(p+12);
            if(type==0){if(!frame.isEmpty()){filtered+=frame;filtered+=f.trace.mid(n,16);frame.clear();}continue;}
            int *state=nullptr;if(type==3){if(code==0)state=&x;else if(code==1)state=&y;else if(code==24)state=&pressure;else if(code==25)state=&distance;}else if(type==1){if(code==320)state=&pen;else if(code==330)state=&touch;}
            require(state,"known original record for filtering");if(*state!=value){frame+=f.trace.mid(n,16);*state=value;}
        }
        require(filtered.size()<f.trace.size(),"filter genuinely removes unchanged records");f.trace=filtered;f.reportTrace();require(f.run().accepted,"filtered observer trace accepted without178-record echo");
    });
    for(int mode=0;mode<7;++mode)scenario([=]{Fixture f;f.object([=](QJsonObject &o){
        if(mode==0)o.insert("child_pid","2147483648");
        if(mode==1)o.insert("child_pid","0125");
        if(mode==2)o.insert("began_ms","10251");
        if(mode==3){auto s=o.value("initial_snapshot").toObject();s.insert("pressure",1);o.insert("initial_snapshot",s);}
        if(mode==4){auto s=o.value("snapshot").toObject();s.insert("x",11138);o.insert("snapshot",s);}
        if(mode==5)o.insert("finished_ms","13010");
        if(mode==6)o.insert("setup_end_ms","18446744073709551616");
    });require(!f.run().accepted,"PID, snapshot, handoff age and absolute deadline bounds");});
    scenario([]{Fixture f;f.object([](QJsonObject &o){o.insert("child_pid","125\n");});const auto r=f.run();std::printf("trailing_lf_child_accepted=%d\n",int(r.accepted));require(!r.accepted,"canonical decimal excludes terminal LF");});
    for(const QString &suffix:{QStringLiteral("\n"),QStringLiteral("\r\n"),QStringLiteral(" "),QStringLiteral("\t"),QString(QChar(0x0661))}){
        for(const char *field:{"child_pid","child_start","began_ms","finished_ms"})scenario([=]{Fixture f;f.object([=](QJsonObject &o){o.insert(field,o.value(field).toString()+suffix);});require(!f.run().accepted,"entire decimal excludes whitespace/nonASCII");});
        for(int field=0;field<4;++field)scenario([=]{Fixture f;
            if(field==0){f.expected.nonce+=suffix;f.object([&](QJsonObject &o){o.insert("nonce",f.expected.nonce);});}
            if(field==1){f.expected.boot+=suffix;f.object([&](QJsonObject &o){o.insert("boot_id",f.expected.boot);});}
            if(field==2){f.expected.candidateStart+=suffix;f.object([&](QJsonObject &o){o.insert("candidate_start",f.expected.candidateStart);});}
            if(field==3){f.expected.holderStart+=suffix;f.completion.holderStart=f.expected.holderStart;f.object([&](QJsonObject &o){o.insert("holder_start",f.expected.holderStart);});}
            require(!f.run().accepted,"Expected bindings also require whole canonical strings");
        });
    }
    std::printf("PASS fixed pen result cases=%u checks=%u actual_producer=true raw_trace_modeled=true actor_observation_modeled=true native_execution=false\n",cases,checks);return 0;
}catch(const std::exception &e){std::fprintf(stderr,"FAIL fixed pen result %s cases=%u checks=%u\n",e.what(),cases,checks);return 1;}}
