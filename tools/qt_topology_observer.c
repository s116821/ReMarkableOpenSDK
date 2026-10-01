/* Development-only sampled ARM32 QObject topology. No target writes or calls. */
#define _GNU_SOURCE
#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/uio.h>
#include <time.h>
#include <unistd.h>

enum { REMOTE_CAP=65536, OUTPUT_CAP=65536, MAP_CAP=131072, MAPS_CAP=1024,
       NODE_CAP=64, CHILD_CAP=64, DEPTH_CAP=4 };
typedef struct { uint64_t lo, hi, inode; unsigned dev_major, dev_minor; bool read, write; } Mapping;
typedef struct { uint32_t object_bytes, data_bytes, dptr, qptr, parent, children, count; } Layout;
typedef ssize_t (*ReadFn)(void *, uint32_t, void *, size_t);
typedef struct {
    Layout layout; Mapping maps[MAPS_CAP]; size_t map_count, remote_bytes, nodes;
    uint32_t visited[NODE_CAP]; ReadFn read; void *context;
    uint64_t deadline; bool incomplete, fatal;
    char output[OUTPUT_CAP]; size_t output_bytes;
} Observer;

static uint64_t monotonic_ms(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts)) return UINT64_MAX;
    return (uint64_t)ts.tv_sec*1000+(uint64_t)ts.tv_nsec/1000000;
}
static bool budget(Observer *o) {
    if (monotonic_ms() >= o->deadline) { o->fatal=true; return false; }
    return !o->fatal;
}
static const Mapping *range(const Observer *o, uint32_t pointer, size_t bytes) {
    uint64_t end=(uint64_t)pointer+bytes;
    if (!pointer || pointer%4 || !bytes || end>UINT64_C(0x100000000)) return NULL;
    for (size_t i=0;i<o->map_count;i++)
        if (o->maps[i].read && pointer>=o->maps[i].lo && end<=o->maps[i].hi) return &o->maps[i];
    return NULL;
}
static bool read_exact(Observer *o, uint32_t pointer, void *data, size_t bytes) {
    if (!budget(o) || !range(o,pointer,bytes) || bytes>REMOTE_CAP-o->remote_bytes) {
        o->incomplete=true; return false;
    }
    o->remote_bytes+=bytes; /* Charge the request even on partial/error transfer. */
    ssize_t got=o->read(o->context,pointer,data,bytes);
    if (got<0 && (errno==EPERM || errno==ENOSYS)) o->fatal=true;
    if (got!=(ssize_t)bytes || !budget(o)) { o->incomplete=true; return false; }
    return true;
}
static uint32_t u32(const unsigned char *p) {
    return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
}
static bool emit(Observer *o, const char *format, ...) {
    va_list args; va_start(args,format);
    int count=vsnprintf(o->output+o->output_bytes,OUTPUT_CAP-o->output_bytes,format,args);
    va_end(args);
    if (count<0 || (size_t)count>=OUTPUT_CAP-o->output_bytes) { o->fatal=true; return false; }
    o->output_bytes+=(size_t)count; return true;
}

/* No RTTI/class strings: unknown is safer than reading uncorroborated descriptors. */
static void visit(Observer *o, uint32_t object, uint32_t parent, unsigned depth) {
    if (!budget(o)) return;
    for (size_t i=0;i<o->nodes;i++) if (o->visited[i]==object) { o->incomplete=true; return; }
    if (o->nodes==NODE_CAP) { o->incomplete=true; return; }
    unsigned char header[64], data[64], header_again[64], data_again[64];
    const Layout *layout=&o->layout;
    if (!read_exact(o,object,header,layout->object_bytes)) return;
    const Mapping *vtable=range(o,u32(header),4);
    if (!vtable || vtable->write || !vtable->inode) { o->incomplete=true; return; }
    uint32_t dptr=u32(header+layout->dptr);
    if (!read_exact(o,dptr,data,layout->data_bytes)) return;
    uint32_t count=u32(data+layout->count), children=u32(data+layout->children);
    if (u32(data+layout->qptr)!=object || u32(data+layout->parent)!=parent || count>CHILD_CAP) {
        o->incomplete=true; return;
    }
    uint32_t addresses[CHILD_CAP]; unsigned char raw[CHILD_CAP*4], again[CHILD_CAP*4];
    if (count && !read_exact(o,children,raw,(size_t)count*4)) return;
    for (uint32_t i=0;i<count;i++) addresses[i]=u32(raw+i*4);
    if (!read_exact(o,object,header_again,layout->object_bytes) ||
        !read_exact(o,dptr,data_again,layout->data_bytes) ||
        memcmp(header,header_again,layout->object_bytes) || memcmp(data,data_again,layout->data_bytes) ||
        (count && (!read_exact(o,children,again,(size_t)count*4) || memcmp(raw,again,(size_t)count*4)))) {
        o->incomplete=true; return;
    }
    size_t ordinal=o->nodes;
    o->visited[o->nodes++]=object;
    emit(o,"%s{\"object\":\"%08" PRIx32 "\",\"data\":\"%08" PRIx32
         "\",\"vptr\":\"%08" PRIx32 "\",\"parent\":\"%08" PRIx32
         "\",\"depth\":%u,\"children\":%" PRIu32 ",\"class\":\"unknown\"}",
         ordinal?",":"",object,dptr,u32(header),parent,depth,count);
    if (depth==DEPTH_CAP && count) { o->incomplete=true; return; }
    for (uint32_t i=0;i<count && budget(o);i++) visit(o,addresses[i],object,depth+1);
    if (!read_exact(o,object,header_again,layout->object_bytes) ||
        !read_exact(o,dptr,data_again,layout->data_bytes) ||
        memcmp(header,header_again,layout->object_bytes) || memcmp(data,data_again,layout->data_bytes) ||
        (count && (!read_exact(o,children,again,(size_t)count*4) || memcmp(raw,again,(size_t)count*4))))
        o->incomplete=true;
}

#ifndef QT_TOPOLOGY_TEST
static ssize_t remote_read(void *context, uint32_t pointer, void *data, size_t bytes) {
    pid_t pid=*(pid_t *)context;
    struct iovec local={data,bytes}, remote={(void *)(uintptr_t)pointer,bytes};
    return process_vm_readv(pid,&local,1,&remote,1,0);
}
static bool bounded_file(const char *path, char *buffer, size_t cap, size_t *length) {
    FILE *file=fopen(path,"rb"); if (!file) return false;
    size_t n=fread(buffer,1,cap,file); int extra=fgetc(file);
    bool ok=!ferror(file) && extra==EOF; fclose(file);
    if (!ok) return false;
    buffer[n]='\0'; *length=n; return true;
}
static bool identity(pid_t pid, uint64_t expected) {
    char path[64], stat_text[4097]; size_t length;
    snprintf(path,sizeof path,"/proc/%ld/stat",(long)pid);
    if (!bounded_file(path,stat_text,4096,&length)) return false;
    char *end=strrchr(stat_text,')'); if (!end || end[1]!=' ') return false;
    char *save=NULL, *token=strtok_r(end+2," ",&save); unsigned field=3;
    if (!token || (token[0]!='R' && token[0]!='S') || token[1]) return false;
    while (token && field<22) { token=strtok_r(NULL," ",&save); field++; }
    if (!token) return false;
    errno=0; char *tail; unsigned long long start=strtoull(token,&tail,10);
    return !errno && !*tail && start==expected;
}
static bool parse_maps(Observer *o, char *text) {
    char *save=NULL;
    for (char *line=strtok_r(text,"\n",&save);line;line=strtok_r(NULL,"\n",&save)) {
        Mapping m; char perms[5]; uint64_t offset;
        if (sscanf(line,"%" SCNx64 "-%" SCNx64 " %4s %" SCNx64 " %x:%x %" SCNu64,
            &m.lo,&m.hi,perms,&offset,&m.dev_major,&m.dev_minor,&m.inode)!=7 ||
            m.lo>=m.hi || o->map_count==MAPS_CAP) return false;
        m.read=perms[0]=='r'; m.write=perms[1]=='w';
        o->maps[o->map_count++]=m;
    }
    return o->map_count>0;
}
static bool number(const char *text, int base, uint64_t *value) {
    if (!*text || *text=='-' || *text=='+') return false;
    errno=0; char *tail; *value=strtoull(text,&tail,base);
    return !errno && !*tail;
}
int main(int argc, char **argv) {
    /* Slot comes from separately hash-verified ELF COPY relocation + runtime maps.
       This collector cannot authenticate ELF/provider hashes; operator must do so. */
    if (argc!=11) { fputs("usage: observer PID START_TICKS SELF_SLOT_HEX OBJECT_BYTES DATA_BYTES DPTR_OFFSET QPTR_OFFSET PARENT_OFFSET CHILD_PTR_OFFSET CHILD_COUNT_OFFSET\n",stderr); return 2; }
    uint64_t pid_value,start,slot_value;
    if (!number(argv[1],10,&pid_value) || !pid_value || pid_value>INT32_MAX ||
        !number(argv[2],10,&start) || !start || !number(argv[3],16,&slot_value) ||
        !slot_value || slot_value>UINT32_MAX) return 2;
    uint32_t layout_values[7];
    for (unsigned i=0;i<7;i++) {
        uint64_t value;
        if (!number(argv[i+4],10,&value) || value>64) return 2;
        layout_values[i]=(uint32_t)value;
    }
    Layout layout={layout_values[0],layout_values[1],layout_values[2],layout_values[3],
                   layout_values[4],layout_values[5],layout_values[6]};
    if (!layout.object_bytes || !layout.data_bytes || layout.object_bytes%4 || layout.data_bytes%4 ||
        layout.dptr%4 || layout.dptr+4>layout.object_bytes) return 2;
    uint32_t offsets[4]={layout.qptr,layout.parent,layout.children,layout.count};
    for (unsigned i=0;i<4;i++) {
        if (offsets[i]%4 || offsets[i]+4>layout.data_bytes) return 2;
        for (unsigned j=0;j<i;j++) if (offsets[i]==offsets[j]) return 2;
    }
    pid_t pid=(pid_t)pid_value;
    Observer *o=calloc(1,sizeof *o); char *maps=malloc(MAP_CAP+1), *maps_again=malloc(MAP_CAP+1);
    if (!o || !maps || !maps_again) { free(o); free(maps); free(maps_again); return 2; }
    o->layout=layout; o->deadline=monotonic_ms()+2000; o->read=remote_read; o->context=&pid;
    char path[64],exe[64]; size_t map_bytes=0,map_bytes_again=0; struct stat executable={0};
    snprintf(path,sizeof path,"/proc/%ld/maps",(long)pid);
    snprintf(exe,sizeof exe,"/proc/%ld/exe",(long)pid);
    bool ready=budget(o) && identity(pid,start) && !stat(exe,&executable) &&
        bounded_file(path,maps,MAP_CAP,&map_bytes);
    if (ready) {
        memcpy(maps_again,maps,map_bytes+1);
        ready=parse_maps(o,maps_again);
    }
    const Mapping *slot_map=ready?range(o,(uint32_t)slot_value,4):NULL;
    ready=ready && slot_map && slot_map->write && slot_map->inode==(uint64_t)executable.st_ino &&
        slot_map->dev_major==major(executable.st_dev) && slot_map->dev_minor==minor(executable.st_dev);
    unsigned char root[4],root_again[4];
    emit(o,"{\"kind\":\"sampled-untrusted-topology\",\"nodes\":[");
    if (ready && read_exact(o,(uint32_t)slot_value,root,4)) {
        visit(o,u32(root),0,0);
        if (!read_exact(o,(uint32_t)slot_value,root_again,4) || memcmp(root,root_again,4)) o->fatal=true;
    } else o->fatal=true;
    struct stat executable_again;
    if (!identity(pid,start) || stat(exe,&executable_again) ||
        executable_again.st_ino!=executable.st_ino || executable_again.st_dev!=executable.st_dev ||
        !bounded_file(path,maps_again,MAP_CAP,&map_bytes_again) ||
        map_bytes_again!=map_bytes || memcmp(maps,maps_again,map_bytes_again)) o->fatal=true;
    if (!budget(o)) o->fatal=true;
    emit(o,"],\"incomplete\":%s,\"remote_bytes\":%zu,\"atomic\":false}\n",
        (o->incomplete||o->fatal)?"true":"false",o->remote_bytes);
    /* A bounded JSON result never claims authority; nonzero exit accompanies refusal. */
    int result=o->fatal?3:(o->incomplete?4:0);
    if (o->output_bytes>=OUTPUT_CAP || fwrite(o->output,1,o->output_bytes,stdout)!=o->output_bytes) result=3;
    free(o); free(maps); free(maps_again); return result;
}
#endif
