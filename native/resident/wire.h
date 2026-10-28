#pragma once
// SDK resident protocol v3. Parsed values are peer assertions, not native leases.
#include <QByteArray>
#include <QCryptographicHash>
#include <QSet>
#include <QtEndian>
#include <cstdint>
#include <optional>
#include <vector>
#include <cmath>
#include <cstring>

namespace remarkable::resident {
constexpr qsizetype MaxFrame = 65536;
constexpr qsizetype MaxCapturePng = 1048576, MaxCaptureFrame = MaxCapturePng + MaxFrame;
constexpr std::size_t MaxPages = 2048;
using Id = QByteArray; // Exactly sixteen nonzero bytes, validated on the wire.
inline bool validId(const Id &id) { return id.size()==16 && id!=QByteArray(16,0); }
struct Facts {
    Id document,page;
    std::vector<Id> order;
};
struct Capture {
    Facts after;
    quint32 width=0,height=0;
    quint64 dprBits=0;
    quint8 procedure=1;
    QByteArray sha256,png;
};
struct Frame {
    // Exact wire runtime: three digests, boot UUID, PID, start ticks, activation UUID.
    QByteArray runtime;
    Id client,server,operation;
    quint64 sequence=0;
    unsigned char message=0; // Hello, Ready, Request, Response.
    unsigned char kind=0; // Command or reply kind, depending on message.
    QByteArray requestDigest;
    Facts facts;
    Capture capture;
    Id target;
    bool targetPresent=false;
    unsigned char rejection=0;
};
inline bool validFacts(const Facts &facts) {
    if(!validId(facts.document)||!validId(facts.page)||facts.order.empty()||facts.order.size()>MaxPages)return false;
    QSet<Id> seen;
    for(const auto &id:facts.order)if(!validId(id)||seen.contains(id))return false;else seen.insert(id);
    return seen.contains(facts.page);
}
inline bool validSelection(const Facts &facts,const Id &target) {
    if(!validFacts(facts)||!validId(target))return false;
    for(std::size_t n=0;n<facts.order.size();++n)if(facts.order[n]==facts.page)
        return target==facts.page||(n+1<facts.order.size()&&facts.order[n+1]==target);
    return false;
}
// Wire envelope validation only; the Rust consumer additionally performs a
// bounded full PNG decode before exposing HistoricalCapture. Native producers
// supply their private, successfully encoded image after final joined admission.
inline bool validCapture(const Facts &before,const Capture &c) {
    double dpr=0;static_assert(sizeof dpr==sizeof c.dprBits);std::memcpy(&dpr,&c.dprBits,sizeof dpr);
    if(!validFacts(before)||before.order.size()>17||before.document!=c.after.document||
       before.page!=c.after.page||before.order!=c.after.order||!c.width||!c.height||
       c.width>1872||c.height>1872||quint64(c.width)*c.height>2628288||!std::isfinite(dpr)||dpr<=0||
       c.procedure!=1||c.sha256.size()!=32||c.png.size()<45||c.png.size()>MaxCapturePng)return false;
    const auto *p=reinterpret_cast<const uchar*>(c.png.constData());
    return c.png.left(8)==QByteArray("\x89PNG\r\n\x1a\n",8)&&qFromBigEndian<quint32>(p+8)==13&&
        c.png.mid(12,4)=="IHDR"&&qFromBigEndian<quint32>(p+16)==c.width&&qFromBigEndian<quint32>(p+20)==c.height&&
        QCryptographicHash::hash(c.png,QCryptographicHash::Sha256)==c.sha256;
}
namespace detail {
class Reader {
    const QByteArray &bytes_;
    qsizetype offset_=0;
public:
    bool good=true;
    explicit Reader(const QByteArray &bytes):bytes_(bytes) {}
    QByteArray take(qsizetype n) {
        if(!good||n<0||n>bytes_.size()-offset_) {good=false;return {};}
        const auto value=bytes_.mid(offset_,n);offset_+=n;return value;
    }
    template<class T>T number() {
        const auto bytes=take(sizeof(T));
        return good?qFromBigEndian<T>(reinterpret_cast<const uchar*>(bytes.constData())):T{};
    }
    Id id(){auto value=take(16);if(!validId(value))good=false;return value;}
    std::vector<Id> order() {
        const auto count=number<quint16>();std::vector<Id> result;
        if(!count||count>MaxPages||qsizetype(count)>remaining()/16) {good=false;return result;}
        result.reserve(count);QSet<Id> seen;
        for(unsigned n=0;n<count&&good;++n){auto value=id();if(seen.contains(value))good=false;seen.insert(value);result.push_back(value);}
        return result;
    }
    Facts facts() {
        Facts result;result.document=id();result.page=id();result.order=order();
        if(!validFacts(result))good=false;
        return result;
    }
    qsizetype remaining()const{return bytes_.size()-offset_;}
};
class Writer {
public:
    QByteArray bytes;
    template<class T>void number(T value) {
        const auto offset=bytes.size();bytes.resize(offset+sizeof(T));
        qToBigEndian<T>(value,reinterpret_cast<uchar*>(bytes.data()+offset));
    }
    void order(const std::vector<Id> &ids){number<quint16>(quint16(ids.size()));for(const auto &id:ids)bytes+=id;}
    void facts(const Facts &value){bytes+=value.document;bytes+=value.page;order(value.order);}
};
}
inline std::optional<Frame> decode(const QByteArray &payload) {
    if(payload.size()>MaxCaptureFrame)return {};
    detail::Reader r(payload);Frame f;
    if(r.take(4)!="RSDK"||r.number<quint16>()!=3)return {};
    f.runtime=r.take(140);
    if(f.runtime.size()!=140)return {};
    for(int offset:{0,32,64})if(f.runtime.mid(offset,32)==QByteArray(32,0))return {};
    if(!validId(f.runtime.mid(96,16))||!validId(f.runtime.mid(124,16))||
       qFromBigEndian<quint32>(reinterpret_cast<const uchar*>(f.runtime.constData()+112))==0||
       qFromBigEndian<quint64>(reinterpret_cast<const uchar*>(f.runtime.constData()+116))==0)return {};
    f.client=r.id();f.server=r.take(16);f.sequence=r.number<quint64>();f.operation=r.id();f.message=r.number<quint8>();
    if(f.message>3 || ((f.message<2)!=(f.sequence==0)) ||
       (f.message==0?f.server!=QByteArray(16,0):!validId(f.server)))return {};
    if(f.message==2) {
        f.kind=r.number<quint8>();if(f.kind>5)return {};
        if(f.kind==5) {f.facts=r.facts();if(f.facts.order.size()>17)return {};}
        else if(f.kind) {
            f.facts=r.facts();f.target=r.id();
            if(f.kind<=2){for(const auto &id:f.facts.order)if(id==f.target)return {};}
            else if(!validSelection(f.facts,f.target))return {};
            if(f.kind==1&&f.facts.order.size()==MaxPages)return {};
        }
    } else if(f.message==3) {
        f.requestDigest=r.take(32);f.kind=r.number<quint8>();if(f.kind>7)return {};
        if(f.kind==7) {
            f.facts=r.facts();auto &c=f.capture;c.after=r.facts();
            c.width=r.number<quint32>();c.height=r.number<quint32>();c.dprBits=r.number<quint64>();
            c.procedure=r.number<quint8>();c.sha256=r.take(32);
            const auto count=r.number<quint32>();if(count>quint32(MaxCapturePng)||qsizetype(count)!=r.remaining())return {};
            c.png=r.take(count);if(!validCapture(f.facts,c))return {};
        } else if(f.kind==0||f.kind==5)f.facts=r.facts();
        else if(f.kind==1) {
            f.facts.document=r.id();f.target=r.id();f.facts.page=f.target;f.facts.order=r.order();
            if(!validFacts(f.facts))return {};
        } else if(f.kind==2||f.kind==6) {
            f.facts=r.facts();const auto present=r.number<quint8>();if(present>1)return {};f.targetPresent=present;
        } else if(f.kind==3) {f.rejection=r.number<quint8>();if(f.rejection>4)return {};}
    }
    if(!r.good||r.remaining()||(payload.size()>MaxFrame&&!(f.message==3&&f.kind==7)))return {};
    return f;
}
inline QByteArray encode(const Frame &f) {
    // Reject oversized fields before appending any caller-controlled collection.
    if(f.runtime.size()!=140||f.client.size()!=16||f.server.size()!=16||f.operation.size()!=16||
       f.facts.order.size()>MaxPages||f.capture.png.size()>MaxCapturePng)return {};
    if((f.message==2&&f.kind)||(f.message==3&&(f.kind<3||f.kind==5||f.kind==6))) {
        if(!validFacts(f.facts))return {};
    }
    if(((f.message==2&&f.kind&&f.kind!=5)||(f.message==3&&f.kind==1))&&!validId(f.target))return {};
    if(f.message==3&&f.kind==7&&!validCapture(f.facts,f.capture))return {};
    detail::Writer w;w.bytes="RSDK";w.number<quint16>(3);w.bytes+=f.runtime;
    w.bytes+=f.client;w.bytes+=f.server;w.number<quint64>(f.sequence);w.bytes+=f.operation;w.number<quint8>(f.message);
    if(f.message==2) {
        w.number<quint8>(f.kind);
        if(f.kind){w.facts(f.facts);if(f.kind!=5)w.bytes+=f.target;}
    } else if(f.message==3) {
        if(f.requestDigest.size()!=32)return {};
        w.bytes+=f.requestDigest;w.number<quint8>(f.kind);
        if(f.kind==7) {
            const auto &c=f.capture;w.facts(f.facts);w.facts(c.after);
            w.number<quint32>(c.width);w.number<quint32>(c.height);w.number<quint64>(c.dprBits);
            w.number<quint8>(c.procedure);w.bytes+=c.sha256;w.number<quint32>(quint32(c.png.size()));w.bytes+=c.png;
        } else if(f.kind==0||f.kind==5)w.facts(f.facts);
        else if(f.kind==1){w.bytes+=f.facts.document;w.bytes+=f.target;w.order(f.facts.order);}
        else if(f.kind==2||f.kind==6){w.facts(f.facts);w.number<quint8>(f.targetPresent?1:0);}
        else if(f.kind==3)w.number<quint8>(f.rejection);
    }
    if(!decode(w.bytes))return {};
    detail::Writer framed;framed.number<quint32>(quint32(w.bytes.size()));framed.bytes+=w.bytes;return framed.bytes;
}
inline QByteArray fingerprint(const Frame &frame) {
    const auto bytes=encode(frame);return bytes.isEmpty()?QByteArray{}:QCryptographicHash::hash(bytes,QCryptographicHash::Sha256);
}
// One frame at a time; caller keeps unconsumed bytes. Errors permanently poison.
class Decoder {
    QByteArray bytes_;
    bool poisoned_=false;
public:
    struct Feed {qsizetype consumed=0;std::optional<Frame> frame;bool error=false;};
    Feed feed(const QByteArray &input) {
        Feed result;
        auto fail=[&] {poisoned_=true;bytes_.clear();result.error=true;return result;};
        if(poisoned_)return fail();
        if(bytes_.size()<4) {
            const auto count=qMin(qsizetype(4)-bytes_.size(),input.size());bytes_+=input.left(count);result.consumed+=count;
            if(bytes_.size()<4)return result;
        }
        const auto size=qFromBigEndian<quint32>(reinterpret_cast<const uchar*>(bytes_.constData()));
        if(!size||size>MaxCaptureFrame)return fail();
        const auto count=qMin(qsizetype(size)+4-bytes_.size(),input.size()-result.consumed);
        bytes_+=input.mid(result.consumed,count);result.consumed+=count;
        if(bytes_.size()!=qsizetype(size)+4)return result;
        result.frame=decode(bytes_.mid(4));if(!result.frame)return fail();bytes_.clear();return result;
    }
    void close(){poisoned_=true;bytes_.clear();}
};
}
