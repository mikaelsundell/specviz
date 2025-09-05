// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - present Mikael Sundell
// https://github.com/mikaelsundell/usdviewer


/*
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - present Mikael Sundell
// https://github.com/mikaelsundell/usdviewer

#include "usdinspectoritem.h"
#include <QPointer>

namespace usd {
class InspectorItemPrivate {
public:
    void init();
    struct Data {
        InspectorItem* item;
    };
    Data d;
};

void
InspectorItemPrivate::init()
{}

InspectorItem::InspectorItem(QTreeWidget* parent)
    : QTreeWidgetItem(parent)
    , p(new InspectorItemPrivate())
{}

InspectorItem::InspectorItem(QTreeWidgetItem* parent)
    : QTreeWidgetItem(parent)
    , p(new InspectorItemPrivate())
{}

InspectorItem::~InspectorItem() {}

}  // namespace usd


#include "usdoutlineritem.h"
#include <QPointer>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usdGeom/imageable.h>
#include <pxr/usd/usdGeom/tokens.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace usd {
class OutlinerItemPrivate {
public:
    void init();
    struct Data {
        UsdPrim prim;
        OutlinerItem* item;
    };
    Data d;
};

void
OutlinerItemPrivate::init()
{}

OutlinerItem::OutlinerItem(QTreeWidget* parent, const UsdPrim& prim)
    : QTreeWidgetItem(parent)
    , p(new OutlinerItemPrivate())
{
    p->d.item = this;
    p->d.prim = prim;
    setFlags(flags() | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    p->init();
}

OutlinerItem::OutlinerItem(QTreeWidgetItem* parent, const UsdPrim& prim)
    : QTreeWidgetItem(parent)
    , p(new OutlinerItemPrivate())
{
    p->d.item = this;
    p->d.prim = prim;
    setFlags(flags() | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    p->init();
}

OutlinerItem::~OutlinerItem() {}

QVariant
OutlinerItem::data(int column, int role) const
{
    if (role == Qt::DisplayRole || role == Qt::ToolTipRole) {
        switch (column) {
        case Name: return QString::fromStdString(p->d.prim.GetName().GetString());
        case Type: return QString::fromStdString(p->d.prim.GetTypeName().GetString());
        case Visible: {
            UsdGeomImageable imageable(p->d.prim);
            if (imageable && p->d.prim.IsActive()) {
                TfToken vis;
                imageable.GetVisibilityAttr().Get(&vis);
                return (vis == UsdGeomTokens->invisible) ? "H" : "V";
            }
            return "";
        }
        default: break;
        }
    }
    else if (role == Qt::UserRole) {
        return QString::fromStdString(p->d.prim.GetPath().GetString());
    }

    return QTreeWidgetItem::data(column, role);
}

bool
OutlinerItem::isVisible() const
{
    UsdGeomImageable imageable(p->d.prim);
    if (imageable && p->d.prim.IsActive()) {
        TfToken vis;
        imageable.GetVisibilityAttr().Get(&vis);
        return vis != UsdGeomTokens->invisible;
    }
    return false;
}

}  // namespace usd
*/
