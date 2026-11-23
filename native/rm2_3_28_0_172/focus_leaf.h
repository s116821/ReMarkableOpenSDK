#pragma once

#include <QGuiApplication>
#include <QList>
#include <QPointer>
#include <QQuickItem>
#include <QQuickWindow>
#include <QThread>
#include <functional>

// Local read-only diagnostic. The caller supplies the already joined Reader
// window, outer focus scope and exact current inner SceneView. No item identity
// or permission survives this synchronous sample.
namespace rem61_focus {
enum class Leaf {
    Refused,
    NoFocusedWindow,
    OtherWindow,
    NoLeaf,
    ExactScene,
    InsideScene,
    ScopeOnly,
    OutsideScope,
};

inline constexpr qsizetype MaxParents = 24;
using Chain = QList<QPointer<QQuickItem>>;

inline bool ancestry(QQuickItem *item, QQuickWindow *window, Chain &chain) {
    for (auto *current = item; current; current = current->parentItem()) {
        if (chain.size() == MaxParents || current->thread() != QThread::currentThread()
            || current->window() != window || chain.contains(current))
            return false;
        chain.append(current);
    }
    return true;
}

inline Leaf sample(QQuickWindow *window, QQuickItem *scope, QQuickItem *scene,
                   const std::function<bool()> &stillJoined) {
    auto *app = qobject_cast<QGuiApplication *>(QCoreApplication::instance());
    if (!app || QCoreApplication::closingDown() || QThread::currentThread() != app->thread()
        || !window || window->thread() != app->thread() || !scope || !scene || scope == scene)
        return Leaf::Refused;

    QPointer<QQuickWindow> windowPin(window);
    QPointer<QQuickItem> scopePin(scope), scenePin(scene);
    Chain sceneChain;
    if (!ancestry(scene, window, sceneChain) || !sceneChain.contains(scope))
        return Leaf::Refused;
    QPointer<QWindow> focusWindow(QGuiApplication::focusWindow());
    QPointer<QQuickItem> leaf(window->activeFocusItem());
    Chain leafChain;
    if (!ancestry(leaf, window, leafChain))
        return Leaf::Refused;
    const auto inWindow = focusWindow == window;
    const auto sceneFocused = scene->hasActiveFocus();
    const auto scopeFocused = scope->hasActiveFocus();

    bool changed = false;
    QObject guard;
    auto invalidate = [&] { changed = true; };
    QObject::connect(app, &QGuiApplication::focusWindowChanged, &guard, invalidate, Qt::DirectConnection);
    QObject::connect(window, &QQuickWindow::activeFocusItemChanged, &guard, invalidate, Qt::DirectConnection);
    QObject::connect(window, &QWindow::activeChanged, &guard, invalidate, Qt::DirectConnection);
    QObject::connect(window, &QObject::destroyed, &guard, invalidate, Qt::DirectConnection);
    auto watched = sceneChain;
    for (const auto &item : leafChain)
        if (!watched.contains(item))
            watched.append(item);
    for (const auto &item : watched) {
        if (!item)
            return Leaf::Refused;
        QObject::connect(item.data(), &QQuickItem::parentChanged, &guard, invalidate, Qt::DirectConnection);
        QObject::connect(item.data(), &QQuickItem::windowChanged, &guard, invalidate, Qt::DirectConnection);
        QObject::connect(item.data(), &QQuickItem::activeFocusChanged, &guard, invalidate, Qt::DirectConnection);
        QObject::connect(item.data(), &QObject::destroyed, &guard, invalidate, Qt::DirectConnection);
    }

    if (!stillJoined || !stillJoined() || changed || !windowPin || !scopePin || !scenePin
        || QCoreApplication::closingDown() || QGuiApplication::focusWindow() != focusWindow
        || windowPin->activeFocusItem() != leaf || scenePin->hasActiveFocus() != sceneFocused
        || scopePin->hasActiveFocus() != scopeFocused)
        return Leaf::Refused;
    Chain currentScene, currentLeaf;
    if (!ancestry(scenePin, windowPin, currentScene) || currentScene != sceneChain
        || !ancestry(leaf, windowPin, currentLeaf) || currentLeaf != leafChain || changed)
        return Leaf::Refused;

    if (!focusWindow)
        return Leaf::NoFocusedWindow;
    if (!inWindow)
        return Leaf::OtherWindow;
    if (!leaf)
        return Leaf::NoLeaf;
    if (leaf == scenePin)
        return sceneFocused ? Leaf::ExactScene : Leaf::Refused;
    if (leafChain.contains(scenePin))
        return Leaf::InsideScene;
    if (leafChain.contains(scopePin))
        return Leaf::ScopeOnly;
    return Leaf::OutsideScope;
}
}
