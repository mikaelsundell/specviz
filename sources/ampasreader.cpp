// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - present Mikael Sundell
// https://github.com/mikaelsundell/specviz

#include "ampasreader.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

SpecReader::Dataset AmpasReader::read(const QString& fileName) {
    Dataset dataset;
    dataset.loaded = false;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "AmpasReader: cannot open file:" << fileName;
        return dataset;
    }

    QByteArray rawData = file.readAll();
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(rawData, &parseError);
    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "AmpasReader: JSON parse error:" << parseError.errorString();
        return dataset;
    }

    QJsonObject root = doc.object();
    if (root.contains("header") && root["header"].isObject()) {
        dataset.header = root["header"].toObject().toVariantMap();
    }

    if (root.contains("spectral_data") && root["spectral_data"].isObject()) {
        QJsonObject spectral = root["spectral_data"].toObject();
        dataset.units = spectral.value("units").toString();

        if (spectral.contains("index") && spectral["index"].isObject()) {
            QJsonObject indexObj = spectral["index"].toObject();
            if (indexObj.contains("main") && indexObj["main"].isArray()) {
                QJsonArray arr = indexObj["main"].toArray();
                for (auto v : arr) {
                    dataset.indices << v.toString();
                }
            }
        }

        if (spectral.contains("data") && spectral["data"].isObject()) {
            QJsonObject dataObj = spectral["data"].toObject();
            if (dataObj.contains("main") && dataObj["main"].isObject()) {
                QJsonObject mainObj = dataObj["main"].toObject();

                for (auto it = mainObj.begin(); it != mainObj.end(); ++it) {
                    bool ok = false;
                    int wavelength = it.key().toInt(&ok);
                    if (!ok) {
                        qWarning() << "AmpasReader: invalid wavelength key:" << it.key();
                        continue;
                    }

                    QVector<double> values;
                    QJsonArray valueArray = it.value().toArray();
                    values.reserve(valueArray.size());
                    for (auto v : valueArray) {
                        values << v.toDouble();
                    }

                    dataset.data.insert(wavelength, values);
                }
            }
        }
    }
    dataset.loaded = true;
    return dataset;
}
