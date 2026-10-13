/* Separately selected R2D: fixed bytes only, no pointer or count traversal. */
#define QT_OBSERVER_OUTPUT_LIMIT 1024
#define QT_OBSERVER_REMOTE_LIMIT 26
#define QML_OBSERVER_NO_MAIN
#include "qml_singleton_observer.c"

typedef struct { bool guard_present,descriptor_present; unsigned char guard,descriptor[12]; } DescriptorSample;
static const char *observe_descriptor(Observer *o,const Manifest *m,uint32_t bias,DescriptorSample s[2],bool *equal) {
    uint64_t guard=(uint64_t)bias+m->value[GUARD_VA],list=(uint64_t)bias+m->value[REGISTRY_VA];
    if (guard>UINT32_MAX || list>UINT32_MAX) { o->incomplete=true;return "bounds"; }
    for (unsigned i=0;i<2;i++) {
        if (!read_exact_bytes(o,(uint32_t)guard,&s[i].guard,1)) return "partial-guard";
        s[i].guard_present=true;
        if (s[i].guard!=0xff) { o->incomplete=true;return "guard"; }
        if (!read_exact(o,(uint32_t)list,s[i].descriptor,12)) return "partial-descriptor";
        s[i].descriptor_present=true;
    }
    *equal=s[0].guard==s[1].guard && !memcmp(s[0].descriptor,s[1].descriptor,12);
    if (!*equal) { o->incomplete=true;return "changed-descriptor"; }
    return "descriptor-sample-only";
}
static void descriptor_output(Observer *o,const char *phase,const DescriptorSample s[2],bool equal) {
    emit(o,"{\"kind\":\"sampled-untrusted-descriptor-only\",\"phase\":\"%s\",\"samples\":[",phase);
    for (unsigned i=0;i<2;i++) {
        emit(o,"%s{\"guard\":",i?",":"");
        if (s[i].guard_present) emit(o,"%u",s[i].guard);else emit(o,"null");
        if (s[i].descriptor_present) {
            uint32_t count=u32(s[i].descriptor+8);
            emit(o,",\"allocation_data_word\":\"%08" PRIx32 "\",\"buffer_word\":\"%08" PRIx32
                "\",\"count_unsigned\":%" PRIu32 ",\"count_signed\":%" PRId64 "}",
                u32(s[i].descriptor),u32(s[i].descriptor+4),count,
                count>INT32_MAX?(int64_t)count-INT64_C(4294967296):(int64_t)count);
        } else emit(o,",\"allocation_data_word\":null,\"buffer_word\":null,\"count_unsigned\":null,\"count_signed\":null}");
    }
    emit(o,"],\"descriptor_equal\":");
    if (s[0].descriptor_present && s[1].descriptor_present) emit(o,"%s",equal?"true":"false");else emit(o,"null");
    emit(o,",\"incomplete\":%s,\"atomic\":false,\"remote_bytes\":%zu}\n",o->incomplete||o->fatal?"true":"false",o->remote_bytes);
}
#ifndef QML_DESCRIPTOR_NO_MAIN
int main(int argc,char **argv) {
    /* Mark the included original discovery implementation used without running it. */
    (void)discover;
    uint64_t pid_value,start;Manifest manifest={0};
    if (argc!=4 || !number(argv[1],10,&pid_value) || !pid_value || pid_value>INT32_MAX ||
        !number(argv[2],10,&start) || !start || !load_manifest(argv[3],&manifest)) return 2;
    pid_t pid=(pid_t)pid_value;Observer *o=calloc(1,sizeof *o);
    char *maps=malloc(MAP_CAP+1),*again=malloc(MAP_CAP+1);
    if (!o || !maps || !again) {free(o);free(maps);free(again);return 2;}
    o->read=remote_read;o->context=&pid;o->deadline=monotonic_ms()+2000;
    char path[64],exe_path[64];struct stat exe={0},provider={0},exe_again,provider_again;
    size_t bytes=0,bytes_again=0;uint32_t bias=0;
    snprintf(path,sizeof path,"/proc/%ld/maps",(long)pid);snprintf(exe_path,sizeof exe_path,"/proc/%ld/exe",(long)pid);
    bool ready=budget(o) && identity(pid,start) && !stat(exe_path,&exe) && !stat(manifest.provider,&provider) && bounded_file(path,maps,MAP_CAP,&bytes);
    if (ready) {memcpy(again,maps,bytes+1);ready=parse_maps(o,again);}
    ready=ready && provider_bias(o,&manifest,&provider,&bias) &&
        image_constant(o,manifest.value[META_EXPECTED],&exe,false) && image_constant(o,manifest.value[VPTR_EXPECTED],&exe,false) &&
        image_constant(o,manifest.value[MANAGER_EXPECTED],&exe,true) && image_constant(o,manifest.value[INVOKER_EXPECTED],&exe,true);
    DescriptorSample samples[2]={0};bool equal=false;const char *phase="preflight";
    if (ready) phase=observe_descriptor(o,&manifest,bias,samples,&equal);else o->fatal=true;
    if (!identity(pid,start) || stat(exe_path,&exe_again) || stat(manifest.provider,&provider_again) ||
        exe.st_ino!=exe_again.st_ino || exe.st_dev!=exe_again.st_dev || provider.st_ino!=provider_again.st_ino || provider.st_dev!=provider_again.st_dev ||
        !bounded_file(path,again,MAP_CAP,&bytes_again) || bytes!=bytes_again || memcmp(maps,again,bytes_again) || !budget(o)) {
        o->fatal=true;phase="process-or-mapping-change";
    }
    descriptor_output(o,phase,samples,equal);
    int result=o->fatal?3:(o->incomplete?4:0);
    if (o->output_bytes>=OUTPUT_CAP || fwrite(o->output,1,o->output_bytes,stdout)!=o->output_bytes) result=3;
    free(o);free(maps);free(again);return result;
}
#endif
