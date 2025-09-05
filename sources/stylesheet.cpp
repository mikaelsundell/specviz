// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - present Mikael Sundell
// https://github.com/mikaelsundell/specviz

#include "stylesheet.h"
#include "icctransform.h"
#include <QFile>
#include <QHash>
#include <QMutex>
#include <QMutexLocker>
#include <QRegularExpression>
#include <QApplication>

QScopedPointer<Stylesheet, Stylesheet::Deleter> Stylesheet::pi;

class StylesheetPrivate : public QObject {
    Q_OBJECT
public:
    StylesheetPrivate();
    ~StylesheetPrivate();

public:
    QString path;
    QString compiled;
    QHash<QString, QColor> palette;
};

StylesheetPrivate::StylesheetPrivate() {}

StylesheetPrivate::~StylesheetPrivate()
{
}

#include "stylesheet.moc"

Stylesheet::Stylesheet()
    : p(new StylesheetPrivate())
{}

Stylesheet::~Stylesheet() {}

bool
Stylesheet::loadCss(const QString& path)
{
    QFile file(path);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        return false;
    }
    p->path = path;
    QString output = QString::fromUtf8(file.readAll());
    for (auto it = p->palette.constBegin(); it != p->palette.constEnd(); ++it) {
        QString placeholder = "%" + it.key() + "%";
        QColor color = it.value();
        QString hsl = QString("hsl(%1, %2%, %3%)")
                          .arg(color.hue() == -1 ? 0 : color.hue())
                          .arg(static_cast<int>(color.hslSaturationF() * 100))
                          .arg(static_cast<int>(color.lightnessF() * 100));
        output.replace(placeholder, hsl, Qt::CaseInsensitive);
    }

    QRegularExpression hslRegex(R"(hsl\(\s*(\d+)\s*,\s*(\d+)%\s*,\s*(\d+)%\s*\))");
    QRegularExpressionMatchIterator it = hslRegex.globalMatch(output);

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        int h = match.captured(1).toInt();
        int s = match.captured(2).toInt();
        int l = match.captured(3).toInt();

        QColor color = QColor::fromHslF(h / 360.0, s / 100.0, l / 100.0);

        ICCTransform* transform = ICCTransform::instance();
        color = transform->map(color.rgb());

        QString replacement = QString("hsl(%1, %2%, %3%)")
                                  .arg(color.hue() == -1 ? 0 : color.hue())
                                  .arg(static_cast<int>(color.hslSaturationF() * 100))
                                  .arg(static_cast<int>(color.lightnessF() * 100));
        output.replace(match.captured(0), replacement);
    }

    p->compiled = output;
    return true;
}

QString Stylesheet::compiled() const
{
    return p->compiled;
}

void Stylesheet::setColor(const QString& name, const QColor& color)
{
    p->palette[name.toLower()] = color;
}

QColor Stylesheet::color(const QString& name) const
{
    return p->palette.value(name.toLower(), QColor());
}

Stylesheet*
Stylesheet::instance()
{
    static QMutex mutex;
    QMutexLocker locker(&mutex);
    if (!pi) {
        pi.reset(new Stylesheet());
    }
    return pi.data();
}
