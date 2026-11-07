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
    for(int index=0;index<23;++index) {
        const auto stem=root+'/'+QStringLiteral("%1").arg(index,2,10,QChar('0'));
        const auto bytes=read(stem+".bin");const auto expectedHash=read(stem+".sha256");
        need(bytes.size()>4);auto frame=decode(bytes.mid(4));need(bool(frame));
        need(encode(*frame)==bytes);need(fingerprint(*frame)==expectedHash);
        need(frame->message==unsigned(index==0?0:index==1?1:(index<7||index==18||index==21)?2:3));
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
        for(char revision : {char(1),char(2),char(3),char(5)}) {auto incompatible=bytes.mid(4);incompatible[5]=revision;need(!decode(incompatible));}
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
    auto visual=decode(read(root+"/22.bin").mid(4));need(bool(visual));
    need(visual->facts.order.empty()&&visual->capture.after.order.empty()&&visual->capture.procedure==2);
    visual->capture.procedure=1;need(encode(*visual).isEmpty());
    auto textRequest=*decode(read(root+"/02.bin").mid(4));
    textRequest.kind=PreEnterRequest;textRequest.operation=QByteArray(15,0)+char(32);
    textRequest.textRange={2,3};
    const auto textRequestBytes=encode(textRequest);
    need(textRequestBytes.mid(207)==QByteArray::fromHex("0a000000020003"));
    auto textReply=textRequest;
    textReply.message=3;textReply.kind=PreEnterReply;
    textReply.requestDigest=fingerprint(textRequest);
    textReply.text.emplace(std::initializer_list<quint16>{0,0xd800,0x2029});
    const auto textBytes=encode(textReply);
    need(textBytes.mid(239)==QByteArray::fromHex("0c0000030000d8002029"));
    if(QFile::exists(root+"/23.bin")) {
        need(textRequestBytes==read(root+"/23.bin"));
        need(textBytes==read(root+"/24.bin"));
        for(int i=23;i<=27;++i) {
            const auto stem=root+'/'+QString::number(i);
            const auto bytes=read(stem+".bin");const auto parsed=decode(bytes.mid(4));
            need(parsed&&encode(*parsed)==bytes&&fingerprint(*parsed)==read(stem+".sha256"));
        }
    }
    need(validPreEnterReply(textRequest,textReply));
    textReply.text->pop_back();need(!validPreEnterReply(textRequest,textReply));
    textReply.text.reset();need(encode(textReply).mid(239)==QByteArray::fromHex("0c01"));
    need(validPreEnterReply(textRequest,textReply));
    for(const char *suffix:{"02","001001","00000100","0100"})
        need(!decode(textBytes.mid(4,236)+QByteArray::fromHex(suffix)));
    textRequest.textRange={0xffffffffU,1};need(encode(textRequest).isEmpty());
    if(QFile::exists(root+"/reader-focus-request.bin")) {
        const auto requestBytes=read(root+"/reader-focus-request.bin");
        const auto replyBytes=read(root+"/reader-focus-reply.bin");
        const auto focusRequest=decode(requestBytes.mid(4));
        const auto focusReply=decode(replyBytes.mid(4));
#ifdef RSDK_DEVELOPMENT_READER_FOCUS_V1
        need(requestBytes.size()==208&&requestBytes.mid(207)==QByteArray::fromHex("0b"));
        need(replyBytes.size()==241&&replyBytes.mid(239)==QByteArray::fromHex("0d03"));
        need(focusRequest&&focusReply&&validReaderFocusReply(*focusRequest,*focusReply));
        need(encode(*focusRequest)==requestBytes&&encode(*focusReply)==replyBytes);
        need(fingerprint(*focusRequest)==read(root+"/reader-focus-request.sha256"));
        need(fingerprint(*focusReply)==read(root+"/reader-focus-reply.sha256"));
        need(focusReply->requestDigest==fingerprint(*focusRequest));
        auto changed=*focusReply;
        for(int value=0;value<=5;++value) {
            changed.readerFocus=ReaderFocus(value);
            const auto roundTrip=decode(encode(changed).mid(4));
            need(roundTrip&&roundTrip->readerFocus==changed.readerFocus);
        }
        need(!decode(requestBytes.mid(4)+'x')&&!decode(replyBytes.mid(4)+'x'));
        need(!decode(replyBytes.mid(4,236))&&!decode(replyBytes.mid(4,236)+char(6)));
        need(!validReaderFocusReply(textRequest,*focusReply));
        need(!validReaderFocusReply(*focusRequest,textReply));
#else
        need(!focusRequest&&!focusReply);
#endif
        std::puts("PASS: reader focus Rust/Qt framing and feature gate");
    }
    std::printf("PASS: %d Rust/Qt vectors, %u checks\n",QFile::exists(root+"/23.bin")?28:23,checks);
}
