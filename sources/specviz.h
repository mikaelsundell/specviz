// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - present Mikael Sundell
// https://github.com/mikaelsundell/specviz

#pragma once

#include <QMainWindow>
#include <QScopedPointer>

class SpecvizPrivate;
class Specviz : public QMainWindow {
public:
    Specviz(QWidget* parent = nullptr);
    virtual ~Specviz();
    void setArguments(const QStringList& arguments);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    QScopedPointer<SpecvizPrivate> p;
};
