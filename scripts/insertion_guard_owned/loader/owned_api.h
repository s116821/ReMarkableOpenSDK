#pragma once
// Original owned interoperability surface. No private target declarations.
#include <QSizeF>
#include <QString>
#include <QUuid>
#include <cstdint>
using Word=std::uint32_t;
using Outer=void(*)(void*,int,const void*,Word,QSizeF,int,Word,const QUuid&);
using Inner=void(*)(void*,int,const QString&,const QString&,std::int32_t,std::int32_t);
using Note=bool(*)(unsigned);
extern "C" void owned_loader_outer(void*,int,const void*,Word,QSizeF,int,Word,const QUuid&);
extern "C" void owned_loader_inner(void*,int,const QString&,const QString&,std::int32_t,std::int32_t);
