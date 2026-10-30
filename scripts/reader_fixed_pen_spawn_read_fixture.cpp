// Read untouched output of the spawned owned FIFO writer through the production consumer.
// Independent completion and UI identity remain explicit fixture models.
#define main reader_fixed_pen_original_read_main
#include "reader_fixed_pen_holder_read_fixture.cpp"
#undef main
int main(int argc,char **argv){try{
    require(argc==2,"one original owned spawn asset folder");
    const QDir dir(QString::fromLocal8Bit(argv[1]));
    const auto manifest=object(read(dir,"manifest.json",16384));
    require(text(manifest,"schema")=="owned-spawn-original-byte-read/1","closed owned spawn manifest schema");
    require(manifest.value("external_admission_modeled").isBool()&&manifest.value("external_admission_modeled").toBool(),"completion model explicit");
    const auto bytes=read(dir,"spawn-positive.json",4096),trace=read(dir,"spawn-positive.raw",131072),factsBytes=read(dir,"spawn-positive.expected.json",4096);
    for(const auto &pair:{qMakePair("scalar_sha256",bytes),qMakePair("trace_sha256",trace),qMakePair("expectation_sha256",factsBytes)})
        require(QString::fromLatin1(QCryptographicHash::hash(pair.second,QCryptographicHash::Sha256).toHex())==text(manifest,pair.first),"original frozen hash match");
    const auto facts=object(factsBytes);
    require(text(facts,"schema")=="owned-holder-consumer-expectation/1"&&text(facts,"case")=="spawn-positive"&&text(facts,"candidate_identity_model")=="live-owned-parent","separately prepared binding model");
    require(facts.value("external_completion_modeled").isBool()&&facts.value("external_completion_modeled").toBool()&&facts.value("expected_accepted").isBool()&&facts.value("expected_accepted").toBool(),"owned expected positive");
    using namespace reader_fixed_pen_result;
    Expected expected{text(facts,"nonce"),text(facts,"boot_id"),text(facts,"candidate_pid"),text(facts,"candidate_start"),text(facts,"holder_pid"),text(facts,"holder_start"),number(facts,"setup_end_ms"),number(facts,"request_ms")};
    Completion completion{true,true,true,true,true,0,expected.holderPid,expected.holderStart,number(facts,"completion_observed_ms"),number(facts,"completion_now_ms"),bytes};
    Claim claim;const auto result=consume(bytes,trace,expected,completion,claim);
    require(result.claimed&&result.accepted&&result.replay.strokeObserved&&result.replay.finalized&&result.replay.finalReleased&&result.replay.contactIntervals==1,"actual spawned owned original bytes replay and qualify");
    require(!Result::nativeAuthority&&!Result::renderAuthority&&!Result::durableAuthority&&!Result::atomicAuthority,"owned trace is not device ink authority");
    const auto again=consume(bytes,trace,expected,completion,claim);
    require(!again.claimed&&!again.accepted&&again.refusal=="claim-spent","same original result cannot be spent twice");
    std::printf("PASS spawned owned original-byte consumer cases=1 checks=%u rewritten_bytes=false completion_modeled=true\n",checks);return 0;
}catch(const std::exception&e){std::fprintf(stderr,"FAIL spawned original-byte consumer %s\n",e.what());return 1;}}

