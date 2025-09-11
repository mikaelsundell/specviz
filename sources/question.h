// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - present Mikael Sundell
// https://github.com/mikaelsundell/specviz

#pragma once

#include <QDialog>

class QuestionPrivate;
class Question : public QDialog {
    Q_OBJECT
public:
    Question(QWidget* parent = nullptr);
    virtual ~Question();
    void setQuestion(const QString& question);

    static bool askQuestion(QWidget* parent, const QString& question);

private:
    QScopedPointer<QuestionPrivate> p;
};
