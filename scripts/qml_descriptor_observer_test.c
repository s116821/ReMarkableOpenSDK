#define main descriptor_observer_main
#include "qml_descriptor_observer.c"
#undef main
#include <assert.h>
typedef struct { unsigned calls,total; unsigned char guard,descriptor[12]; unsigned partial_call,change_call; } Fixture;
static void word(unsigned char *p,uint32_t v) {for(unsigned i=0;i<4;i++)p[i]=(unsigned char)(v>>(8*i));}
static ssize_t read_owned(void *context,uint32_t address,void *out,size_t count) {
    Fixture *f=context;f->calls++;f->total+=(unsigned)count;
    assert(f->total<=26); /* No count can expand the fixed read set. */
    if (address==0x10fd && count==1) memcpy(out,&f->guard,1);
    else {assert(address==0x1100 && count==12);memcpy(out,f->descriptor,12);}
    if (f->change_call==f->calls) ((unsigned char *)out)[0]^=1;
    return f->partial_call==f->calls?(ssize_t)count-1:(ssize_t)count;
}
static void setup(Observer *o,Fixture *f,Manifest *m,uint32_t count) {
    memset(o,0,sizeof *o);memset(f,0,sizeof *f);memset(m,0,sizeof *m);
    o->read=read_owned;o->context=f;o->deadline=monotonic_ms()+2000;
    o->maps[0]=(Mapping){.lo=0x1000,.hi=0x2000,.read=true};o->map_count=1;
    m->value[GUARD_VA]=0x10fd;m->value[REGISTRY_VA]=0x1100;
    f->guard=0xff;word(f->descriptor,0xffffffff);word(f->descriptor+4,1);word(f->descriptor+8,count);
}
int main(void) {
    Observer *o=calloc(1,sizeof *o);assert(o);Fixture f;Manifest m;DescriptorSample samples[2];bool equal;
    uint32_t counts[]={0,1025,UINT32_MAX,0x80000000,INT32_MAX};
    for(unsigned i=0;i<sizeof counts/sizeof counts[0];i++) {
        setup(o,&f,&m,counts[i]);memset(samples,0,sizeof samples);equal=false;
        assert(!strcmp(observe_descriptor(o,&m,0,samples,&equal),"descriptor-sample-only"));
        assert(equal && !o->incomplete && f.calls==4 && f.total==26 && o->remote_bytes==26);
        descriptor_output(o,"descriptor-sample-only",samples,equal);assert(o->output_bytes<1024);
        assert(strstr(o->output,"\"allocation_data_word\":\"ffffffff\"") && strstr(o->output,"\"buffer_word\":\"00000001\""));
        if(counts[i]==UINT32_MAX)assert(strstr(o->output,"\"count_signed\":-1"));
        assert(!read_exact_bytes(o,0x10fd,&f.guard,1));assert(f.calls==4);
    }
    for(unsigned guard=0;guard<3;guard++) {
        setup(o,&f,&m,0);f.guard=guard==2?0xfe:(unsigned char)guard;
        memset(samples,0,sizeof samples);equal=false;
        assert(!strcmp(observe_descriptor(o,&m,0,samples,&equal),"guard"));
        assert(f.calls==1 && samples[0].guard_present && !samples[0].descriptor_present && !samples[1].guard_present);
        descriptor_output(o,"guard",samples,equal);assert(strstr(o->output,"\"count_unsigned\":null"));
    }
    for(unsigned partial=1;partial<=4;partial++) {
        setup(o,&f,&m,UINT32_MAX);f.partial_call=partial;memset(samples,0,sizeof samples);equal=false;
        observe_descriptor(o,&m,0,samples,&equal);assert(o->incomplete && f.calls==partial && o->remote_bytes<=26);
        assert(!samples[(partial-1)/2].descriptor_present);
        descriptor_output(o,"partial",samples,equal);assert(o->output_bytes<1024);
    }
    for(unsigned changed=3;changed<=4;changed++) {
        setup(o,&f,&m,0);f.change_call=changed;memset(samples,0,sizeof samples);equal=false;
        observe_descriptor(o,&m,0,samples,&equal);assert(o->incomplete && !equal && f.calls==changed);
        assert(samples[0].descriptor_present && samples[1].guard_present);
    }
    setup(o,&f,&m,0);o->deadline=0;memset(samples,0,sizeof samples);equal=false;
    observe_descriptor(o,&m,0,samples,&equal);assert(o->fatal && !f.calls && !samples[0].guard_present);
    setup(o,&f,&m,0);m.value[REGISTRY_VA]=0x1ffc;memset(samples,0,sizeof samples);
    observe_descriptor(o,&m,0,samples,&equal);assert(o->incomplete && f.calls==1);
    setup(o,&f,&m,0);memset(samples,0,sizeof samples);equal=false;
    assert(!strcmp(observe_descriptor(o,&m,UINT32_MAX,samples,&equal),"bounds") && !f.calls);
    free(o);puts("owned descriptor-only fixed26-byte/no-traversal fixtures pass");return 0;
}
