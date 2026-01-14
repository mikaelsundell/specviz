// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - present Mikael Sundell
// https://github.com/mikaelsundell/specviz

#pragma once

#include <QDialog>
#include <QUrl>

class MessageBoxPrivate;
class MessageBox : public QDialog {
    Q_OBJECT
public:
    static bool information(QWidget* parent, const QString& title, const QString& text);
    static bool warning(QWidget* parent, const QString& title, const QString& text);
    static bool question(QWidget* parent, const QString& title, const QString& text);
    static bool about(QWidget* parent, const QString& title, const QString& heading, const QString& details,
                      const QString& url = QString());
    static bool update(QWidget* parent, const QString& title, const QString& heading, const QString& details,
                       const QString& url = QString());

private:
    MessageBox(QWidget* parent = nullptr);
    ~MessageBox();

private:
    QScopedPointer<MessageBoxPrivate> p;
};
