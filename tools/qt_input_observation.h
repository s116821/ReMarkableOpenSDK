#pragma once
#include <QTouchEvent>
#include <QMouseEvent>
#include <QQuickItem>
#include <QQuickWindow>
#include <QJsonArray>
#include <QJsonDocument>
#include <QImageWriter>
#include <array>
#include <cmath>
#include <limits>
#include <unistd.h>

namespace qml_access {
// Fixed development evidence storage; no I/O or accepted-state changes here.
struct InputObservationStore {
    struct Point {
        int id=0,state=0,validMask=0;
        double x=0,y=0,sceneX=0,sceneY=0,globalX=0,globalY=0;
    };
    static Point point(int id,int state,QPointF local,QPointF scene,QPointF global) {
        Point p;p.id=id;p.state=state;
        const auto frame=[&](QPointF value,int bit,double &x,double &y){
            if(std::isfinite(value.x()) && std::isfinite(value.y())){p.validMask|=bit;x=value.x();y=value.y();}
        };
        frame(local,1,p.x,p.y);frame(scene,2,p.sceneX,p.sceneY);frame(global,4,p.globalX,p.globalY);
        return p;
    }
    struct Record {
        qint64 ms=0,deviceSystemId=0; quint64 timestamp=0;
        bool devicePresent=false;int deviceType=0;
        int type=0,relationship=0,source=0,buttons=0,points=0;
        std::array<Point,4> positions{};
    };
    std::array<Record,64> records{};
    std::array<quint32,7> counts{};
    int size=0;
    bool sealed=false,recordOverflow=false,pointOverflow=false,countOverflow=false;
    void observe(QObject *object,QEvent *event,QQuickWindow *window,qint64 ms) {
        if (sealed || !window) return;
        int index=-1;
        switch(event->type()) {
        case QEvent::TouchBegin:index=0;break; case QEvent::TouchUpdate:index=1;break;
        case QEvent::TouchEnd:index=2;break; case QEvent::TouchCancel:index=3;break;
        case QEvent::MouseButtonPress:index=4;break; case QEvent::MouseButtonRelease:index=5;break;
        case QEvent::MouseMove:index=6;break; default:return;
        }
        if (counts[index]==std::numeric_limits<quint32>::max()) countOverflow=true; else ++counts[index];
        if (size==int(records.size())) { recordOverflow=true;return; }
        auto &r=records[size++];r.ms=ms;r.type=int(event->type());
        if (object==window) r.relationship=1;
        else if (auto *item=qobject_cast<QQuickItem *>(object)) r.relationship=item->window()==window ? 2:3;
        else r.relationship=qobject_cast<QWindow *>(object) ? 3:0;
        const auto *device=static_cast<QPointerEvent *>(event)->pointingDevice();
        if(device){r.devicePresent=true;r.deviceSystemId=device->systemId();r.deviceType=int(device->type());}
        const auto add=[&](int id,int state,QPointF local,QPointF scene,QPointF global){
            r.positions[r.points++]=point(id,state,local,scene,global);
        };
        if (index<4) {
            const auto *touch=static_cast<QTouchEvent *>(event);r.timestamp=touch->timestamp();
            const auto &points=touch->points();
            if (points.size()>4) pointOverflow=true;
            for (int i=0;i<qMin(qsizetype(4),points.size());++i) add(points[i].id(),int(points[i].state()),points[i].position(),points[i].scenePosition(),points[i].globalPosition());
        } else {
            const auto *mouse=static_cast<QMouseEvent *>(event);r.timestamp=mouse->timestamp();
            r.source=int(mouse->source());r.buttons=int(mouse->buttons());add(0,int(mouse->button()),mouse->position(),mouse->scenePosition(),mouse->globalPosition());
        }
    }
    QJsonObject json() const {
        QJsonArray totals,events;
        for (auto count:counts) totals.append(double(count));
        for (int i=0;i<size;++i) {
            const auto &r=records[i];QJsonArray points;
            for (int p=0;p<r.points;++p) {
                const auto &v=r.positions[p];
                const auto coordinate=[&](int bit,double value){return v.validMask&bit ? QJsonValue(value):QJsonValue(QJsonValue::Null);};
                points.append(QJsonObject{{"id",v.id},{"state",v.state},{"valid_mask",v.validMask},
                    {"x",coordinate(1,v.x)},{"y",coordinate(1,v.y)},
                    {"scene_x",coordinate(2,v.sceneX)},{"scene_y",coordinate(2,v.sceneY)},
                    {"global_x",coordinate(4,v.globalX)},{"global_y",coordinate(4,v.globalY)}});
            }
            events.append(QJsonObject{{"ms",r.ms},{"timestamp",QString::number(r.timestamp)},
                {"device_present",r.devicePresent},
                {"device_system_id",r.devicePresent ? QJsonValue(QString::number(r.deviceSystemId)):QJsonValue(QJsonValue::Null)},
                {"device_type",r.devicePresent ? QJsonValue(r.deviceType):QJsonValue(QJsonValue::Null)},
                {"type",r.type},{"relationship",r.relationship},{"source",r.source},{"buttons",r.buttons},{"points",points}});
        }
        return {{"counts",totals},{"events",events},{"record_overflow",recordOverflow},
            {"point_overflow",pointOverflow},{"count_overflow",countOverflow},{"output_truncated",false}};
    }
};
// One capped private PNG stream; the caller owns descriptor and scope checks.
class InputImageOutput final:public QIODevice {
public:
    explicit InputImageOutput(int fd):fd_(fd){open(QIODevice::WriteOnly);}
    qint64 bytes() const{return bytes_;}
protected:
    qint64 readData(char *,qint64) override{return -1;}
    qint64 writeData(const char *data,qint64 length) override {
        if (length<0 || length>8388608-bytes_) return -1;
        const auto written=::write(fd_,data,size_t(length));
        if (written!=length) return -1;
        bytes_+=written;return written;
    }
private:int fd_;qint64 bytes_=0;
};
inline QByteArray inputCompletionBytes(QJsonObject object) {
    auto events=object.value("events").toArray();
    auto bytes=QJsonDocument(object).toJson(QJsonDocument::Compact);
    while (bytes.size()>8192 && !events.isEmpty()) {
        events.removeLast();object["events"]=events;object["output_truncated"]=true;
        bytes=QJsonDocument(object).toJson(QJsonDocument::Compact);
    }
    return bytes.size()<=8192 ? bytes:QByteArray{};
}
}
