#pragma once
#include <QtCore>

struct SHAElement_;
typedef QSharedPointer<SHAElement_> SHAElement;
typedef QSharedPointer<QList<SHAElement>> SHAContainer;
typedef QList<QVariantMap> SHALinkList;
typedef QList<SHAContainer> SHAContainerList;

struct SHAElement_ {
    int x{};
    int y{};
    QString name;
    int id;
    SHALinkList linkList;
    SHAContainerList containerList;
};
