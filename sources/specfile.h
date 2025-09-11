// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - present Mikael Sundell
// https://github.com/mikaelsundell/specviz

#pragma once

#include <QMap>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QVector>

class SpecFile {
public:
    struct Dataset {
        QString name;
        QVariantMap header;               // flexible header
        QString units;                    // e.g. "relative"
        QStringList indices;              // e.g. ["R","G","B"]
        QMap<int, QVector<double>> data;  // wavelength -> [values]
        bool loaded;
    };
    virtual ~SpecFile() = default;
    virtual Dataset read(const QString& fileName) = 0;
    virtual bool write(const Dataset& dataset, const QString& fileName) = 0;
    virtual QStringList extensions() = 0;
};
