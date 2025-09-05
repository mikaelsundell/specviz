// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - present Mikael Sundell
// https://github.com/mikaelsundell/specviz

#include "sekonicreader.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

SpecReader::Dataset SeconicReader::read(const QString& fileName) {
    Dataset dataset;
    dataset.loaded = false;
    return dataset;
}
