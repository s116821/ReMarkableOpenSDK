/* Fixed-shape, read-only existing-instance discovery; private manifest required. */
#define QT_OBSERVER_OUTPUT_LIMIT 8192
#define QT_OBSERVER_NO_TOPOLOGY
#define QT_OBSERVER_NO_MAIN
#include "qt_topology_observer.c"

enum { TYPE_CAP=1024, MATCH_CAP=4, TAPE_CAP=16 };
enum { LOAD_VA, FILE_OFFSET, FILE_BYTES, MEMORY_BYTES, REGISTRY_VA, GUARD_VA,
       KIND_OFFSET, META_OFFSET, EXTRA_OFFSET, SINGLETON_KIND, META_EXPECTED,
       INFO_OFFSET, CALLBACK_OFFSET, CALLBACK_BYTES, CAPTURE_OFFSET, MANAGER_OFFSET,
       INVOKER_OFFSET, MANAGER_EXPECTED, INVOKER_EXPECTED, CONTROL_OFFSET, OBJECT_OFFSET,
       STRONG_OFFSET, VPTR_EXPECTED, DPTR_OFFSET, BACKLINK_OFFSET, MANIFEST_FIELDS };
typedef struct { char provider[512]; uint32_t value[MANIFEST_FIELDS]; } Manifest;
typedef struct { uint32_t address; size_t bytes; unsigned char data[64]; } Sample;
typedef struct { Sample fields[TAPE_CAP]; size_t count; uint32_t type,object,control,capture,dptr,strong; } Candidate;
typedef struct { uint32_t pointer,kind,meta; } TypeSample;

static bool manifest_valid(const Manifest *m) {
    const uint32_t *v=m->value;
    if (!m->provider[0] || m->provider[0]!='/' || !v[FILE_BYTES] ||
        v[MEMORY_BYTES]<v[FILE_BYTES] || (uint64_t)v[LOAD_VA]+v[MEMORY_BYTES]>UINT32_MAX ||
        v[LOAD_VA]%4096!=v[FILE_OFFSET]%4096 || v[REGISTRY_VA]%4 ||
        v[REGISTRY_VA]<v[LOAD_VA] || (uint64_t)v[REGISTRY_VA]+12>(uint64_t)v[LOAD_VA]+v[MEMORY_BYTES] ||
        v[GUARD_VA]<v[LOAD_VA] || (uint64_t)v[GUARD_VA]+1>(uint64_t)v[LOAD_VA]+v[MEMORY_BYTES] ||
        !v[CALLBACK_BYTES] || v[CALLBACK_BYTES]>64 || v[CALLBACK_BYTES]%4) return false;
    const unsigned offsets[]={KIND_OFFSET,META_OFFSET,EXTRA_OFFSET,INFO_OFFSET,CALLBACK_OFFSET,
        CAPTURE_OFFSET,MANAGER_OFFSET,INVOKER_OFFSET,CONTROL_OFFSET,OBJECT_OFFSET,
        STRONG_OFFSET,DPTR_OFFSET,BACKLINK_OFFSET};
    for (size_t i=0;i<sizeof offsets/sizeof offsets[0];i++)
        if (v[offsets[i]]>128 || v[offsets[i]]%4) return false;
    const unsigned callback[]={CAPTURE_OFFSET,MANAGER_OFFSET,INVOKER_OFFSET};
    for (unsigned i=0;i<3;i++) {
        if (v[callback[i]]+4>v[CALLBACK_BYTES]) return false;
        for (unsigned j=0;j<i;j++) if (v[callback[i]]==v[callback[j]]) return false;
    }
    if (v[CONTROL_OFFSET]==v[OBJECT_OFFSET] || !v[DPTR_OFFSET] ||
        v[KIND_OFFSET]==v[META_OFFSET] || v[KIND_OFFSET]==v[EXTRA_OFFSET] ||
        v[META_OFFSET]==v[EXTRA_OFFSET]) return false;
    const unsigned constants[]={META_EXPECTED,MANAGER_EXPECTED,INVOKER_EXPECTED,VPTR_EXPECTED};
    for (unsigned i=0;i<4;i++) if (!v[constants[i]] || v[constants[i]]%4) return false;
    return true;
}
static bool load_manifest(const char *path, Manifest *m) {
    char text[4097]; size_t bytes;
    if (!bounded_file(path,text,4096,&bytes)) return false;
    char *save=NULL,*magic=strtok_r(text,"\n",&save),*provider=strtok_r(NULL,"\n",&save);
    if (!magic || strcmp(magic,"qml-existing-instance-arm32-v1") || !provider ||
        strlen(provider)>=sizeof m->provider) return false;
    strcpy(m->provider,provider);
    char *remaining=save,*tokens=NULL,*token=strtok_r(remaining," \r\n\t",&tokens);
    for (unsigned i=0;i<MANIFEST_FIELDS;i++) {
        uint64_t value;
        if (!token || !number(token,16,&value) || value>UINT32_MAX) return false;
        m->value[i]=(uint32_t)value; token=strtok_r(NULL," \r\n\t",&tokens);
    }
    return !token && manifest_valid(m);
}
static bool field_address(uint32_t base, uint32_t offset, uint32_t *address) {
    uint64_t sum=(uint64_t)base+offset;
    if (!base || base%4 || sum>UINT32_MAX) return false;
    *address=(uint32_t)sum; return true;
}
static bool word_at(Observer *o, uint32_t base, uint32_t offset, uint32_t *value) {
    uint32_t address; unsigned char bytes[4];
    if (!field_address(base,offset,&address) || !read_exact(o,address,bytes,4)) {
        o->incomplete=true; return false;
    }
    *value=u32(bytes); return true;
}
static bool sample(Observer *o, Candidate *c, uint32_t base, uint32_t offset, size_t bytes, Sample **out) {
    uint32_t address;
    if (c->count==TAPE_CAP || bytes>64 || !field_address(base,offset,&address)) {
        o->incomplete=true; return false;
    }
    Sample *s=&c->fields[c->count]; s->address=address; s->bytes=bytes;
    if (!read_exact(o,address,s->data,bytes)) return false;
    c->count++; *out=s; return true;
}
static bool sampled_word(Observer *o, Candidate *c, uint32_t base, uint32_t offset, uint32_t *out) {
    Sample *s; if (!sample(o,c,base,offset,4,&s)) return false;
    *out=u32(s->data); return true;
}
static bool same_file(const Mapping *map, const struct stat *file) {
    return map && map->inode==(uint64_t)file->st_ino && map->dev_major==major(file->st_dev)
        && map->dev_minor==minor(file->st_dev);
}
static bool image_constant(Observer *o, uint32_t value, const struct stat *exe, bool code) {
    const Mapping *map=range(o,value,4);
    return same_file(map,exe) && !map->write && (!code || map->execute);
}
static bool readable_extent(Observer *o, uint64_t start, uint64_t end) {
    if (!start || start>=end || end>UINT64_C(0x100000000)) return false;
    while (start<end) {
        uint64_t next=start;
        for (size_t i=0;i<o->map_count;i++)
            if (o->maps[i].read && start>=o->maps[i].lo && start<o->maps[i].hi) {
                next=o->maps[i].hi<end?o->maps[i].hi:end; break;
            }
        if (next==start) return false;
        start=next;
    }
    return true;
}
static bool provider_bias(Observer *o, const Manifest *m, const struct stat *provider, uint32_t *bias) {
    size_t matches=0; const uint32_t *v=m->value;
    uint32_t virtual_page=v[LOAD_VA]&~UINT32_C(4095), offset_page=v[FILE_OFFSET]&~UINT32_C(4095);
    for (size_t i=0;i<o->map_count;i++) {
        const Mapping *map=&o->maps[i];
        if (map->read && same_file(map,provider) && map->offset==offset_page) {
            if (map->lo<virtual_page || map->lo-virtual_page>UINT32_MAX) return false;
            *bias=(uint32_t)(map->lo-virtual_page); matches++;
        }
    }
    if (matches!=1) return false;
    uint64_t file_end=((uint64_t)v[FILE_OFFSET]+v[FILE_BYTES]+4095)&~UINT64_C(4095);
    for (size_t i=0;i<o->map_count;i++) {
        const Mapping *map=&o->maps[i];
        if (same_file(map,provider) && map->offset>=offset_page && map->offset<file_end) {
            uint64_t expected=(uint64_t)*bias+virtual_page+map->offset-offset_page;
            if (!map->read || map->lo!=expected) return false;
        }
    }
    return readable_extent(o,(uint64_t)*bias+v[LOAD_VA],
        (uint64_t)*bias+v[LOAD_VA]+v[MEMORY_BYTES]);
}
static const char *discover(Observer *o, const Manifest *m, uint32_t bias, Candidate candidates[MATCH_CAP], size_t *matches, size_t *type_count) {
    const uint32_t *v=m->value; uint64_t guard64=(uint64_t)bias+v[GUARD_VA], list64=(uint64_t)bias+v[REGISTRY_VA];
    if (guard64>UINT32_MAX || list64>UINT32_MAX) { o->incomplete=true; return "bounds"; }
    uint32_t guard=(uint32_t)guard64,list=(uint32_t)list64;
    const Mapping *guard_map=range_bytes(o,guard,1), *list_map=range(o,list,12);
    if (!guard_map || !list_map) { o->incomplete=true; return "registry-mapping"; }
    unsigned char initialized,descriptor[12],descriptor_again[12],array[TYPE_CAP*4],array_again[TYPE_CAP*4];
    if (!read_exact_bytes(o,guard,&initialized,1) || initialized!=0xff) { o->incomplete=true; return "guard"; }
    if (!read_exact(o,list,descriptor,12)) return "registry";
    uint32_t count=u32(descriptor+8),buffer=u32(descriptor+4);
    if (count>TYPE_CAP) { o->incomplete=true; return "type-cap"; }
    *type_count=count;
    if (count && !read_exact(o,buffer,array,(size_t)count*4)) return "type-array";
    TypeSample types[TYPE_CAP]; memset(types,0,sizeof types);
    for (uint32_t i=0;i<count;i++) {
        TypeSample *type=&types[i]; type->pointer=u32(array+i*4);
        if (!type->pointer) continue;
        if (!word_at(o,type->pointer,v[KIND_OFFSET],&type->kind) ||
            !word_at(o,type->pointer,v[META_OFFSET],&type->meta)) return "record";
        if (type->kind!=v[SINGLETON_KIND] || type->meta!=v[META_EXPECTED]) continue;
        if (*matches==MATCH_CAP) { o->incomplete=true; return "candidate-cap"; }
        Candidate *c=&candidates[(*matches)++]; c->type=type->pointer;
        uint32_t extra,info; Sample *callback;
        if (!sampled_word(o,c,c->type,v[EXTRA_OFFSET],&extra) ||
            !sampled_word(o,c,extra,v[INFO_OFFSET],&info) ||
            !sample(o,c,info,v[CALLBACK_OFFSET],v[CALLBACK_BYTES],&callback)) return "callback-descriptor";
        if (u32(callback->data+v[MANAGER_OFFSET])!=v[MANAGER_EXPECTED] ||
            u32(callback->data+v[INVOKER_OFFSET])!=v[INVOKER_EXPECTED]) { o->incomplete=true; return "foreign-callback"; }
        c->capture=u32(callback->data+v[CAPTURE_OFFSET]);
        if (!sampled_word(o,c,c->capture,v[CONTROL_OFFSET],&c->control) ||
            !sampled_word(o,c,c->capture,v[OBJECT_OFFSET],&c->object) ||
            !sampled_word(o,c,c->control,v[STRONG_OFFSET],&c->strong)) return "guarded-pointer";
        if (!c->strong) { o->incomplete=true; return "expired"; }
        uint32_t vptr,backlink;
        if (!sampled_word(o,c,c->object,0,&vptr) || vptr!=v[VPTR_EXPECTED] ||
            !sampled_word(o,c,c->object,v[DPTR_OFFSET],&c->dptr) ||
            !sampled_word(o,c,c->dptr,v[BACKLINK_OFFSET],&backlink) || backlink!=c->object) {
            o->incomplete=true; return "object";
        }
    }
    for (uint32_t i=0;i<count;i++) if (types[i].pointer) {
        uint32_t kind,meta;
        if (!word_at(o,types[i].pointer,v[KIND_OFFSET],&kind) ||
            !word_at(o,types[i].pointer,v[META_OFFSET],&meta) || kind!=types[i].kind || meta!=types[i].meta) {
            o->incomplete=true; return "changed-record";
        }
    }
    for (size_t i=0;i<*matches;i++) for (size_t j=0;j<candidates[i].count;j++) {
        Sample *s=&candidates[i].fields[j]; unsigned char again[64];
        if (!read_exact(o,s->address,again,s->bytes) || memcmp(s->data,again,s->bytes)) {
            o->incomplete=true; return "changed-candidate";
        }
    }
    unsigned char initialized_again;
    if (!read_exact_bytes(o,guard,&initialized_again,1) || initialized_again!=initialized ||
        !read_exact(o,list,descriptor_again,12) || memcmp(descriptor,descriptor_again,12) ||
        (count && (!read_exact(o,buffer,array_again,(size_t)count*4) || memcmp(array,array_again,(size_t)count*4)))) {
        o->incomplete=true; return "changed-registry";
    }
    if (*matches!=1) { o->incomplete=true; return *matches?"ambiguous":"no-match"; }
    return "matched-sample-only";
}

#ifndef QML_OBSERVER_NO_MAIN
int main(int argc, char **argv) {
    uint64_t pid_value,start; Manifest manifest={0};
    if (argc!=4 || !number(argv[1],10,&pid_value) || !pid_value || pid_value>INT32_MAX ||
        !number(argv[2],10,&start) || !start || !load_manifest(argv[3],&manifest)) return 2;
    pid_t pid=(pid_t)pid_value; Observer *o=calloc(1,sizeof *o);
    char *maps=malloc(MAP_CAP+1),*maps_again=malloc(MAP_CAP+1);
    if (!o || !maps || !maps_again) { free(o);free(maps);free(maps_again);return 2; }
    o->read=remote_read; o->context=&pid; o->deadline=monotonic_ms()+2000;
    char path[64],exe_path[64]; struct stat exe={0},provider={0},exe_again,provider_again;
    size_t map_bytes=0,map_bytes_again=0;
    snprintf(path,sizeof path,"/proc/%ld/maps",(long)pid); snprintf(exe_path,sizeof exe_path,"/proc/%ld/exe",(long)pid);
    bool ready=budget(o) && identity(pid,start) && !stat(exe_path,&exe) && !stat(manifest.provider,&provider) &&
        bounded_file(path,maps,MAP_CAP,&map_bytes);
    if (ready) { memcpy(maps_again,maps,map_bytes+1);ready=parse_maps(o,maps_again); }
    uint32_t bias=0;
    ready=ready && provider_bias(o,&manifest,&provider,&bias) &&
        image_constant(o,manifest.value[META_EXPECTED],&exe,false) &&
        image_constant(o,manifest.value[VPTR_EXPECTED],&exe,false) &&
        image_constant(o,manifest.value[MANAGER_EXPECTED],&exe,true) &&
        image_constant(o,manifest.value[INVOKER_EXPECTED],&exe,true);
    Candidate candidates[MATCH_CAP]={0}; size_t matches=0,type_count=0;
    const char *phase="preflight";
    if (ready) phase=discover(o,&manifest,bias,candidates,&matches,&type_count); else o->fatal=true;
    if (!identity(pid,start) || stat(exe_path,&exe_again) || stat(manifest.provider,&provider_again) ||
        exe.st_ino!=exe_again.st_ino || exe.st_dev!=exe_again.st_dev ||
        provider.st_ino!=provider_again.st_ino || provider.st_dev!=provider_again.st_dev ||
        !bounded_file(path,maps_again,MAP_CAP,&map_bytes_again) || map_bytes!=map_bytes_again ||
        memcmp(maps,maps_again,map_bytes_again) || !budget(o)) { o->fatal=true;phase="process-or-mapping-change"; }
    emit(o,"{\"kind\":\"sampled-untrusted-existing-instance\",\"phase\":\"%s\",\"types\":%zu,\"matches\":%zu,\"candidates\":[",phase,type_count,matches);
    for (size_t i=0;i<matches;i++) emit(o,"%s{\"type\":\"%08" PRIx32 "\",\"capture\":\"%08" PRIx32
        "\",\"control\":\"%08" PRIx32 "\",\"object\":\"%08" PRIx32 "\",\"data\":\"%08" PRIx32
        "\",\"strong_sample\":%" PRId32 "}",i?",":"",candidates[i].type,candidates[i].capture,candidates[i].control,
        candidates[i].object,candidates[i].dptr,(int32_t)candidates[i].strong);
    emit(o,"],\"incomplete\":%s,\"atomic\":false,\"remote_bytes\":%zu}\n",(o->incomplete||o->fatal)?"true":"false",o->remote_bytes);
    int result=o->fatal?3:(o->incomplete?4:0);
    if (o->output_bytes>=OUTPUT_CAP || fwrite(o->output,1,o->output_bytes,stdout)!=o->output_bytes) result=3;
    free(o);free(maps);free(maps_again);return result;
}
#endif
