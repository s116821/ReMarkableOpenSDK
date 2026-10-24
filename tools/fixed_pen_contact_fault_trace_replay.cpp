// Pure replay of retained 16-byte trace records through the actual source policy.
// All snapshots supplied by the caller; this grants no native/device authority.
#include "reader_fixed_pen_stream.h"
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <climits>
#include <cstdint>
struct Record {std::uint32_t seconds,micros;std::uint16_t type,code;std::int32_t value;};
static_assert(sizeof(Record)==16);
bool integer(const char*s,int&v){char*end=nullptr;errno=0;const long n=std::strtol(s,&end,10);if(errno||!end||!*s||*end||n<INT_MIN||n>INT_MAX)return false;v=int(n);return true;}
bool snapshot(char**args,reader_fixed_pen_stream::Snapshot&s){
 int a[11]{};for(unsigned i=0;i<11;++i)if(!integer(args[i],a[i])||(i>=6&&a[i]!=0&&a[i]!=1))return false;
 s.x=a[0];s.y=a[1];s.pressure=a[2];s.distance=a[3];s.tiltX=a[4];s.tiltY=a[5];s.pen=a[6];s.touch=a[7];s.rubber=a[8];s.stylus=a[9];s.stylus2=a[10];return s.valid();
}
int main(int argc,char**argv){
 if(argc!=24)return 2;
 reader_fixed_pen_stream::Snapshot initial{},final{};if(!snapshot(argv+2,initial)||!snapshot(argv+13,final))return 2;
 FILE*f=std::fopen(argv[1],"rb");if(!f)return 2;
 reader_fixed_pen_stream::Stream stream(initial);Record record{};unsigned n=0;bool okay=true;
 while(true){const auto bytes=std::fread(&record,1,sizeof record,f);if(!bytes)break;if(bytes!=sizeof record||++n>8192||!stream.accept({record.type,record.code,record.value})){okay=false;break;}}
 const bool readError=std::ferror(f)!=0;const bool closed=std::fclose(f)==0;if(readError||!closed)okay=false;
 if(!okay||!stream.releasedMatches(final))return 1;
 const auto r=stream.report();
 std::printf("{\"valid\":%s,\"finalized\":%s,\"partial_frame\":%s,\"raw_contact_possible\":%s,\"complete_contact\":%s,\"final_released\":%s,\"start_endpoint_observed\":%s,\"end_endpoint_observed\":%s,\"stroke_observed\":%s,\"records\":%u,\"frames\":%u,\"contact_frames\":%u,\"contact_intervals\":%u,\"refusal\":%u}\n",r.valid?"true":"false",r.finalized?"true":"false",r.partialFrame?"true":"false",r.rawContactPossible?"true":"false",r.completeContact?"true":"false",r.finalReleased?"true":"false",r.startEndpointObserved?"true":"false",r.endEndpointObserved?"true":"false",r.strokeObserved?"true":"false",r.records,r.frames,r.contactFrames,r.contactIntervals,unsigned(r.refusal));
 return r.valid&&r.finalized&&r.finalReleased&&!r.strokeObserved?0:1;
}
