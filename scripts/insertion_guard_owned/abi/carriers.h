#pragma once
// Original owned carrier declarations. They are NOT recovered native types.
#include <QString>
#include <QSizeF>
#include <QUuid>
#include <cstddef>
#include <cstdint>
struct OwnedOptionalQString { QString value; std::uint8_t engaged; };
static_assert(sizeof(void *) == 4 && sizeof(QString) == 12 && alignof(QString) == 4);
static_assert(sizeof(OwnedOptionalQString) == 16 && offsetof(OwnedOptionalQString, engaged) == 12);
static_assert(sizeof(QUuid) == 16 && alignof(QUuid) == 4);
static_assert(sizeof(qreal) == 8 && sizeof(QSizeF) == 16);
struct OwnedWorker;
struct OwnedMap;
using OuterCarrier = void (*)(OwnedWorker *, std::int32_t, const OwnedOptionalQString &,
    std::uint8_t, QSizeF, std::int32_t, std::uint8_t, const QUuid &);
using InnerCarrier = void (*)(OwnedMap *, std::int32_t, const QString &, const QString &,
    std::int32_t, std::int32_t);
extern "C" void owned_outer_entry(OwnedWorker *, std::int32_t, const OwnedOptionalQString &,
    std::uint8_t, QSizeF, std::int32_t, std::uint8_t, const QUuid &);
extern "C" void owned_inner_entry(OwnedMap *, std::int32_t, const QString &, const QString &,
    std::int32_t, std::int32_t);
