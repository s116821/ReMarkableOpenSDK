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
    for(int index=0;index<14;++index) {
        const auto stem=root+'/'+QStringLiteral("%1").arg(index,2,10,QChar('0'));
        const auto bytes=read(stem+".bin");const auto expectedHash=read(stem+".sha256");
        need(bytes.size()>4);auto frame=decode(bytes.mid(4));need(bool(frame));
        need(encode(*frame)==bytes);need(fingerprint(*frame)==expectedHash);
        need(frame->message==unsigned(index==0?0:index==1?1:index<5?2:3));
        for(qsizetype split=0;split<bytes.size();++split) {
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
        auto future=bytes.mid(4);future[5]=2;need(!decode(future));
        decoder.close();need(decoder.feed(bytes).error);
    }
    for(quint32 size:{quint32(0),quint32(MaxFrame+1),quint32(0xffffffff)}) {
        detail::Writer writer;writer.number<quint32>(size);writer.bytes+=QByteArray(100000,'x');
        Decoder decoder;need(decoder.feed(writer.bytes).error);need(decoder.feed({}).error);
    }
    std::printf("PASS: 14 Rust/Qt vectors, %u checks\n",checks);
}
