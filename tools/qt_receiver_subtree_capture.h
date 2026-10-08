#pragma once
#include "qt_focus_owner.h"

namespace qml_access {
// Unqualified capture correlation only. Never call this for facts admission.
inline bool receiverSubtreeCapturePredicate(const PageOwner &owner,QQmlEngine *engine,const std::function<bool()> &progress,const char **reason=nullptr,bool allowUnfocusedArea=false) {
    const auto refuse=[reason](const char *value){if(reason)*reason=value;return false;};
    if(!progress())return refuse("capture-context");
    if(!owner.window || !owner.receiver || !owner.scene || !owner.document || !engine ||
        QThread::currentThread()!=engine->thread() || owner.window->thread()!=engine->thread() ||
        owner.receiver->thread()!=engine->thread() || owner.scene->thread()!=engine->thread() ||
        owner.document->thread()!=engine->thread())return refuse("pointers-or-threads");
    if(QGuiApplication::focusWindow()!=owner.window || !owner.window->isActive() || !owner.window->isVisible())return refuse("window-focus-active-visible");
    if(qmlEngine(owner.receiver)!=engine || qmlEngine(owner.scene)!=engine)return refuse("engine-association");
    if(owner.receiver->window()!=owner.window || owner.scene->window()!=owner.window)return refuse("window-association");
    if(!owner.receiver->isVisible() || !owner.receiver->isEnabled() || !owner.scene->isVisible() || !owner.scene->isEnabled())return refuse("item-visible-enabled");
    // SceneView.hasActiveFocus deliberately excluded in this separate predicate.
    bool descendant=false;
    QPointer<QQuickItem> parent=owner.scene->parentItem();
    for(int depth=0;parent && depth<24;++depth,parent=parent->parentItem())
        if(parent==owner.receiver){descendant=true;break;}
    if(!descendant)return refuse("receiver-ancestor");
    const QVariant focused=owner.receiver->property("drawingAreaFocused");
    if(!progress())return refuse("capture-context");
    if(!owner.receiver || !owner.scene || focused.metaType()!=QMetaType::fromType<bool>() || (!allowUnfocusedArea && !focused.toBool()))return refuse("drawing-area-focused");
    QPointer<QObject> receiverDocument,sceneDocument;
    if(!objectProperty(owner.receiver,"document",receiverDocument))return refuse("receiver-document");
    if(!progress())return refuse("capture-context");
    if(!objectProperty(owner.scene,"document",sceneDocument))return refuse("scene-document");
    if(!progress())return refuse("capture-context");
    if(!owner.document || receiverDocument!=owner.document || sceneDocument!=owner.document)return refuse("document-identity");
    return true;
}
inline bool receiverSubtreeCaptureIdentity(const PageOwner &owner,const QString &document,const QStringList &order,
    const std::function<bool()> &progress,const char **reason=nullptr) {
    const auto refuse=[reason](const char *value){if(reason)*reason=value;return false;};
    if(!progress() || !owner.document || !owner.receiver || !owner.scene)return refuse("identity-context");
    const QVariant id=owner.document->property("id");
    if(!progress() || !owner.document || !owner.receiver || !owner.scene)return refuse("identity-context");
    const QVariant index=owner.receiver->property("currentPage");
    if(!progress() || !owner.document || !owner.receiver || !owner.scene)return refuse("identity-context");
    const QVariant page=owner.scene->property("pageId");
    if(!progress() || !owner.document || !owner.receiver || !owner.scene)return refuse("identity-context");
    const QVariant alias=owner.receiver->property("currentPageId");
    if(!progress() || !owner.document || !owner.receiver || !owner.scene)return refuse("identity-context");
    // Preserve all four reads before comparisons and the historical conjunction order.
    if(id.metaType()!=QMetaType::fromType<QString>())return refuse("identity-document-type");
    if(id.toString()!=document)return refuse("identity-document-mismatch");
    if(index.metaType()!=QMetaType::fromType<int>())return refuse("identity-index-type");
    if(index.toInt()<0 || index.toInt()>=order.size())return refuse("identity-index-range");
    if(page.metaType()!=QMetaType::fromType<QString>())return refuse("identity-scene-page-type");
    if(alias.metaType()!=QMetaType::fromType<QString>())return refuse("identity-receiver-page-type");
    if(page.toString()!=alias.toString())return refuse("identity-page-alias-mismatch");
    if(page.toString()!=order[index.toInt()])return refuse("identity-page-order-mismatch");
    return true;
}
inline bool receiverSubtreeWatchNotify(FocusOwnerGuard &guard,QObject *object,const QMetaMethod &signal) {
    if(!guard.context || !object || !signal.isValid() || signal.returnMetaType()!=QMetaType::fromType<void>())return false;
    const int slot=guard.context->metaObject()->indexOfSlot("invalidateCapture()");
    return slot>=0 && guard.retain(QObject::connect(object,signal,guard.context,guard.context->metaObject()->method(slot)));
}
inline const char *findReceiverSubtreeCaptureOwner(QQmlEngine *engine,const std::function<bool()> &progress,
    FocusOwnerGuard &guard,PageOwner &out,PageOwnerDiagnostics &d,FocusChainDiagnostics &chain,
    const QString &document,const QStringList &order,const std::function<bool(const PageOwner &)> &observe,int itemCap=256,int depthCap=8,bool allowUnfocusedArea=false,bool detailedIdentityDiagnostics=false) {
    out={};chain.items=0;
    if(itemCap!=256 && itemCap!=512 && itemCap!=1024 && itemCap!=2048 && itemCap!=4096)return "open-capture-scope-refused";
    if((depthCap!=8 && depthCap!=16 && depthCap!=32) || (depthCap==16 && itemCap!=2048 && itemCap!=4096) || (depthCap==32 && itemCap!=4096) || (itemCap==4096 && depthCap!=16 && depthCap!=32))return "open-capture-scope-refused";
    if(detailedIdentityDiagnostics && !allowUnfocusedArea)return "open-capture-scope-refused";
    if(allowUnfocusedArea && (itemCap!=4096 || depthCap!=32))return "open-capture-scope-refused";
    const QPointer<QQmlEngine> producer=engine;
    if(!progress())return "open-context-lost";
    if(!engine || QThread::currentThread()!=engine->thread())return "open-capture-scope-refused";
    const QPointer<QQuickWindow> window=qobject_cast<QQuickWindow *>(QGuiApplication::focusWindow());
    if(!progress())return "open-context-lost";
    if(!window || !window->isActive() || !window->isVisible() || !guard.watchWindow(window))return "open-capture-scope-refused";
    guard.root=window->contentItem();guard.anchor=window->activeFocusItem();
    if(!progress())return "open-context-lost";
    if(!guard.root || !guard.anchor)return "open-capture-scope-refused";
    QPointer<QQuickItem> item=guard.anchor;QSet<QQuickItem *> seen;
    for(int edges=0;;++edges){
        if(!progress())return "open-context-lost";
        if(!item || !producer || item->thread()!=producer->thread() || item->window()!=window || seen.contains(item))return "open-capture-scope-refused";
        seen.insert(item);guard.chain.append(item);++chain.items;
        if(!guard.watchItem(item))return "open-capture-scope-refused";
        if(!progress())return "open-context-lost";
        if(!guard.endpoints()){guard.invalidate();(void)progress();return "open-context-lost";}
        if(item==guard.root){chain.complete=true;break;}
        if(edges==24)return "open-capture-scope-refused";
        item=item->parentItem();
        if(!progress())return "open-context-lost";
    }
    QList<QPointer<QQuickItem>> receivers;
    for(const auto &node:guard.chain){
        if(!progress())return "open-context-lost";
        if(!node)return "open-item-lost";
        if(qmlEngine(node)==engine && (node->flags()&QQuickItem::ItemIsFocusScope) &&
            propertyType(node,"currentPage",QMetaType::fromType<int>()) &&
            propertyType(node,"currentPageId",QMetaType::fromType<QString>()) &&
            propertyType(node,"drawingAreaFocused",QMetaType::fromType<bool>()) &&
            node->metaObject()->indexOfProperty("document")>=0)receivers.append(node);
    }
    if(!progress())return "open-context-lost";
    if(receivers.size()!=1)return receivers.isEmpty() ? "open-capture-receiver-unavailable":"open-capture-receiver-ambiguous";
    const auto receiver=receivers.first();
    // Install receiver value observers before its first native document getter.
    for(const char *name:{"document","currentPage","currentPageId","drawingAreaFocused"}){
        const auto property=receiver->metaObject()->property(receiver->metaObject()->indexOfProperty(name));
        if(!property.hasNotifySignal() || !receiverSubtreeWatchNotify(guard,receiver,property.notifySignal()))return "open-capture-scope-refused";
    }
    if(!progress())return "open-context-lost";
    struct Node {QPointer<QQuickItem> item;int depth;};
    QList<Node> queue{{receiver,0}};QList<QPointer<QQuickItem>> scenes;seen.clear();d.visited=d.scenes=0;d.receivers=1;
    for(int cursor=0;cursor<queue.size();++cursor){
        if(!progress())return "open-context-lost";
        const auto node=queue[cursor];
        if(!node.item || !producer || node.item->thread()!=producer->thread() || node.item->window()!=window || seen.contains(node.item))return "open-capture-scope-refused";
        seen.insert(node.item);++d.visited;
        const auto changed=[mark=guard.invalidate]{mark();};
        if(!guard.retain(QObject::connect(node.item,&QObject::destroyed,guard.context,changed)) ||
            !guard.retain(QObject::connect(node.item,&QQuickItem::parentChanged,guard.context,changed)) ||
            !guard.retain(QObject::connect(node.item,&QQuickItem::windowChanged,guard.context,changed)) ||
            !guard.retain(QObject::connect(node.item,&QQuickItem::childrenChanged,guard.context,changed)))return "open-capture-scope-refused";
        if(!progress())return "open-context-lost";
        if(focusSceneCandidate(node.item,engine)){scenes.append(node.item);d.scenes=scenes.size();if(scenes.size()>8)return "open-candidate-bound";}
        if(!progress())return "open-context-lost";
        const auto children=node.item->childItems();
        if(!progress())return "open-context-lost";
        if(!children.isEmpty() && (node.depth==depthCap || children.size()>itemCap-queue.size())){
            d.topologyLimit=node.depth==depthCap ? "subtree-depth":"subtree-items";
            d.topologyDepth=node.depth;d.topologyQueueSize=queue.size();d.topologyChildCount=children.size();
            return "open-capture-subtree-bound";
        }
        for(auto *child:children)queue.append({child,node.depth+1});
    }
    if(!progress())return "open-context-lost";
    d.matches=0;
    for(int si=0;si<scenes.size();++si){
        if(!progress())return "open-context-lost";
        PageOwner candidate{window,receiver,scenes[si],{}};const char *reason=nullptr;
        bool valid=objectProperty(candidate.receiver,"document",candidate.document);
        if(!progress())return "open-context-lost";
        if(valid && !observe(candidate))return "open-capture-scope-refused";
        if(!progress())return "open-context-lost";
        if(valid)valid=receiverSubtreeCapturePredicate(candidate,producer,progress,&reason,allowUnfocusedArea);
        if(!progress())return "open-context-lost";
        if(valid)valid=receiverSubtreeCaptureIdentity(candidate,document,order,progress,detailedIdentityDiagnostics ? &reason:nullptr);
        if(!progress())return "open-context-lost";
        if(!valid){if(d.firstScene<0){d.firstReceiver=0;d.firstScene=si;d.firstRejection=reason ? reason:"capture-identity";}continue;}
        ++d.matches;
        if(d.matches>1){out={};return "open-owner-ambiguous";}
        out=candidate;
    }
    if(!d.matches)return "open-owner-unavailable";
    if(!progress())return "open-context-lost";
    const bool valid=receiverSubtreeCapturePredicate(out,producer,progress,&d.finalRejection,allowUnfocusedArea) && receiverSubtreeCaptureIdentity(out,document,order,progress,detailedIdentityDiagnostics ? &d.finalRejection:nullptr);
    if(!progress()){out={};return "open-context-lost";}
    if(!valid){out={};return "open-context-lost";}
    return "open-capture-scene-correlated";
}
}
