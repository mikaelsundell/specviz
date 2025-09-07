// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - present Mikael Sundell
// https://github.com/mikaelsundell/specviz

#include "sekonicreader.h"

#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

SpecReader::Dataset
SekonicReader::read(const QString& fileName)
{
    Dataset dataset;
    dataset.loaded = false;
    return dataset;
}
