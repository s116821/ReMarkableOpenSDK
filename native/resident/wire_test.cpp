#include "wire.h"
#include <QFile>
#include <cstdio>
#include <cstdlib>
using namespace remarkable::resident;
static unsigned checks=0;
static void need(bool value) {++checks;if(!value){std::fprintf(stderr,"wire check %u failed\n",checks);std::abort();}}
static QByteArray read(const QString &path) {QFile file(path);need(file.open(QIODevice::ReadOnly));return file.readAll();}
int main(int argc,char **argv) {
    need(argc==2);const QString root=QString::fromLocal8Bit(argv[1]);
    for(int index=0;index<21;++index) {
        const auto stem=root+'/'+QStringLiteral("%1").arg(index,2,10,QChar('0'));
        const auto bytes=read(stem+".bin");const auto expectedHash=read(stem+".sha256");
        need(bytes.size()>4);auto frame=decode(bytes.mid(4));need(bool(frame));
        need(encode(*frame)==bytes);need(fingerprint(*frame)==expectedHash);
        need(frame->message==unsigned(index==0?0:index==1?1:(index<7||index==18)?2:3));
        if(frame->message==2&&frame->kind>=3&&frame->kind<5) {
            auto changed=*frame;changed.target=changed.facts.page;need(!encode(changed).isEmpty());
            changed.target=changed.facts.order.front();need(encode(changed).isEmpty());
            changed.target=QByteArray(16,'x');need(encode(changed).isEmpty());
        }
        for(qsizetype split=0;split<bytes.size();split+=bytes.size()>MaxFrame?65521:1) {
            Decoder decoder;auto first=decoder.feed(bytes.left(split));need(!first.error&&!first.frame&&first.consumed==split);
            auto second=decoder.feed(bytes.mid(split));need(!second.error&&bool(second.frame)&&second.consumed==bytes.size()-split);
            need(encode(*second.frame)==bytes);
            if(split>=4)need(!decode(bytes.mid(4,split-4)));
        }
        Decoder decoder;const auto joined=bytes+bytes;auto first=decoder.feed(joined);
        need(!first.error&&bool(first.frame)&&first.consumed==bytes.size());
        auto second=decoder.feed(joined.mid(first.consumed));need(!second.error&&bool(second.frame));
        need(!decode(bytes.mid(4)+'x'));
        auto nil=bytes.mid(4);nil.replace(102,16,QByteArray(16,0));need(!decode(nil));
        for(char revision : {char(1),char(2),char(4)}) {auto incompatible=bytes.mid(4);incompatible[5]=revision;need(!decode(incompatible));}
        decoder.close();need(decoder.feed(bytes).error);
    }
    for(quint32 size:{quint32(0),quint32(MaxCaptureFrame+1),quint32(0xffffffff)}) {
        detail::Writer writer;writer.number<quint32>(size);writer.bytes+=QByteArray(100000,'x');
        Decoder decoder;need(decoder.feed(writer.bytes).error);need(decoder.feed({}).error);
    }
    const auto capture=decode(read(root+"/19.bin").mid(4));need(bool(capture));
    need(capture->capture.png.size()>MaxFrame);
    for(int fault=0;fault<5;++fault) {
        auto changed=*capture;
        if(fault==0)changed.capture.png[70]^=1;
        if(fault==1)changed.capture.png.resize(MaxCapturePng+1);
        if(fault==2)changed.capture.after.order[0]=changed.capture.after.order[1];
        if(fault==3)changed.capture.procedure=2;
        if(fault==4)changed.capture.dprBits=0;
        need(encode(changed).isEmpty());
    }
    auto oversized=read(root+"/02.bin").mid(4);oversized.resize(MaxFrame+1);need(!decode(oversized));
    std::printf("PASS: 21 Rust/Qt vectors, %u checks\n",checks);
}
