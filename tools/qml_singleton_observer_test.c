#define main qml_observer_main
#include "qml_singleton_observer.c"
#undef main
#include <assert.h>

typedef struct { unsigned char data[0x8000]; unsigned calls,changed_reads,capture_reads,extra_reads; uint32_t change; bool partial; } Fixture;
static void put(Fixture *f,uint32_t address,uint32_t value) {
    assert(address>=0x1000 && address+4<=0x9000);
    for (unsigned i=0;i<4;i++) f->data[address-0x1000+i]=(unsigned char)(value>>(8*i));
}
static ssize_t fixture_read(void *context,uint32_t address,void *bytes,size_t count) {
    Fixture *f=context; f->calls++;
    if (address>=0x2200 && address<0x2600) f->capture_reads++;
    if (address>=0x1900 && address<0x2200) f->extra_reads++;
    if (address<0x1000 || (uint64_t)address+count>0x9000) { errno=EFAULT;return -1; }
    memcpy(bytes,f->data+address-0x1000,count);
    if (address==f->change && ++f->changed_reads>=2) ((unsigned char *)bytes)[0]^=4;
    return f->partial?(ssize_t)count-1:(ssize_t)count;
}
static void setup(Observer *o,Fixture *f,Manifest *m) {
    memset(o,0,sizeof *o);memset(f,0,sizeof *f);memset(m,0,sizeof *m);
    strcpy(m->provider,"/owned/provider");
    uint32_t values[MANIFEST_FIELDS]={0x1000,0x1000,0x400,0x800,0x1100,0x10fd,
        60,68,8,1,0xa000,0,20,16,0,8,12,0xa100,0xa200,0,4,4,0xa300,4,4};
    memcpy(m->value,values,sizeof values);assert(manifest_valid(m));
    o->read=fixture_read;o->context=f;o->deadline=monotonic_ms()+2000;
    o->maps[0]=(Mapping){.lo=0x1000,.hi=0x9000,.read=true,.write=true};o->map_count=1;
    f->data[0xfd]=0xff;put(f,0x1104,0x1400);put(f,0x1108,1);put(f,0x1400,0x1800);
    put(f,0x1800+60,1);put(f,0x1800+68,0xa000);put(f,0x1808,0x1900);put(f,0x1900,0x2000);
    put(f,0x2014,0x2200);put(f,0x201c,0xa100);put(f,0x2020,0xa200);
    put(f,0x2200,0x2300);put(f,0x2204,0x2400);put(f,0x2304,UINT32_MAX);
    put(f,0x2400,0xa300);put(f,0x2404,0x2500);put(f,0x2504,0x2400);
}
static const char *run(Observer *o,Manifest *m,Candidate *c,size_t *matches) {
    size_t types=0;memset(c,0,MATCH_CAP*sizeof *c);*matches=0;
    return discover(o,m,0,c,matches,&types);
}
static void discovery_cases(void) {
    Observer *o=calloc(1,sizeof *o);Fixture *f=calloc(1,sizeof *f);Manifest m;Candidate c[MATCH_CAP];size_t matches;
    assert(o && f);setup(o,f,&m);
    assert(!strcmp(run(o,&m,c,&matches),"matched-sample-only") && matches==1 && !o->incomplete);
    assert(c[0].object==0x2400 && c[0].strong==UINT32_MAX && o->remote_bytes<REMOTE_CAP);
    for (unsigned guard=0;guard<3;guard++) {
        setup(o,f,&m);f->data[0xfd]=(unsigned char)(guard==2?0xfe:guard);
        assert(!strcmp(run(o,&m,c,&matches),"guard") && f->calls==1 && !f->extra_reads && !f->capture_reads);
    }
    for (unsigned role=0;role<2;role++) {
        setup(o,f,&m);put(f,role?0x1800+68:0x1800+60,7);
        assert(!strcmp(run(o,&m,c,&matches),"no-match") && !f->extra_reads && !f->capture_reads);
    }
    for (unsigned slot=0;slot<2;slot++) {
        setup(o,f,&m);put(f,slot?0x2020:0x201c,0xa101);
        assert(!strcmp(run(o,&m,c,&matches),"foreign-callback") && !f->capture_reads);
    }
    setup(o,f,&m);put(f,0x2304,0);assert(!strcmp(run(o,&m,c,&matches),"expired"));
    setup(o,f,&m);put(f,0x2200,0);assert(!strcmp(run(o,&m,c,&matches),"guarded-pointer"));
    setup(o,f,&m);put(f,0x2400,0xa304);assert(!strcmp(run(o,&m,c,&matches),"object"));
    setup(o,f,&m);put(f,0x2504,0x2404);assert(!strcmp(run(o,&m,c,&matches),"object"));
    for (unsigned count=2;count<=5;count+=3) {
        setup(o,f,&m);put(f,0x1108,count);
        for (unsigned i=0;i<count;i++) put(f,0x1400+i*4,0x1800);
        assert(!strcmp(run(o,&m,c,&matches),count==2?"ambiguous":"candidate-cap") && o->incomplete && matches<=4);
    }
    for (unsigned large=0;large<2;large++) {
        setup(o,f,&m);put(f,0x1108,large?UINT32_MAX:TYPE_CAP+1);
        assert(!strcmp(run(o,&m,c,&matches),"type-cap") && f->calls==2);
    }
    /* A full non-null registry, with its sole matching record at the end.
     * Keep the array separate from the fixed candidate chain. */
    const unsigned counts[]={3096,TYPE_CAP};
    for (size_t k=0;k<sizeof counts/sizeof counts[0];k++) {
        setup(o,f,&m);put(f,0x1104,0x4000);put(f,0x1108,counts[k]);
        put(f,0x3000+60,0);put(f,0x3000+68,0);
        for (unsigned i=0;i<counts[k];i++) put(f,0x4000+i*4,i+1==counts[k]?0x1800:0x3000);
        assert(!strcmp(run(o,&m,c,&matches),"matched-sample-only") && matches==1 && !o->incomplete);
        assert(o->remote_bytes==24u*counts[k]+26u+96u && o->remote_bytes<REMOTE_CAP);
        /* Changed high-count array data is rejected on the second array read. */
        o->remote_bytes=0;o->incomplete=false;f->change=0x4000;f->changed_reads=0;
        assert(!strcmp(run(o,&m,c,&matches),"changed-registry") && o->incomplete);
        /* Exhaust the remaining budget before this same high-count sample. */
        o->remote_bytes=REMOTE_CAP-13;o->incomplete=false;f->change=0;f->changed_reads=0;
        assert(!strcmp(run(o,&m,c,&matches),"type-array") && o->incomplete && o->remote_bytes==REMOTE_CAP);
    }
    const uint32_t changed[]={0x1800+60,0x2014,0x2200,0x2400,0x2504,0x10fd,0x1100,0x1400};
    for (size_t i=0;i<sizeof changed/sizeof changed[0];i++) {
        setup(o,f,&m);f->change=changed[i];
        const char *phase=run(o,&m,c,&matches);
        assert(strstr(phase,"changed-")==phase && o->incomplete);
    }
    setup(o,f,&m);f->partial=true;assert(!strcmp(run(o,&m,c,&matches),"guard") && o->incomplete);
    setup(o,f,&m);o->deadline=monotonic_ms();run(o,&m,c,&matches);assert(o->fatal && !f->calls);
    setup(o,f,&m);o->remote_bytes=REMOTE_CAP;run(o,&m,c,&matches);assert(o->incomplete && !f->calls);
    setup(o,f,&m);put(f,0x1400,0xfffffffcu);assert(!strcmp(run(o,&m,c,&matches),"record"));
    setup(o,f,&m);put(f,0x1400,0x1801);assert(!strcmp(run(o,&m,c,&matches),"record"));
    setup(o,f,&m);o->output_bytes=OUTPUT_CAP-1;assert(!emit(o,"too long") && o->fatal);
    free(o);free(f);
}
static void manifest_and_mapping_cases(void) {
    Observer *o=calloc(1,sizeof *o);Fixture *f=calloc(1,sizeof *f);Manifest m;assert(o && f);setup(o,f,&m);
    Manifest bad=m;bad.value[META_OFFSET]=129;assert(!manifest_valid(&bad));
    bad=m;bad.value[GUARD_VA]=0x9000;assert(!manifest_valid(&bad));
    bad=m;bad.value[CALLBACK_BYTES]=8;assert(!manifest_valid(&bad));
    bad=m;bad.value[CONTROL_OFFSET]=bad.value[OBJECT_OFFSET];assert(!manifest_valid(&bad));
    struct stat provider={.st_ino=3,.st_dev=makedev(8,1)};uint32_t bias;
    o->map_count=3;
    o->maps[0]=(Mapping){.lo=0x51000,.hi=0x52000,.read=true,.inode=3,.offset=0x1000,.dev_major=8,.dev_minor=1};
    o->maps[1]=(Mapping){.lo=0x52000,.hi=0x53000,.read=true,.write=true,.inode=3,.offset=0x2000,.dev_major=8,.dev_minor=1};
    o->maps[2]=(Mapping){.lo=0x53000,.hi=0x54000,.read=true,.write=true};
    m.value[FILE_BYTES]=0x1600;m.value[MEMORY_BYTES]=0x2800;
    assert(provider_bias(o,&m,&provider,&bias) && bias==0x50000); /* RELRO split and anonymous BSS. */
    provider.st_ino=4;assert(!provider_bias(o,&m,&provider,&bias));provider.st_ino=3;
    o->maps[1].lo+=4;assert(!provider_bias(o,&m,&provider,&bias));o->maps[1].lo-=4;
    o->maps[2].lo+=4;assert(!provider_bias(o,&m,&provider,&bias));
    FILE *file=fopen("/tmp/owned-r2-manifest","wb");assert(file);
    fprintf(file,"qml-existing-instance-arm32-v1\n%s\n",m.provider);
    for (unsigned i=0;i<MANIFEST_FIELDS;i++) fprintf(file,"%x ",m.value[i]);
    fclose(file);
    Manifest restored={0};assert(load_manifest("/tmp/owned-r2-manifest",&restored));
    assert(!memcmp(m.value,restored.value,sizeof m.value));
    file=fopen("/tmp/owned-r2-manifest","ab");assert(file);fputs("unknown",file);fclose(file);
    assert(!load_manifest("/tmp/owned-r2-manifest",&restored));unlink("/tmp/owned-r2-manifest");
    free(o);free(f);
}
int main(void) { discovery_cases();manifest_and_mapping_cases();puts("owned fixed-shape singleton fixtures pass");return 0; }
