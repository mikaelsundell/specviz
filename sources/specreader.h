// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - present Mikael Sundell
// https://github.com/mikaelsundell/specviz

#pragma once

#include <QVariantMap>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QVector>

class SpecReader {
public:
    struct Dataset {
        QVariantMap header;               // flexible header
        QString units;                    // e.g. "relative"
        QStringList indices;              // e.g. ["R","G","B"]
        QMap<int, QVector<double>> data;  // wavelength -> [values]
        bool loaded;
    };
    virtual ~SpecReader() = default;
    virtual Dataset read(const QString& fileName) = 0;
    virtual QStringList extensions() = 0;
};

