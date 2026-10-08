#pragma once
#include "qt_page_owner.h"
#include <QSet>

namespace qml_access {
struct FocusChainDiagnostics {
    qint64 items=-1;
    bool complete=false;
    const char *failure=nullptr;
};
struct FocusSceneFunnel {
    qint64 engine=0,sceneClass=0,pageId=0,pageIdChanged=0,documentWrapperChanged=0,pass=0;
};
inline bool focusSceneCandidate(QQuickItem *node,QQmlEngine *engine,FocusSceneFunnel *funnel=nullptr) {
    // First failure only; preserve the original metadata predicate short circuit.
    if(qmlEngine(node)!=engine){if(funnel)++funnel->engine;return false;}
    if(!classInherits(node,"SceneView")){if(funnel)++funnel->sceneClass;return false;}
    if(!propertyType(node,"pageId",QMetaType::fromType<QString>())){if(funnel)++funnel->pageId;return false;}
    if(node->metaObject()->indexOfSignal("pageIdChanged()")<0){if(funnel)++funnel->pageIdChanged;return false;}
    if(node->metaObject()->indexOfSignal("documentWrapperChanged()")<0){if(funnel)++funnel->documentWrapperChanged;return false;}
    if(funnel)++funnel->pass;
    return true;
}
// Entry-owned observers; callbacks only latch, never sample or traverse.
struct FocusOwnerGuard {
    QPointer<QQuickWindow> window;
    QPointer<QQuickItem> root,anchor;
    QList<QPointer<QQuickItem>> chain;
    std::function<void()> invalidate;
    QPointer<QObject> context;
    QList<QMetaObject::Connection> connections;
    bool retain(QMetaObject::Connection connection){if(!connection)return false;connections.append(connection);return true;}
    void disconnect(){for(const auto &connection:connections)QObject::disconnect(connection);connections.clear();}
    bool watchWindow(QQuickWindow *value) {
        window=value;
        const auto changed=[mark=invalidate]{mark();};
        return context && window &&
            retain(QObject::connect(value,&QQuickWindow::activeFocusItemChanged,context,changed)) &&
            retain(QObject::connect(value,&QObject::destroyed,context,changed)) &&
            retain(QObject::connect(value,&QWindow::activeChanged,context,changed)) &&
            retain(QObject::connect(value,&QWindow::visibleChanged,context,changed));
    }
    bool watchItem(QQuickItem *value) {
        const auto changed=[mark=invalidate]{mark();};
        return context && value &&
            retain(QObject::connect(value,&QObject::destroyed,context,changed)) &&
            retain(QObject::connect(value,&QQuickItem::parentChanged,context,changed)) &&
            retain(QObject::connect(value,&QQuickItem::windowChanged,context,changed)) &&
            retain(QObject::connect(value,&QQuickItem::activeFocusChanged,context,changed));
    }
    bool endpoints() const {
        return window && root && anchor && window->contentItem()==root &&
            window->activeFocusItem()==anchor;
    }
};
inline const char *findFocusPageOwner(QQmlEngine *engine,const std::function<bool()> &progress,
    FocusOwnerGuard &guard, PageOwner &out, PageOwnerDiagnostics &d, FocusChainDiagnostics &c,
    FocusSceneFunnel *funnel=nullptr) {
    out={}; c.items=0;
    const QPointer<QQmlEngine> producer=engine;
    const auto local=[&](const char *label){
        if(!progress())return "open-context-lost";
        c.failure=label;return "open-focus-chain-refused";
    };
    if(!progress())return "open-context-lost";
    if(!engine || QThread::currentThread()!=engine->thread())return local("item-context");
    const QPointer<QQuickWindow> window=qobject_cast<QQuickWindow *>(QGuiApplication::focusWindow());
    if(!progress())return "open-context-lost";
    if(!window || !window->isActive() || !window->isVisible())return local("anchor-unavailable");
    if(!guard.watchWindow(window))return local("observer-unavailable");
    if(!progress())return "open-context-lost";
    guard.root=window->contentItem(); guard.anchor=window->activeFocusItem();
    if(!progress())return "open-context-lost";
    if(!guard.root || !guard.anchor)return local("anchor-unavailable");
    QPointer<QQuickItem> item=guard.anchor;
    QSet<QQuickItem *> seen;
    for(int edges=0;;++edges){
        if(!progress())return "open-context-lost";
        if(!item)return local("root-unreached");
        if(!producer || item->thread()!=producer->thread() || item->window()!=window)return local("item-context");
        if(seen.contains(item))return local("cycle");
        if(!progress())return "open-context-lost";
        seen.insert(item);guard.chain.append(item);++c.items;
        if(!guard.watchItem(item))return local("observer-unavailable");
        if(!progress())return "open-context-lost";
        if(!guard.endpoints()){guard.invalidate();return local("item-context");}
        if(!progress())return "open-context-lost";
        if(item==guard.root){c.complete=true;break;}
        if(edges==24)return local("depth-bound");
        item=item->parentItem(); // exactly one read per followed edge; never edge25
        if(!progress())return "open-context-lost";
    }
    QList<QPointer<QQuickItem>> receivers,scenes;
    d.visited=d.receivers=d.scenes=0;
    for(auto it=guard.chain.crbegin();it!=guard.chain.crend();++it){
        if(!progress())return "open-context-lost";
        const auto node=*it;
        if(!node)return "open-item-lost";
        ++d.visited;
        if(qmlEngine(node)==engine && (node->flags()&QQuickItem::ItemIsFocusScope) &&
            propertyType(node,"currentPage",QMetaType::fromType<int>()) &&
            propertyType(node,"currentPageId",QMetaType::fromType<QString>()) &&
            propertyType(node,"drawingAreaFocused",QMetaType::fromType<bool>()) &&
            node->metaObject()->indexOfProperty("document")>=0)receivers.append(node);
        if(focusSceneCandidate(node,engine,funnel))scenes.append(node);
        d.receivers=receivers.size();d.scenes=scenes.size();
        if(!progress())return "open-context-lost";
        if(d.receivers>8 || d.scenes>8)return "open-candidate-bound";
    }
    d.matches=0;
    for(int ri=0;ri<receivers.size();++ri)for(int si=0;si<scenes.size();++si){
        if(!progress())return "open-context-lost";
        PageOwner candidate{window,receivers[ri],scenes[si],{}};
        const char *reason=nullptr;
        bool valid=objectProperty(candidate.receiver,"document",candidate.document);
        if(!valid)reason="receiver-document";
        else valid=activeOwner(candidate,engine,&reason);
        if(!progress())return "open-context-lost";
        if(!valid){if(d.firstReceiver<0){d.firstReceiver=ri;d.firstScene=si;d.firstRejection=reason;}continue;}
        ++d.matches;
        if(d.matches>1){out={};return "open-owner-ambiguous";}
        out=candidate;
    }
    if(!d.matches)return "open-owner-unavailable";
    if(!progress()){out={};return "open-context-lost";}
    const char *finalReason=nullptr;
    const bool finalValid=activeOwner(out,producer,&finalReason);
    if(!progress()){out={};return "open-context-lost";}
    if(!finalValid){d.finalRejection=finalReason;out={};return "open-context-lost";}
    return "open-owner-observed";
}
}
