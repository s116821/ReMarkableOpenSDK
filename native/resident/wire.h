#pragma once
// SDK resident protocol v4. Parsed values are peer assertions, not native leases.
#include <QByteArray>
#include <QCryptographicHash>
#include <QSet>
#include <QtEndian>
#include <cstdint>
#include <optional>
#include <vector>
#include <cmath>
#include <cstring>
#include <algorithm>

namespace remarkable::resident {
constexpr qsizetype MaxFrame = 65536;
constexpr qsizetype MaxCapturePng = 1048576, MaxCaptureFrame = MaxCapturePng + MaxFrame;
constexpr std::size_t MaxPages = 2048;
constexpr quint8 PreEnterRequest=10,PreEnterReply=12;
constexpr quint8 ReaderFocusRequest=11,ReaderFocusReply=13;
inline constexpr bool isComposerRequest(quint8 kind) {
#ifdef RSDK_DEVELOPMENT_COMPOSER_V1
    return kind>=12&&kind<=15;
#else
    (void)kind;return false;
#endif
}
inline constexpr bool isComposerReply(quint8 kind) {
#ifdef RSDK_DEVELOPMENT_COMPOSER_V1
    return kind>=14&&kind<=17;
#else
    (void)kind;return false;
#endif
}
inline constexpr bool isReaderFocusRequest([[maybe_unused]] quint8 kind) {
#ifdef RSDK_DEVELOPMENT_READER_FOCUS_V1
    return kind==ReaderFocusRequest;
#else
    return false;
#endif
}
inline constexpr bool isReaderFocusReply([[maybe_unused]] quint8 kind) {
#ifdef RSDK_DEVELOPMENT_READER_FOCUS_V1
    return kind==ReaderFocusReply;
#else
    return false;
#endif
}
#ifdef RSDK_DEVELOPMENT_READER_FOCUS_V1
enum class ReaderFocus : quint8 {
    Refused=0, NoLeaf=1, ExactScene=2, InsideScene=3, ScopeOnly=4, OutsideScope=5
};
#endif
constexpr quint16 MaxPreEnterUnits=4096;
struct TextRange {quint32 offset=0;quint16 count=0;};
inline bool validTextRange(TextRange range) {
    return range.count<=MaxPreEnterUnits&&quint64(range.offset)+range.count<=2147483646;
}
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
#ifdef RSDK_DEVELOPMENT_COMPOSER_V1
struct Composer {
    quint8 status=1,attempts=0;
    Id visit,range,event;
    qint32 revision=0,afterRevision=0;
    std::vector<quint16> text,keyText;
    std::vector<qint32> keys;
    quint64 inputSerial=0;
    qint32 key=0;
    quint16 count=0;
    quint32 modifiers=0,nativeScan=0,nativeVirtual=0,nativeModifiers=0;
    bool autoRepeat=false,spontaneous=false;
};
inline bool validComposerKeys(const std::vector<qint32> &keys) {
    if(keys.empty()||keys.size()>4)return false;
    QSet<qint32> seen;
    for(auto key:keys) {
        if(key<=0||key>=0x01ffffff||seen.contains(key))return false;
        seen.insert(key);
    }
    return true;
}
inline bool validComposerOutcome(quint8 kind,const Composer &c) {
    if(!isComposerReply(kind)||c.status>2||(c.attempts&~quint8(7)))return false;
    const bool effect=kind==14||kind==16;
    if(!effect&&(c.attempts&2))return false;
    if(c.status==0) {
        if(c.attempts!=(effect?7:5)||!validId(c.visit)||!validId(c.range)||c.revision<0)return false;
        if(kind==16&&c.afterRevision<0)return false;
        if((kind==15||kind==17)&&c.text.size()>4096)return false;
        if(kind==17&&(!validId(c.event)||!c.inputSerial||c.key<=0||c.key>=0x01ffffff||c.keyText.size()>64))return false;
        return true;
    }
    if(c.status==1)return c.attempts==0||c.attempts==5;
    return (c.attempts&1)!=0;
}
#endif
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
    TextRange textRange;
    std::optional<std::vector<quint16>> text; // Empty optional is a terminal refusal.
#ifdef RSDK_DEVELOPMENT_READER_FOCUS_V1
    ReaderFocus readerFocus=ReaderFocus::Refused;
#endif
    Id target;
    bool targetPresent=false;
    unsigned char rejection=0;
#ifdef RSDK_DEVELOPMENT_COMPOSER_V1
    Composer composer;
#endif
};
inline bool validPreEnterReply(const Frame &request,const Frame &reply) {
    if(request.kind!=PreEnterRequest)return reply.kind!=PreEnterReply;
    if(reply.kind==3)return reply.rejection==0||reply.rejection==4;
    return reply.kind==PreEnterReply&&(!reply.text||reply.text->size()==request.textRange.count);
}
inline bool validComposerReply(const Frame &request,const Frame &reply) {
    if(!isComposerRequest(request.kind))return !isComposerReply(reply.kind);
#ifdef RSDK_DEVELOPMENT_COMPOSER_V1
    if(reply.kind==3)return reply.rejection==0||reply.rejection==3||reply.rejection==4;
    if(reply.kind!=request.kind+2||!validComposerOutcome(reply.kind,reply.composer))return false;
    const auto &result=reply.composer;
    if(result.status!=0)return true;
    if(reply.facts.document!=request.facts.document||reply.facts.page!=request.facts.page||
       reply.facts.order!=request.facts.order)return false;
    if(request.kind!=12&&(result.visit!=request.composer.visit||result.range!=request.composer.range))return false;
    if(request.kind==14&&result.revision!=request.composer.revision)return false;
    if(request.kind==15&&std::find(request.composer.keys.begin(),request.composer.keys.end(),result.key)==request.composer.keys.end())return false;
    return true;
#else
    return false;
#endif
}
#ifdef RSDK_DEVELOPMENT_READER_FOCUS_V1
inline bool validReaderFocusReply(const Frame &request,const Frame &reply) {
    if(request.kind!=ReaderFocusRequest)return reply.kind!=ReaderFocusReply;
    return reply.kind==ReaderFocusReply||(reply.kind==3&&(reply.rejection==0||reply.rejection==3));
}
#endif
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
inline bool validImage(const Capture &c,quint8 procedure) {
    double dpr=0;static_assert(sizeof dpr==sizeof c.dprBits);std::memcpy(&dpr,&c.dprBits,sizeof dpr);
    if(!c.width||!c.height||
       c.width>1872||c.height>1872||quint64(c.width)*c.height>2628288||!std::isfinite(dpr)||dpr<=0||
       c.procedure!=procedure||c.sha256.size()!=32||c.png.size()<45||c.png.size()>MaxCapturePng)return false;
    const auto *p=reinterpret_cast<const uchar*>(c.png.constData());
    return c.png.left(8)==QByteArray("\x89PNG\r\n\x1a\n",8)&&qFromBigEndian<quint32>(p+8)==13&&
        c.png.mid(12,4)=="IHDR"&&qFromBigEndian<quint32>(p+16)==c.width&&qFromBigEndian<quint32>(p+20)==c.height&&
        QCryptographicHash::hash(c.png,QCryptographicHash::Sha256)==c.sha256;
}
inline bool validCapture(const Facts &before,const Capture &c) {
    return validFacts(before)&&before.order.size()<=17&&before.document==c.after.document&&
        before.page==c.after.page&&before.order==c.after.order&&validImage(c,1);
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
#ifdef RSDK_DEVELOPMENT_COMPOSER_V1
inline std::vector<quint16> composerUnits(Reader &reader,std::size_t maximum) {
    const auto count=reader.number<quint16>();std::vector<quint16> units;
    if(count>maximum||qsizetype(count)*2>reader.remaining()){reader.good=false;return units;}
    units.reserve(count);
    for(unsigned i=0;i<count;++i)units.push_back(reader.number<quint16>());
    return units;
}
inline void composerUnits(Writer &writer,const std::vector<quint16> &units) {
    writer.number<quint16>(quint16(units.size()));
    for(auto unit:units)writer.number<quint16>(unit);
}
inline bool readComposerRequest(Reader &r,Frame &f) {
    f.facts=r.facts();auto &c=f.composer;
    if(f.kind==12)return r.good;
    c.visit=r.id();c.range=r.id();
    if(f.kind==14){c.revision=r.number<qint32>();c.text=composerUnits(r,4096);if(c.revision<0)return false;}
    if(f.kind==15) {
        const auto count=r.number<quint8>();if(!count||count>4)return false;
        for(unsigned i=0;i<count;++i)c.keys.push_back(r.number<qint32>());
        if(!validComposerKeys(c.keys))return false;
    }
    return r.good;
}
inline bool writeComposerRequest(Writer &w,const Frame &f) {
    const auto &c=f.composer;
    if(!validFacts(f.facts)||(f.kind!=12&&(!validId(c.visit)||!validId(c.range))))return false;
    if(f.kind==14&&(c.revision<0||c.text.size()>4096))return false;
    if(f.kind==15&&!validComposerKeys(c.keys))return false;
    w.facts(f.facts);
    if(f.kind==12)return true;
    w.bytes+=c.visit;w.bytes+=c.range;
    if(f.kind==14){w.number<qint32>(c.revision);composerUnits(w,c.text);}
    if(f.kind==15){w.number<quint8>(quint8(c.keys.size()));for(auto key:c.keys)w.number<qint32>(key);}
    return true;
}
inline bool readComposerReply(Reader &r,Frame &f) {
    auto &c=f.composer;c.status=r.number<quint8>();c.attempts=r.number<quint8>();
    if(c.status==0) {
        f.facts=r.facts();c.visit=r.id();c.range=r.id();c.revision=r.number<qint32>();
        if(f.kind==16)c.afterRevision=r.number<qint32>();
        if(f.kind==15||f.kind==17)c.text=composerUnits(r,4096);
        if(f.kind==17) {
            c.event=r.id();c.inputSerial=r.number<quint64>();c.key=r.number<qint32>();c.count=r.number<quint16>();
            c.modifiers=r.number<quint32>();c.nativeScan=r.number<quint32>();c.nativeVirtual=r.number<quint32>();
            c.nativeModifiers=r.number<quint32>();const auto repeat=r.number<quint8>(),spontaneous=r.number<quint8>();
            if(repeat>1||spontaneous>1)return false;
            c.autoRepeat=repeat;c.spontaneous=spontaneous;c.keyText=composerUnits(r,64);
        }
    }
    return r.good&&validComposerOutcome(f.kind,c);
}
inline bool writeComposerReply(Writer &w,const Frame &f) {
    const auto &c=f.composer;
    if(!validComposerOutcome(f.kind,c))return false;
    w.number<quint8>(c.status);w.number<quint8>(c.attempts);
    if(c.status!=0)return true;
    if(!validFacts(f.facts))return false;
    w.facts(f.facts);w.bytes+=c.visit;w.bytes+=c.range;w.number<qint32>(c.revision);
    if(f.kind==16)w.number<qint32>(c.afterRevision);
    if(f.kind==15||f.kind==17)composerUnits(w,c.text);
    if(f.kind==17) {
        w.bytes+=c.event;w.number<quint64>(c.inputSerial);w.number<qint32>(c.key);w.number<quint16>(c.count);
        w.number<quint32>(c.modifiers);w.number<quint32>(c.nativeScan);w.number<quint32>(c.nativeVirtual);
        w.number<quint32>(c.nativeModifiers);w.number<quint8>(c.autoRepeat);w.number<quint8>(c.spontaneous);
        composerUnits(w,c.keyText);
    }
    return true;
}
#endif
}
inline std::optional<Frame> decode(const QByteArray &payload) {
    if(payload.size()>MaxCaptureFrame)return {};
    detail::Reader r(payload);Frame f;
    if(r.take(4)!="RSDK"||r.number<quint16>()!=4)return {};
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
        f.kind=r.number<quint8>();
#ifdef RSDK_DEVELOPMENT_ORDINARY_CLOSE_V1
#ifdef RSDK_DEVELOPMENT_FIXTURE_REOPEN_V1
        if(f.kind>9&&f.kind!=PreEnterRequest&&!isReaderFocusRequest(f.kind)&&!isComposerRequest(f.kind))return {};
        if(f.kind==9){f.facts=r.facts();if(f.facts.order.size()>16)return {};}
        else
#else
        if(f.kind>8&&f.kind!=PreEnterRequest&&!isReaderFocusRequest(f.kind)&&!isComposerRequest(f.kind))return {};
#endif
        if(f.kind==7){f.facts=r.facts();if(f.facts.order.size()>16)return {};}
        else if(f.kind==8){}
        else
#else
        if(f.kind>6&&f.kind!=PreEnterRequest&&!isReaderFocusRequest(f.kind)&&!isComposerRequest(f.kind))return {};
#endif
#ifdef RSDK_DEVELOPMENT_COMPOSER_V1
        if(isComposerRequest(f.kind)) {if(!detail::readComposerRequest(r,f))return {};}
        else
#endif
        if(f.kind==PreEnterRequest) {
            f.textRange={r.number<quint32>(),r.number<quint16>()};
            if(!validTextRange(f.textRange))return {};
        }else if(f.kind==5) {f.facts=r.facts();if(f.facts.order.size()>17)return {};}
        else if(f.kind&&f.kind!=6&&!isReaderFocusRequest(f.kind)) {
            f.facts=r.facts();f.target=r.id();
            if(f.kind<=2){for(const auto &id:f.facts.order)if(id==f.target)return {};}
            else if(!validSelection(f.facts,f.target))return {};
            if(f.kind==1&&f.facts.order.size()==MaxPages)return {};
        }
    } else if(f.message==3) {
        f.requestDigest=r.take(32);f.kind=r.number<quint8>();
#ifdef RSDK_DEVELOPMENT_ORDINARY_CLOSE_V1
#ifdef RSDK_DEVELOPMENT_FIXTURE_REOPEN_V1
        if(f.kind>11&&f.kind!=PreEnterReply&&!isReaderFocusReply(f.kind)&&!isComposerReply(f.kind))return {};
#else
        if(f.kind>10&&f.kind!=PreEnterReply&&!isReaderFocusReply(f.kind)&&!isComposerReply(f.kind))return {};
#endif
#else
        if(f.kind>8&&f.kind!=PreEnterReply&&!isReaderFocusReply(f.kind)&&!isComposerReply(f.kind))return {};
#endif
#ifdef RSDK_DEVELOPMENT_READER_FOCUS_V1
        if(f.kind==ReaderFocusReply) {
            const auto value=r.number<quint8>();
            if(value>quint8(ReaderFocus::OutsideScope))return {};
            f.readerFocus=ReaderFocus(value);
        }else
#endif
#ifdef RSDK_DEVELOPMENT_COMPOSER_V1
        if(isComposerReply(f.kind)) {if(!detail::readComposerReply(r,f))return {};}
        else
#endif
        if(f.kind==PreEnterReply) {
            const auto status=r.number<quint8>();
            if(status==0) {
                const auto count=r.number<quint16>();
                if(count>MaxPreEnterUnits||r.remaining()!=qsizetype(count)*2)return {};
                auto &units=f.text.emplace();
                units.reserve(count);
                for(unsigned n=0;n<count;++n)units.push_back(r.number<quint16>());
            }else if(status!=1)return {};
        }else if(f.kind==7||f.kind==8) {
            auto &c=f.capture;
            if(f.kind==7){f.facts=r.facts();c.after=r.facts();}
            c.width=r.number<quint32>();c.height=r.number<quint32>();c.dprBits=r.number<quint64>();
            c.procedure=r.number<quint8>();c.sha256=r.take(32);
            const auto count=r.number<quint32>();if(count>quint32(MaxCapturePng)||qsizetype(count)!=r.remaining())return {};
            c.png=r.take(count);if(f.kind==7?!validCapture(f.facts,c):!validImage(c,2))return {};
        } else if(f.kind==0||f.kind==5)f.facts=r.facts();
        else if(f.kind==1) {
            f.facts.document=r.id();f.target=r.id();f.facts.page=f.target;f.facts.order=r.order();
            if(!validFacts(f.facts))return {};
        } else if(f.kind==2||f.kind==6) {
            f.facts=r.facts();const auto present=r.number<quint8>();if(present>1)return {};f.targetPresent=present;
        } else if(f.kind==3) {f.rejection=r.number<quint8>();if(f.rejection>4)return {};}
    }
    if(!r.good||r.remaining()||(payload.size()>MaxFrame&&!(f.message==3&&(f.kind==7||f.kind==8))))return {};
    return f;
}
inline QByteArray encode(const Frame &f) {
    // Reject oversized fields before appending any caller-controlled collection.
    if(f.runtime.size()!=140||f.client.size()!=16||f.server.size()!=16||f.operation.size()!=16||
       f.facts.order.size()>MaxPages||f.capture.png.size()>MaxCapturePng||
       (f.text&&f.text->size()>MaxPreEnterUnits))return {};
#ifdef RSDK_DEVELOPMENT_COMPOSER_V1
    if(f.composer.text.size()>4096||f.composer.keyText.size()>64||f.composer.keys.size()>4)return {};
#endif
    if((f.message==2&&f.kind&&f.kind!=6&&f.kind!=8&&f.kind!=PreEnterRequest&&!isReaderFocusRequest(f.kind))||
       (f.message==3&&(f.kind<3||f.kind==5||f.kind==6))) {
        if(!validFacts(f.facts))return {};
    }
    if(((f.message==2&&f.kind&&f.kind<5)||(f.message==3&&f.kind==1))&&!validId(f.target))return {};
    if(f.message==3&&f.kind==7&&!validCapture(f.facts,f.capture))return {};
    if(f.message==3&&f.kind==8&&!validImage(f.capture,2))return {};
    detail::Writer w;w.bytes="RSDK";w.number<quint16>(4);w.bytes+=f.runtime;
    w.bytes+=f.client;w.bytes+=f.server;w.number<quint64>(f.sequence);w.bytes+=f.operation;w.number<quint8>(f.message);
    if(f.message==2) {
        w.number<quint8>(f.kind);
#ifdef RSDK_DEVELOPMENT_COMPOSER_V1
        if(isComposerRequest(f.kind)){if(!detail::writeComposerRequest(w,f))return {};}
        else
#endif
        if(f.kind==PreEnterRequest) {
            if(!validTextRange(f.textRange))return {};
            w.number<quint32>(f.textRange.offset);w.number<quint16>(f.textRange.count);
        }else if(f.kind&&f.kind!=6&&f.kind!=8&&!isReaderFocusRequest(f.kind)){w.facts(f.facts);if(f.kind!=5&&f.kind!=7&&f.kind!=9)w.bytes+=f.target;}
    } else if(f.message==3) {
        if(f.requestDigest.size()!=32)return {};
        w.bytes+=f.requestDigest;w.number<quint8>(f.kind);
#ifdef RSDK_DEVELOPMENT_READER_FOCUS_V1
        if(f.kind==ReaderFocusReply)w.number<quint8>(quint8(f.readerFocus));
        else
#endif
#ifdef RSDK_DEVELOPMENT_COMPOSER_V1
        if(isComposerReply(f.kind)){if(!detail::writeComposerReply(w,f))return {};}
        else
#endif
        if(f.kind==PreEnterReply) {
            w.number<quint8>(f.text?0:1);
            if(f.text) {
                w.number<quint16>(quint16(f.text->size()));
                for(const auto unit:*f.text)w.number<quint16>(unit);
            }
        }else if(f.kind==7||f.kind==8) {
            const auto &c=f.capture;
            if(f.kind==7){w.facts(f.facts);w.facts(c.after);}
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
    bool idle()const{return !poisoned_&&bytes_.isEmpty();}
    void close(){poisoned_=true;bytes_.clear();}
};
}
