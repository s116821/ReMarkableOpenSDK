/* Owned synthetic buffers only; no proprietary process or ABI claim. */
#define main observer_main
#include "qt_topology_observer.c"
#undef main
#include <assert.h>

typedef struct { unsigned char bytes[4096]; int partial, change; unsigned calls; } Fixture;
static void put(Fixture *f, size_t offset, uint32_t value) {
    for (unsigned i=0;i<4;i++) f->bytes[offset+i]=(unsigned char)(value>>(8*i));
}
static ssize_t fixture_read(void *context, uint32_t address, void *bytes, size_t count) {
    Fixture *f=context; f->calls++;
    if (address<0x1000 || (uint64_t)address+count>0x2000) return -1;
    memcpy(bytes,f->bytes+address-0x1000,count);
    if (f->change && address==0x1040 && f->calls>=(unsigned)f->change) ((unsigned char *)bytes)[20]^=1;
    if (f->partial<0) { errno=EPERM; return -1; }
    return f->partial?(ssize_t)count-1:(ssize_t)count;
}
static void setup(Observer *o, Fixture *f) {
    memset(o,0,sizeof *o); memset(f,0,sizeof *f);
    o->layout=(Layout){8,24,4,4,8,16,20};
    o->deadline=monotonic_ms()+2000; o->context=f; o->read=fixture_read;
    o->maps[0]=(Mapping){.lo=0x1000,.hi=0x2000,.read=true,.write=true};
    o->maps[1]=(Mapping){.lo=0x2000,.hi=0x2100,.read=true,.inode=1}; o->map_count=2;
    put(f,0,0x2000); put(f,4,0x1040); put(f,0x44,0x1000);
}
static void child(Fixture *f) {
    put(f,0x54,1); put(f,0x50,0x1080); put(f,0x80,0x1100);
    put(f,0x100,0x2000); put(f,0x104,0x1140);
    put(f,0x144,0x1100); put(f,0x148,0x1000);
}
static void topology_cases(void) {
    Observer *o=calloc(1,sizeof *o); Fixture f; assert(o);
    setup(o,&f); child(&f); visit(o,0x1000,0,0);
    assert(o->nodes==2 && !o->incomplete && !o->fatal && o->remote_bytes<=REMOTE_CAP);
    setup(o,&f); f.partial=1; visit(o,0x1000,0,0); assert(!o->nodes && o->incomplete);
    setup(o,&f); f.change=3; visit(o,0x1000,0,0); assert(!o->nodes && o->incomplete);
    setup(o,&f); f.change=6; visit(o,0x1000,0,0); assert(o->nodes==1 && o->incomplete);
    setup(o,&f); f.partial=-1; visit(o,0x1000,0,0); assert(o->fatal && f.calls==1);
    for (uint32_t count=65;count;count=0) {
        setup(o,&f); put(&f,0x54,count); visit(o,0x1000,0,0); assert(!o->nodes && o->incomplete);
    }
    setup(o,&f); put(&f,0x54,UINT32_MAX); visit(o,0x1000,0,0); assert(!o->nodes && o->incomplete);
    setup(o,&f); put(&f,4,0xfffffffcu); visit(o,0x1000,0,0); assert(!o->nodes && o->incomplete);
    setup(o,&f); put(&f,4,0x1041); visit(o,0x1000,0,0); assert(!o->nodes && o->incomplete);
    setup(o,&f); put(&f,0x44,0x1100); visit(o,0x1000,0,0); assert(!o->nodes && o->incomplete);
    setup(o,&f); child(&f); put(&f,0x80,0x1000); visit(o,0x1000,0,0); assert(o->nodes==1 && o->incomplete);
    setup(o,&f); child(&f); visit(o,0x1000,0,DEPTH_CAP); assert(o->nodes==1 && o->incomplete);
    setup(o,&f); o->nodes=NODE_CAP; visit(o,0x1000,0,0); assert(o->incomplete && !f.calls);
    setup(o,&f); o->remote_bytes=REMOTE_CAP-7; visit(o,0x1000,0,0); assert(o->incomplete && !f.calls);
    setup(o,&f); o->deadline=monotonic_ms(); visit(o,0x1000,0,0); assert(o->fatal && !f.calls);
    setup(o,&f); o->maps[1].write=true; visit(o,0x1000,0,0); assert(o->incomplete);
    setup(o,&f); o->output_bytes=OUTPUT_CAP-1; assert(!emit(o,"too long") && o->fatal);
    free(o);
}
static uint64_t own_start(void) {
    char text[4097]; size_t length; assert(bounded_file("/proc/self/stat",text,4096,&length));
    char *save=NULL,*token=strtok_r(strrchr(text,')')+2," ",&save);
    for (unsigned field=3;field<22;field++) token=strtok_r(NULL," ",&save);
    assert(token); return strtoull(token,NULL,10);
}
static void process_cases(void) {
    uint64_t start=own_start(); assert(identity(getpid(),start)); assert(!identity(getpid(),start+1));
    static uint32_t value=0x12345678;
    uint32_t copy=0; pid_t pid=getpid();
    assert((uintptr_t)&value<=UINT32_MAX); /* Host fixture built for 32-bit target layout. */
    assert(remote_read(&pid,(uint32_t)(uintptr_t)&value,&copy,4)==4 && copy==value);
    char too_small[2]; size_t length; assert(!bounded_file("/proc/self/maps",too_small,1,&length));
    Observer *o=calloc(1,sizeof *o); assert(o);
    char valid[]="00001000-00002000 rw-p 00000000 00:00 0\n00002000-00002100 r--p 00000000 08:01 1\n";
    assert(parse_maps(o,valid) && o->map_count==2);
    char invalid[]="broken\n"; assert(!parse_maps(o,invalid)); free(o);
    /* Initialized globals remain in the owned executable's writable file mapping. */
    static uint32_t object[2]={1,1}, data[6]={1,1,1,1,1,1}, slot=1;
    static const uint32_t owned_vtable[2]={1,2};
    assert((uintptr_t)object<=UINT32_MAX && (uintptr_t)data<=UINT32_MAX && (uintptr_t)&slot<=UINT32_MAX);
    object[0]=(uint32_t)(uintptr_t)owned_vtable; object[1]=(uint32_t)(uintptr_t)data;
    data[1]=(uint32_t)(uintptr_t)object; data[2]=0; data[4]=0; data[5]=0;
    slot=(uint32_t)(uintptr_t)object;
    char pid_text[32],start_text[32],slot_text[32];
    snprintf(pid_text,sizeof pid_text,"%ld",(long)getpid());
    snprintf(start_text,sizeof start_text,"%" PRIu64,start);
    snprintf(slot_text,sizeof slot_text,"%" PRIx32,(uint32_t)(uintptr_t)&slot);
    char *args[]={"fixture",pid_text,start_text,slot_text,"8","24","4","4","8","16","20"};
    char own_maps[MAP_CAP+1]; size_t own_map_bytes;
    assert(bounded_file("/proc/self/maps",own_maps,MAP_CAP,&own_map_bytes));
    o=calloc(1,sizeof *o); assert(o && parse_maps(o,own_maps));
    const Mapping *slot_map=range(o,(uint32_t)(uintptr_t)&slot,4);
    struct stat own_exe; assert(!stat("/proc/self/exe",&own_exe));
    assert(slot_map && slot_map->write);
    assert(slot_map->inode==(uint64_t)own_exe.st_ino && slot_map->dev_major==major(own_exe.st_dev)
        && slot_map->dev_minor==minor(own_exe.st_dev));
    free(o);
    assert(observer_main(11,args)==0);
    snprintf(start_text,sizeof start_text,"%" PRIu64,start+1);
    assert(observer_main(11,args)==3);
    args[4]="65"; assert(observer_main(11,args)==2);
}
int main(void) { setbuf(stdout,NULL); topology_cases(); process_cases(); puts("owned topology/transfer/identity fixtures pass"); return 0; }
