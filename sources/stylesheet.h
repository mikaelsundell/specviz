// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - present Mikael Sundell
// https://github.com/mikaelsundell/specviz

#pragma once

#include <QObject>
#include <QColor>
#include <QScopedPointer>
#include <QHash>

class StylesheetPrivate;
class Stylesheet : public QObject {
    Q_OBJECT
public:
    enum ColorRole {
        Base,
        BaseAlt,
        Accent,
        AccentAlt,
        Text,
        TextDisabled,
        Highlight,
        Border,
        Scrollbar,
        Progress,
    };
    Q_ENUM(ColorRole)

    static Stylesheet* instance();
    void applyQss(const QString& qss);
    bool loadQss(const QString& path);
    QString compiled() const;
    

    void setColor(ColorRole role, const QColor& color);
    QColor color(ColorRole role) const;

private:
    Stylesheet();
    ~Stylesheet();
    Stylesheet(const Stylesheet&) = delete;
    Stylesheet& operator=(const Stylesheet&) = delete;

    QString roleName(ColorRole role) const;

    class Deleter {
    public:
        static void cleanup(Stylesheet* pointer) { delete pointer; }
    };
    static QScopedPointer<Stylesheet, Deleter> pi;
    QScopedPointer<StylesheetPrivate> p;
};
