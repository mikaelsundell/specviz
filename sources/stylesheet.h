// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - present Mikael Sundell
// https://github.com/mikaelsundell/specviz

#pragma once

#include <QObject>

class StylesheetPrivate;
class Stylesheet : public QObject {
public:
    Q_OBJECT
public:
    static Stylesheet* instance();
    bool loadCss(const QString& path);
    QString compiled() const;

    void setColor(const QString& name, const QColor& color);
    QColor color(const QString& name) const;

private:
    Stylesheet();
    ~Stylesheet();
    Stylesheet(const Stylesheet&) = delete;
    Stylesheet& operator=(const Stylesheet&) = delete;
    class Deleter {
    public:
        static void cleanup(Stylesheet* pointer) { delete pointer; }
    };
    static QScopedPointer<Stylesheet, Deleter> pi;
    QScopedPointer<StylesheetPrivate> p;
};
