// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - present Mikael Sundell
// https://github.com/mikaelsundell/specviz

#pragma once

#include <QLabel>

class TraceLabelPrivate;
class TraceLabel : public QLabel {
    Q_OBJECT
public:
    TraceLabel(QWidget* parent = nullptr);
    virtual ~TraceLabel();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QScopedPointer<TraceLabelPrivate> p;
};
