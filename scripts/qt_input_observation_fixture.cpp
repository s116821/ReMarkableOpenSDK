#include "qt_page_facts_entry.h"
#include <QQmlApplicationEngine>
#include <QTemporaryDir>
#include <QDir>
#include <cstdio>
using namespace qml_access;
static bool put(const QString &path,const QByteArray &value) {
    QFile file(path);if(!file.open(QIODevice::WriteOnly|QIODevice::NewOnly))return false;
    if(!file.setPermissions(QFile::ReadOwner|QFile::WriteOwner))return false;
    return file.write(value)==value.size();
}
namespace qml_access {
struct InputObservationFixtureAccess {
    static bool filter(FactsEntry &entry,QObject *object,QEvent *event){return entry.eventFilter(object,event);}
    static InputObservationStore &store(FactsEntry &entry){return entry.observation_;}
    static bool capturing(FactsEntry &entry){return entry.depth_>0 && entry.acceptedAt_>=0 && !entry.done_;}
};
}
int main(int argc,char **argv) {
    QGuiApplication app(argc,argv);app.setQuitOnLastWindowClosed(false);
    if(argc!=2)return 2;
    const QByteArray mode(argv[1]);
    const auto drain=[]{for(int i=0;i<40;++i)QCoreApplication::processEvents();};
    QTemporaryDir parent;const QString nonce=QStringLiteral("0123456789abcdef0123456789abcdef");
    const auto root=parent.path()+"/rmb-qt-probe-"+nonce;
    if(!QDir().mkdir(root) || ::chmod(root.toUtf8().constData(),0700))return 3;
    if(!put(root+"/owner",nonce.toLatin1()) || !put(root+"/attempt.identity",QByteArray::number(::getpid())+' '+FactsEntry::processStart()+'\n'))return 4;
    QQmlApplicationEngine engine;
    engine.loadData("import QtQuick\nimport QtQuick.Window\nWindow {visible:true;width:200;height:200;color:'white'}");
    if(engine.rootObjects().size()!=1)return 5;
    auto *window=qobject_cast<QQuickWindow *>(engine.rootObjects()[0]);window->requestActivate();
    drain();
    if(mode=="frames"){
        InputObservationStore store;
        QPointingDevice first("fixture-first",std::numeric_limits<qint64>::min(),QInputDevice::DeviceType::TouchScreen,QPointingDevice::PointerType::Finger,QInputDevice::Capability::Position,4,0);
        QPointingDevice second("fixture-second",std::numeric_limits<qint64>::max(),QInputDevice::DeviceType::Mouse,QPointingDevice::PointerType::Generic,QInputDevice::Capability::Position,1,3);
        QEventPoint tp(7,QEventPoint::State::Pressed,QPointF(110,120),QPointF(210,220));
        QTouchEvent touch(QEvent::TouchBegin,&first,Qt::NoModifier,{tp});
        store.observe(window,&touch,window,1);store.observe(window,&touch,window,2);
        QMouseEvent mouse(QEvent::MouseMove,QPointF(10,20),QPointF(110,120),QPointF(210,220),Qt::NoButton,Qt::NoButton,Qt::NoModifier,&second);
        store.observe(window,&mouse,window,3);
        const auto &p=store.records[0].positions[0];
        if(p.x!=tp.position().x() || p.y!=tp.position().y() || p.sceneX!=110 || p.sceneY!=120 || p.globalX!=210 || p.globalY!=220 || p.validMask!=7)return 23;
        const auto &m=store.records[2].positions[0];
        if(m.x!=10 || m.y!=20 || m.sceneX!=110 || m.sceneY!=120 || m.globalX!=210 || m.globalY!=220 || m.id!=0 || m.state!=int(Qt::NoButton))return 24;
        if(!store.records[0].devicePresent || store.records[0].deviceSystemId!=first.systemId() || store.records[1].deviceSystemId!=first.systemId() || store.records[2].deviceSystemId!=second.systemId())return 25;
        auto events=store.json()["events"].toArray();
        if(events[0].toObject()["device_system_id"].toString()!=QStringLiteral("-9223372036854775808") || events[2].toObject()["device_system_id"].toString()!=QStringLiteral("9223372036854775807"))return 26;
        QTouchEvent noDevice(QEvent::TouchCancel,nullptr,Qt::NoModifier,QList<QEventPoint>{});
        store.observe(window,&noDevice,window,4);
        const auto absent=store.json()["events"].toArray()[3].toObject();
        if(absent["device_present"].toBool() || !absent["device_system_id"].isNull() || !absent["device_type"].isNull())return 27;
        for(int mask=0;mask<8;++mask){
            const double nan=std::numeric_limits<double>::quiet_NaN(),inf=std::numeric_limits<double>::infinity();
            InputObservationStore masks;masks.size=1;masks.records[0].points=1;
            masks.records[0].positions[0]=InputObservationStore::point(19,3,
                mask&1 ? QPointF(1e308,-8388608):QPointF(nan,1),
                mask&2 ? QPointF(12,13):QPointF(1,inf),
                mask&4 ? QPointF(22,23):QPointF(-inf,1));
            const auto point=masks.json()["events"].toArray()[0].toObject()["points"].toArray()[0].toObject();
            if(point["valid_mask"].toInt()!=mask || point["id"].toInt()!=19 || point["state"].toInt()!=3 || masks.pointOverflow)return 28;
            const char *keys[]={"x","y","scene_x","scene_y","global_x","global_y"};
            for(int frame=0;frame<3;++frame)for(int axis=0;axis<2;++axis)
                if(point[keys[frame*2+axis]].isNull()==bool(mask&(1<<frame)))return 29;
            if((mask&1) && (point["x"].toDouble()!=1e308 || point["y"].toDouble()!=-8388608))return 30;
        }
        for(int i=store.size;i<64;++i)store.observe(window,&mouse,window,5);
        if(store.size!=64 || store.recordOverflow)return 31;
        store.observe(window,&mouse,window,6);if(store.size!=64 || !store.recordOverflow || store.counts[6]!=62)return 32;
        for(auto &record:store.records){record.devicePresent=true;record.deviceSystemId=std::numeric_limits<qint64>::min();record.deviceType=std::numeric_limits<int>::max();record.points=4;
            for(auto &point:record.positions)point=InputObservationStore::point(std::numeric_limits<int>::min(),std::numeric_limits<int>::max(),{1e308,-1e308},{1e308,-1e308},{1e308,-1e308});}
        const auto bytes=inputCompletionBytes(store.json());const auto bounded=QJsonDocument::fromJson(bytes).object();
        if(bytes.isEmpty() || bytes.size()>8192 || !bounded["output_truncated"].toBool() || bounded["events"].toArray().isEmpty() || bounded["events"].toArray().size()>=64)return 33;
        for(const auto &value:bounded["events"].toArray())if(value.toObject()["points"].toArray().size()!=4)return 34;
    }
    if(mode=="caps"){
        InputObservationStore store;
        QList<QEventPoint> points;
        for(int i=0;i<5;++i)points.append(QEventPoint(i,QEventPoint::State::Pressed,QPointF(10,20),QPointF(10,20)));
        QTouchEvent touch(QEvent::TouchBegin,QPointingDevice::primaryPointingDevice(),Qt::NoModifier,points);
        touch.setAccepted(true);store.observe(window,&touch,window,10);
        if(!touch.isAccepted() || store.size!=1 || store.records[0].points!=4 || !store.pointOverflow)return 16;
        store.counts[0]=std::numeric_limits<quint32>::max();store.observe(window,&touch,window,11);
        if(!store.countOverflow || store.counts[0]!=std::numeric_limits<quint32>::max())return 17;
        QMouseEvent duplicate(QEvent::MouseButtonPress,QPointF(10,20),QPointF(10,20),Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);
        QQuickItem retainedItem;retainedItem.setParentItem(window->contentItem());
        QObject unknown;QQuickWindow other;
        store.observe(&retainedItem,&duplicate,window,12);store.observe(&unknown,&duplicate,window,12);store.observe(&other,&duplicate,window,12);
        if(store.counts[4]!=3 || store.records[2].relationship!=2 || store.records[3].relationship!=0 || store.records[4].relationship!=3)return 21;
        store.size=64;
        for(auto &record:store.records){record.points=4;for(auto &point:record.positions){point.x=1e308;point.y=-1e308;}}
        const auto bounded=inputCompletionBytes(store.json());
        const auto object=QJsonDocument::fromJson(bounded).object();
        if(bounded.isEmpty() || bounded.size()>8192 || !object["output_truncated"].toBool() || object["events"].toArray().size()>=64)return 18;
        QFile png(root+"/cap-fixture.png");if(!png.open(QIODevice::WriteOnly))return 19;
        InputImageOutput output(png.handle());QByteArray bytes(8388608,'x');
        if(output.write(bytes)!=bytes.size() || output.write("x",1)!=-1 || output.bytes()!=8388608)return 20;
    }
    FactsEntryConfig config;config.nonce=nonce;config.directory=root;
    config.facts={QStringLiteral("00000000-0000-4000-8000-000000000001"),{QStringLiteral("00000000-0000-4000-8000-000000000002")},6,5000};
    config.setupBudgetMs=120000;config.developmentSetup120=true;config.developmentInputObservation=true;
    config.setupSelection=QStringLiteral("main-dev-input-observation-120s");
    qint64 clock=0;int callbacks=0,captureClockCalls=0;bool boundaryChecked=false;FactsEntryResult result;
    FactsEntry *entryPointer=nullptr;
    FactsEntry entry(&app,config,[&](FactsEntryResult value){++callbacks;result=value;},[&]{
        if(entryPointer && InputObservationFixtureAccess::capturing(*entryPointer) && ++captureClockCalls==2 && mode.startsWith("completion-")) {
            QMetaObject::invokeMethod(&app,[&]{
                boundaryChecked=!QFileInfo::exists(root+"/input-observation-complete.json");
                if(mode=="completion-window-loss")window->hide();
                if(mode=="completion-destroy-window"){delete window;window=nullptr;}
                if(mode=="completion-cancel")entryPointer->cancel();
                if(mode=="completion-deadline")clock=5000;
            },Qt::QueuedConnection);
        }
        return clock;
    });
    entryPointer=&entry;
    if(mode=="inactive-window")window->hide();
    entry.start();drain();
    if(mode=="inactive-window"){
        if(callbacks!=1 || QFileInfo::exists(root+"/input-observation-ready") || QFileInfo::exists(root+"/input-observation-complete.json"))return 22;
        std::printf("observation-inactive-window: PASS (no ready marker; synthetic Qt/offscreen)\n");return 0;
    }
    QFile ready(root+"/input-observation-ready");if(!ready.open(QIODevice::ReadOnly))return 6;
    const auto fields=ready.readAll().trimmed().split(' ');if(fields.size()!=9)return 7;
    QMouseEvent event(QEvent::MouseButtonPress,QPointF(90,100),QPointF(90,100),Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);
    const bool accepted=mode=="accepted-true";event.setAccepted(accepted);
    for(int i=0;i<(mode=="overflow" ? 70:1);++i)
        if(InputObservationFixtureAccess::filter(entry,window,&event) || event.isAccepted()!=accepted)return 8;
    auto token=fields.mid(0,5).join(' ')+" end-input-observation 120000 main-dev-input-observation-120s\n";
    if(mode=="wrong-purpose")token.replace("end-input-observation","read-facts");
    if(mode=="oversize")token=QByteArray(257,'x');
    if(mode=="setup-boundary")clock=120000;
    if(mode=="window-loss")window->hide();
    if(mode=="restoring")put(root+"/restore.claim","closed");
    if(mode=="cross-facts")put(root+"/facts-request","read-facts");
    if(!put(root+"/input-observation-end",token))return 9;
    drain();
    const bool success=mode=="good" || mode=="overflow" || mode=="duplicate" || mode=="caps" || mode=="frames" || mode=="accepted-true";
    if(mode=="duplicate"){entry.start();drain();}
    QFile complete(root+"/input-observation-complete.json");
    if(success){
        if(!complete.open(QIODevice::ReadOnly))return 10;
        const auto bytes=complete.readAll();const auto json=QJsonDocument::fromJson(bytes).object();
        if(json["evidence_profile"].toString()!=QStringLiteral("device-frames-v1") || bytes.size()>8192 || !json["gui_callback_completed"].toBool() || json["native_authority"].toBool(true) ||
           json["render_authority"].toBool(true) || json["ui_acknowledged"].toBool(true) ||
           json["counts"].toArray()[4].toInt()!=(mode=="overflow" ? 70:1) ||
           json["record_overflow"].toBool()!=(mode=="overflow"))return 11;
    }else if(complete.exists())return 12;
    if(mode.startsWith("completion-") && !boundaryChecked)return 15;
    if(callbacks!=1 || result.observed || QFileInfo::exists(root+"/facts-waiting") || QFileInfo::exists(root+"/diagnostics.json"))return 13;
    auto &store=InputObservationFixtureAccess::store(entry);const int before=store.size;
    InputObservationFixtureAccess::filter(entry,window,&event);if(store.size!=before)return 14;
    std::printf("observation-%s: PASS (%s; synthetic Qt/offscreen, no tablet)\n",argv[1],qPrintable(result.stage));return 0;
}
