// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - present Mikael Sundell
// https://github.com/mikaelsundell/specviz

#include "tracelabel.h"
#include <QFontMetrics>
#include <QPainter>
#include <QPointer>
#include <QStyle>
#include <QStyleOption>

class TraceLabelPrivate : public QObject {
public:
    void init();
    void updateElision();
    struct Data {
        QPointer<TraceLabel> widget;
    };
    Data d;
};

void
TraceLabelPrivate::init()
{
}

TraceLabel::TraceLabel(QWidget* parent)
    : QLabel(parent)
    , p(new TraceLabelPrivate())
{
    p->d.widget = this;
    p->init();
}

TraceLabel::~TraceLabel() {}

void
TraceLabel::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QStyleOption opt;
    opt.initFrom(this);

    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

    QRect r = contentsRect();
    const QMargins m = contentsMargins();
    r.adjust(m.left(), m.top(), -m.right(), -m.bottom());
    r.adjust(0, 0, -20, 0);

    if (r.width() <= 0 || r.height() <= 0)
        return;

    const QString fullText = QLabel::text();
    const QFontMetrics fm(font());
    const QString elided = fm.elidedText(fullText, Qt::ElideRight, r.width());
    style()->drawItemText(
        &p,
        r,
        alignment(),
        opt.palette,
        isEnabled(),
        elided,
        foregroundRole()
    );
}
