// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - present Mikael Sundell
// https://github.com/mikaelsundell/specviz

#pragma once

#include <QDockWidget>

class DockWidgetPrivate;
class DockWidget : public QDockWidget {
    Q_OBJECT
public:
    DockWidget(QWidget* parent = nullptr);
    virtual ~DockWidget();

    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    QScopedPointer<DockWidgetPrivate> p;
};
