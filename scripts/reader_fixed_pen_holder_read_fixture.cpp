// Original frozen FIFO/child producer bytes; external Completion is an explicit model.
#include "reader_fixed_pen_result.h"
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <cstdio>
#include <stdexcept>

namespace {
unsigned cases=0,checks=0;
void require(bool yes,const char *why){++checks;if(!yes)throw std::runtime_error(why);}
QByteArray read(const QDir &dir,const QString &name,qint64 cap){
    QFile file(dir.filePath(name));require(file.open(QIODevice::ReadOnly),"frozen original asset opens");
    require(file.size()>=0&&file.size()<=cap,"frozen asset bounded");
    const auto data=file.read(cap+1);require(data.size()==file.size()&&file.atEnd(),"frozen complete original bytes");return data;
}
QString text(const QJsonObject &o,const char *name){
    const auto v=o.value(QString::fromLatin1(name));require(v.isString(),"independent expected string type");return v.toString();
}
quint64 number(const QJsonObject &o,const char *name){
    quint64 value=0;require(reader_fixed_pen_result::detail::decimal(text(o,name),&value),"independent expected canonical decimal");return value;
}
QJsonObject object(const QByteArray &bytes){
    QJsonParseError error;const auto doc=QJsonDocument::fromJson(bytes,&error);
    require(error.error==QJsonParseError::NoError&&doc.isObject()&&doc.toJson(QJsonDocument::Compact)+'\n'==bytes,"frozen expectation manifest canonical original bytes");return doc.object();
}
void one(const QDir &dir,const QJsonObject &entry){
    using namespace reader_fixed_pen_result;
    ++cases;
    const auto name=text(entry,"case");require(name=="positive"||name=="canceled"||name=="unfenced-diagnostic","fixed owned case only");
    const auto bytes=read(dir,name+".json",4096),trace=read(dir,name+".raw",131072),factsBytes=read(dir,name+".expected.json",4096);
    require(QString::fromLatin1(QCryptographicHash::hash(bytes,QCryptographicHash::Sha256).toHex())==text(entry,"scalar_sha256"),"untouched original scalar hash");
    require(QString::fromLatin1(QCryptographicHash::hash(trace,QCryptographicHash::Sha256).toHex())==text(entry,"trace_sha256"),"untouched original raw trace hash");
    require(QString::fromLatin1(QCryptographicHash::hash(factsBytes,QCryptographicHash::Sha256).toHex())==text(entry,"expectation_sha256"),"separately frozen expectation hash");
    const auto facts=object(factsBytes);
    require(text(facts,"schema")=="owned-holder-consumer-expectation/1"&&text(facts,"case")==name&&text(facts,"candidate_identity_model")=="live-owned-parent","expectation provenance is owned UI-context model");
    require(facts.value("external_completion_modeled").isBool()&&facts.value("external_completion_modeled").toBool(),"no native actor admission claim");
    require(facts.value("expected_accepted").isBool()&&facts.value("expected_accepted").toBool()==(name=="positive"),"expected classification comes from frozen case manifest");
    Expected expected{text(facts,"nonce"),text(facts,"boot_id"),text(facts,"candidate_pid"),text(facts,"candidate_start"),text(facts,"holder_pid"),text(facts,"holder_start"),number(facts,"setup_end_ms"),number(facts,"request_ms")};
    require(expected.candidatePid!=expected.holderPid,"actual owned parent and holder identities distinct");
    // These are trusted owned test inputs, not observations authenticated by this reader.
    Completion completion{true,true,true,true,true,0,expected.holderPid,expected.holderStart,number(facts,"completion_observed_ms"),number(facts,"completion_now_ms"),bytes};
    Claim claim;const auto result=consume(bytes,trace,expected,completion,claim);
    if(result.accepted!=(name=="positive"))std::fprintf(stderr,"case=%s refusal=%s\n",qPrintable(name),qPrintable(result.refusal));
    require(result.claimed&&result.accepted==(name=="positive"),"original child/FIFO producer through strict consumer");
    require(!Result::nativeAuthority&&!Result::renderAuthority&&!Result::durableAuthority&&!Result::atomicAuthority,"no authority promotion");
    if(name=="positive")require(result.replay.strokeObserved&&result.replay.finalized&&result.replay.finalReleased&&result.replay.contactIntervals==1,"actual FIFO trace independently replays");
    const auto again=consume(bytes,trace,expected,completion,claim);require(!again.claimed&&!again.accepted&&again.refusal=="claim-spent","original-byte admission cannot retry");
    std::printf("case=%s accepted=%d scalar=%lld trace=%lld bytes_unchanged=true completion_modeled=true\n",qPrintable(name),int(result.accepted),static_cast<long long>(bytes.size()),static_cast<long long>(trace.size()));
}
}
int main(int argc,char **argv){try{
    require(argc==2,"one frozen owned asset directory");const QDir dir(QString::fromLocal8Bit(argv[1]));
    const auto manifest=object(read(dir,"manifest.json",16384));require(text(manifest,"schema")=="owned-holder-original-byte-read/1","frozen independent asset manifest");
    require(manifest.value("external_admission_modeled").isBool()&&manifest.value("external_admission_modeled").toBool(),"manifest explicit admission model");
    const auto list=manifest.value("cases").toArray();require(list.size()==3,"exactly three original cases");
    QStringList seen;for(const auto &v:list){require(v.isObject(),"manifest case object");const auto entry=v.toObject();const auto name=text(entry,"case");require(!seen.contains(name),"no repeated case");seen.append(name);one(dir,entry);}
    require(seen.contains("positive")&&seen.contains("canceled")&&seen.contains("unfenced-diagnostic"),"complete original positive/refusal/diagnostic set");
    std::printf("PASS holder original-byte read cases=%u checks=%u rewritten_bytes=false external_admission_modeled=true native_execution=false\n",cases,checks);return 0;
}catch(const std::exception &e){std::fprintf(stderr,"FAIL holder original-byte read %s cases=%u checks=%u\n",e.what(),cases,checks);return 1;}}
