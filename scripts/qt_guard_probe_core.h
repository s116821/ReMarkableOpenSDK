/* Development evidence only: retain metadata and a weak guard, never QObject. */
#pragma once
#include <QtQml/private/qqmlmetatype_p.h>
#include <QtQml/qqmlprivate.h>
#include <QPointer>

static_assert(QT_VERSION == QT_VERSION_CHECK(6, 10, 3), "Probe requires exact reviewed Qt headers");

namespace guard_probe {
struct Candidate {
    QQmlType type;
    QQmlType::SingletonInstanceInfo::ConstPtr info;
    QPointer<QObject> guard;
    bool registrationMatch = false;
    bool typedTargetMatch = false;
};

inline Candidate lookup(const QString &module, const QString &name,
                        QTypeRevision version, const QMetaObject *expectedMeta) {
    Candidate result;
    // One already-registered lookup. Qt returns the first matching registration;
    // this cannot establish uniqueness. No module import or engine is requested.
    result.type = QQmlMetaType::qmlType(module + QLatin1Char('/') + name, version);
    if (!result.type.isValid() || !result.type.isQObjectSingleton() ||
        result.type.module() != module || result.type.elementName() != name ||
        result.type.version() != version || !expectedMeta ||
        result.type.baseMetaObject() != expectedMeta) return result;
    result.info = result.type.singletonInstanceInfo();
    if (!result.info || result.info->scriptCallback || !result.info->qobjectCallback)
        return result;
    result.registrationMatch = true;
    const auto *target = result.info->qobjectCallback.target<QQmlPrivate::SingletonInstanceFunctor>();
    if (!target) return result; // Includes incompatible RTTI identity; no raw fallback.
    result.typedTargetMatch = true;
    result.guard = target->m_object; // Same-type weak copy; does not access QObject.
    return result;
}
} // namespace guard_probe
