#include "qt_focus_owner.h"
#include <QQmlContext>
#include <cstdio>
class SceneView : public QQuickItem { Q_OBJECT };
class PageMetadata : public SceneView {
    Q_OBJECT
    Q_PROPERTY(QString pageId READ pageId)
public:
    mutable int reads=0;
    QString pageId() const {++reads;return {};}
};
class PageSignal : public PageMetadata {
    Q_OBJECT
signals: void pageIdChanged();
};
class CompleteScene : public PageSignal {
    Q_OBJECT
signals: void documentWrapperChanged();
};
class WrongPageMetadata : public SceneView {
    Q_OBJECT
    Q_PROPERTY(int pageId READ pageId)
public:
    mutable int reads=0;
    int pageId() const {++reads;return 0;}
signals: void pageIdChanged(); void documentWrapperChanged();
};
int main(int argc,char **argv) {
    QGuiApplication app(argc,argv);
    QQmlEngine engine;
    CompleteScene detached,positive;
    QQuickItem wrongClass;
    SceneView missingPage;
    PageMetadata missingPageSignal;
    PageSignal missingWrapperSignal;
    WrongPageMetadata wrongPage;
    QQuickItem *nodes[]={&detached,&wrongClass,&missingPage,&missingPageSignal,&missingWrapperSignal,&positive,&wrongPage};
    const int expected[]={0,1,2,3,4,5,2};
    const char *names[]={"engine","class","page-id-missing","page-id-changed","document-wrapper-changed","pass","page-id-type"};
    for(int i=0;i<7;++i){
        if(i)QQmlEngine::setContextForObject(nodes[i],engine.rootContext());
        qml_access::FocusSceneFunnel funnel;
        const bool result=qml_access::focusSceneCandidate(nodes[i],&engine,&funnel);
        const qint64 counts[]={funnel.engine,funnel.sceneClass,funnel.pageId,funnel.pageIdChanged,funnel.documentWrapperChanged,funnel.pass};
        if(result!=(expected[i]==5))return 1;
        for(int j=0;j<6;++j)if(counts[j]!=(j==expected[i] ? 1:0))return 2;
        if(qml_access::focusSceneCandidate(nodes[i],&engine)!=result)return 3;
        std::printf("PASS scene-funnel %s\n",names[i]);
    }
    if(detached.reads || positive.reads || missingPageSignal.reads || missingWrapperSignal.reads || wrongPage.reads)return 4;
    std::puts("PASS scene-funnel metadata-only zero getters");
    return 0;
}
#include "qt_focus_scene_funnel_fixture.moc"
