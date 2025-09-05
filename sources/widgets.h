// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - present Mikael Sundell
// https://github.com/mikaelsundell/usdviewer

#pragma once

#include <QTreeWidgetItem>

class TreeItemPrivate;
class TreeItem : public QTreeWidgetItem {
public:
    enum Column { Key = 0, Value };
    TreeItem(QTreeWidget* parent);
    TreeItem(QTreeWidgetItem* parent);
    virtual ~TreeItem();

private:
    QScopedPointer<TreeItemPrivate> p;
};
