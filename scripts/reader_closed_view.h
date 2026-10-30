#pragma once
// Fresh sampled UI closure, not a storage fence or an ownership lease.
#include "reader_fixture_navigation.h"

namespace reader_closed_view {
inline constexpr char MainContext[]="qrc:/qml/device/view/main/MainView.qml";
struct Result {
    bool observed=false,compositionObserved=false,loaderReady=false,documentNull=false;
    QString refusal;
    static constexpr bool nativeAuthority=false,durableAuthority=false;
};
namespace detail {
class Sample final {
    QPointer<QQuickItem> main;
    QPointer<QQmlEngine> engine;
    QPointer<QQuickWindow> window;
    reader_selected::detail::Epoch epoch;
    QList<QPointer<QObject>> retained;
    QList<const QMetaObject *> metadata;
    QList<QPair<QObject *,int>> watched;
    const std::function<bool()> &guard;
    QString failure;
    bool no(const char *s){if(failure.isEmpty())failure=QString::fromLatin1(s);return false;}
    bool basic(){
        if(!failure.isEmpty())return false;
        if(!main||!engine||!window||epoch.value||QCoreApplication::closingDown())return no("closed-context");
        for(qsizetype i=0;i<retained.size();++i){
            const auto &p=retained[i];
            if(!p||p->thread()!=QThread::currentThread())return no("closed-lifetime");
            if(p->metaObject()!=metadata[i])return no("closed-metadata");
            const auto scope=qobject_cast<QQmlContext *>(p.data());
            if(scope&&(!scope->isValid()||scope->engine()!=engine))return no("closed-context");
        }
        return true;
    }
    bool allowed(){return basic()&&guard&&guard()&&basic();}
    bool retain(QObject *p){
        QPointer<QObject> pin=p;
        if(!pin||pin->thread()!=QThread::currentThread())return no("closed-lifetime");
        if(retained.contains(pin))return true;
        if(retained.size()>=192)return no("closed-object-cap");
        retained.append(pin);
        metadata.append(pin->metaObject());
        if(!QObject::connect(pin,&QObject::destroyed,&epoch,&reader_selected::detail::Epoch::changed,Qt::DirectConnection))return no("closed-notification");
        return bool(pin);
    }
    bool signal(QObject *p,const QMetaMethod &s){
        if(!s.isValid())return no("closed-notification");
        const QPair<QObject *,int> key(p,s.methodIndex());
        if(watched.contains(key))return true;
        const int slot=epoch.metaObject()->indexOfSlot("changed()");
        if(!QObject::connect(p,s,&epoch,epoch.metaObject()->method(slot),Qt::DirectConnection))return no("closed-notification");
        watched.append(key);return true;
    }
    QMetaProperty watch(QObject *p,const char *name){
        QPointer<QObject> pin=p;
        if(!retain(pin)||!allowed())return {};
        const QMetaObject *meta=pin->metaObject();
        if(meta->propertyCount()>512){no("closed-property-cap");return {};}
        const int index=meta->indexOfProperty(name);
        if(index<0){no("closed-property-missing");return {};}
        const QMetaProperty property=meta->property(index);
        if(!property.isReadable()||(!property.isConstant()&&(!property.hasNotifySignal()||!signal(pin,property.notifySignal())))){
            no("closed-notification");return {};
        }
        if(!allowed()||!pin||pin->metaObject()!=meta){no("closed-metadata");return {};}
        return property;
    }
    QVariant read(QObject *p,const char *name,QMetaType type={},bool pointer=false){
        QPointer<QObject> pin=p;
        const auto prop=watch(pin,name);
        if(!prop.isValid()||!pin)return {};
        const QMetaObject *meta=pin->metaObject();
        if(pointer?!prop.metaType().flags().testFlag(QMetaType::PointerToQObject):
                    (type.isValid()&&prop.metaType()!=type)){no("closed-property-type");return {};}
        const QVariant value=prop.read(pin);
        // Protect a borrowed getter return before guard()/permit can run.
        QPointer<QObject> returned;
        if(pointer&&value.isValid()&&value.metaType().flags().testFlag(QMetaType::PointerToQObject)){
            returned=value.value<QObject *>();
            if(returned&&!retain(returned))return {};
        }
        if(!allowed()||!pin||pin->metaObject()!=meta||!value.isValid()||value.metaType()!=prop.metaType()){
            no("closed-property-read");return {};
        }
        return value;
    }
    bool visual(QQuickItem *p){
        QPointer<QQuickItem> pin=p;
        if(!retain(pin)||!allowed())return false;
        for(const char *name:{"parent","visible","enabled"})if(!watch(pin,name).isValid())return false;
        for(const char *name:{"childrenChanged()","windowChanged(QQuickWindow*)"}){
            if(!pin)return no("closed-lifetime");
            const auto *meta=pin->metaObject();const int i=meta->indexOfSignal(name);
            if(i<0||!signal(pin,meta->method(i)))return no("closed-notification");
        }
        return allowed()&&pin&&pin->window()==window&&qmlEngine(pin)==engine;
    }
public:
    Sample(QQuickItem *m,const std::function<bool()> &g):main(m),engine(qmlEngine(m)),window(m?m->window():nullptr),guard(g){}
    Result run(){
        Result r;
        const auto refused=[&]{r.refusal=failure.isEmpty()?QStringLiteral("closed-state"):failure;return r;};
        if(!retain(main)||!retain(engine)||!retain(window)||!visual(main))return refused();
        // A component root's qmlContext can be its instantiation context.
        // Its immediate visual children expose the component's lexical scope.
        QList<QPointer<QQuickItem>> seeds{main};
        for(QQuickItem *child:main->childItems())seeds.append(child);
        if(seeds.size()>65||!allowed())return refused();
        QPointer<QQmlContext> scope;
        QPointer<QObject> selectedLoader;
        for(const auto &seed:seeds){
            if(!seed||!visual(seed))return refused();
            QPointer<QQmlContext> candidate=qmlContext(seed);
            for(int n=0;candidate&&n<16;++n){
                if(!retain(candidate)||!allowed()||!candidate->isValid()||candidate->engine()!=engine)return refused();
                if(candidate->baseUrl()==QUrl(QString::fromLatin1(MainContext))){
                    QPointer<QObject> boundRoot=candidate->objectForName(QStringLiteral("root"));
                    if(boundRoot&&!retain(boundRoot))return refused();
                    QPointer<QObject> boundLoader=candidate->objectForName(QStringLiteral("documentView"));
                    if(boundLoader&&!retain(boundLoader))return refused();
                    if(!allowed())return refused();
                    if(boundRoot==main&&boundLoader){
                        if(selectedLoader&&selectedLoader!=boundLoader)return refused();
                        scope=candidate;selectedLoader=boundLoader;
                    }
                }
                QPointer<QQmlContext> next=candidate->parentContext();
                if(next&&!retain(next))return refused();
                if(!allowed())return refused();
                candidate=next;
            }
            if(candidate)return refused();
        }
        if(!scope||!allowed()||scope->baseUrl()!=QUrl(QString::fromLatin1(MainContext)))return refused();
        QPointer<QObject> lexicalRoot=scope->objectForName(QStringLiteral("root"));
        if(lexicalRoot&&!retain(lexicalRoot))return refused();
        QPointer<QObject> loaderObject=scope->objectForName(QStringLiteral("documentView"));
        if(loaderObject&&!retain(loaderObject))return refused();
        if(!allowed()||lexicalRoot!=main||!loaderObject)return refused();
        QPointer<QQuickItem> loader=qobject_cast<QQuickItem *>(loaderObject.data());
        if(!loader||!visual(loader))return refused();
        QPointer<QQuickItem> ancestor=loader;
        for(int n=0;ancestor&&ancestor!=main&&n<8;++n){
            QPointer<QQuickItem> next=ancestor->parentItem();
            if(!next||!visual(next))return refused();
            ancestor=next;
        }
        if(ancestor!=main||!allowed())return refused();
        const auto statusProperty=watch(loader,"status");
        if(!statusProperty.isValid()||!statusProperty.isEnumType())return refused();
        bool readyKnown=false;const int ready=statusProperty.enumerator().keyToValue("Ready",&readyKnown);
        if(!readyKnown)return refused();
        const auto status=read(loader,"status",statusProperty.metaType());
        const auto async=read(loader,"asynchronous",QMetaType::fromType<bool>());
        const auto visible=read(loader,"visible",QMetaType::fromType<bool>());
        const auto active=read(main,"documentViewActive",QMetaType::fromType<bool>());
        const auto itemValue=read(loader,"item",{},true);
        QPointer<QQuickItem> reader=qobject_cast<QQuickItem *>(itemValue.value<QObject *>());
        if(!status.isValid()||status.toInt()!=ready||!async.isValid()||async.toBool()||!visible.isValid()||visible.toBool()||
           !active.isValid()||active.toBool()||!reader||!visual(reader)||reader->parentItem()!=loader)return refused();
        const auto loaded=read(reader,"documentLoaded",QMetaType::fromType<bool>());
        const auto document=read(reader,"document",{},true);
        if(!loaded.isValid()||loaded.toBool()||!document.isValid()||document.value<QObject *>()||!allowed())return refused();
        if(!scope||!scope->isValid()||scope->engine()!=engine||scope->objectForName(QStringLiteral("root"))!=main||
           scope->objectForName(QStringLiteral("documentView"))!=loader||!allowed())return refused();
        r.observed=r.loaderReady=r.documentNull=true;return r;
    }
};
}
inline Result observe(QQuickWindow *window,const std::function<bool()> &permit,int budgetMs=1000){
    Result result;
    const auto navigation=reader_selected::inspectLegacyFixture(window,[&](QQuickItem *main,QQmlContext *,const std::function<bool()> &guard){
        result=detail::Sample(main,guard).run();return result.observed;
    },permit,budgetMs);
    result.compositionObserved=navigation.compositionObserved;
    if(!navigation.compositionObserved){result.observed=false;if(result.refusal.isEmpty())result.refusal=navigation.refusal;}
    return result;
}
} // namespace reader_closed_view
