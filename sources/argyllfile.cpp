// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - present Mikael Sundell
// https://github.com/mikaelsundell/specviz

#include "argyllfile.h"

#include <QDebug>
#include <QFile>
#include <QRegularExpression>
#include <QStringList>
#include <QTextStream>

SpecFile::Dataset
ArgyllFile::read(const QString& fileName)
{
    Dataset dataset;
    dataset.loaded = false;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Argyll: cannot open file:" << fileName;
        return dataset;
    }

    QTextStream in(&file);
    QStringList dataFormat;
    QVector<double> values;
    int bands = 0;
    int numSets = 0;
    double startNm = 0.0;
    double endNm = 0.0;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#')) {
            continue;
        }

        if (line.startsWith("DESCRIPTOR") || line.startsWith("ORIGINATOR") || line.startsWith("CREATED")
            || line.startsWith("MEAS_TYPE") || line.startsWith("SPECTRAL_BANDS") || line.startsWith("SPECTRAL_START_NM")
            || line.startsWith("SPECTRAL_END_NM") || line.startsWith("SPECTRAL_NORM")
            || line.startsWith("NUMBER_OF_FIELDS") || line.startsWith("NUMBER_OF_SETS")) {
            QString key = line.section(' ', 0, 0).trimmed();
            QString value = line.section(' ', 1).trimmed().remove('"');
            dataset.header.insert(key, value);

            if (key == "SPECTRAL_BANDS") {
                bands = value.toInt();
            }
            else if (key == "SPECTRAL_START_NM") {
                startNm = value.toDouble();
            }
            else if (key == "SPECTRAL_END_NM") {
                endNm = value.toDouble();
            }
            else if (key == "NUMBER_OF_SETS") {
                numSets = value.toInt();
            }
        }

        if (line.startsWith("BEGIN_DATA_FORMAT")) {
            while (!in.atEnd()) {
                QString dfLine = in.readLine().trimmed();
                if (dfLine.startsWith("END_DATA_FORMAT"))
                    break;
            }
            continue;
        }

        if (line.startsWith("BEGIN_DATA")) {
            while (!in.atEnd()) {
                QString dLine = in.readLine().trimmed();
                if (dLine.startsWith("END_DATA"))
                    break;
                QStringList parts = dLine.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
                for (const QString& p : parts)
                    values.append(p.toDouble());
            }
        }
    }

    if (numSets <= 0)
        numSets = 1;

    if (!dataFormat.isEmpty()) {
        dataset.indices = dataFormat;
    }
    else {
        for (int s = 0; s < numSets; ++s)
            dataset.indices << QString("Set %1").arg(s + 1);
    }

    if (!values.isEmpty() && bands > 0) {
        double step = (bands > 1) ? (endNm - startNm) / (bands - 1) : 0.0;
        for (int i = 0; i < bands; ++i) {
            int wl = static_cast<int>(qRound(startNm + i * step));

            QVector<double> row;
            row.reserve(numSets);
            for (int s = 0; s < numSets; ++s) {
                int idx = s * bands + i;
                if (idx < values.size())
                    row.append(values[idx]);
            }

            dataset.data.insert(wl, row);
        }
    }
    if (dataset.header.contains("ORIGINATOR")) {
        dataset.name = dataset.header.value("ORIGINATOR").toString();
    }
    else {
        dataset.name = "Argyll spectral reflectance/emission data";
    }
    if (dataset.header.contains("MEAS_TYPE")) {
        QString type = dataset.header.value("MEAS_TYPE").toString().toUpper();
        if (type == "AMBIENT") {
            dataset.units = "ambient illuminance";
        }
        else if (type == "REFLECTIVE") {
            dataset.units = "reflectance sensitivity";
        }
    }
    dataset.loaded = true;
    return dataset;
}

bool
ArgyllFile::write(const Dataset& dataset, const QString& fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "ArgyllFile: cannot write file:" << fileName;
        return false;
    }

    QTextStream out(&file);

    for (auto it = dataset.header.constBegin(); it != dataset.header.constEnd(); ++it) {
        out << it.key() << " " << it.value().toString() << "\n";
    }

    qsizetype bands = dataset.data.size();
    if (bands == 0) {
        qWarning() << "ArgyllFile: dataset has no spectral data to write.";
        return false;
    }

    qsizetype numSets = dataset.indices.size();
    if (numSets == 0)
        numSets = 1;

    int startNm = dataset.data.firstKey();
    int endNm = dataset.data.lastKey();

    out << "SPECTRAL_BANDS " << bands << "\n";
    out << "SPECTRAL_START_NM " << startNm << "\n";
    out << "SPECTRAL_END_NM " << endNm << "\n";
    out << "NUMBER_OF_SETS " << numSets << "\n";
    out << "BEGIN_DATA_FORMAT\n";
    for (const QString& idx : dataset.indices) {
        out << idx << " ";
    }
    out << "\nEND_DATA_FORMAT\n";
    out << "BEGIN_DATA\n";
    for (auto it = dataset.data.constBegin(); it != dataset.data.constEnd(); ++it) {
        const QVector<double>& row = it.value();
        for (int i = 0; i < row.size(); ++i) {
            out << row[i];
            if (i < row.size() - 1)
                out << " ";
        }
        out << "\n";
    }
    out << "END_DATA\n";

    file.close();
    return true;
}
