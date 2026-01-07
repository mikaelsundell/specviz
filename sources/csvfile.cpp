// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - present Mikael Sundell
// https://github.com/mikaelsundell/specviz

#include "csvfile.h"

#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStringList>
#include <QTextStream>

SpecFile::Dataset
CsvFile::read(const QString& fileName)
{
    Dataset dataset;
    dataset.loaded = false;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "CsvFile: cannot open file:" << fileName;
        return dataset;
    }
    QTextStream in(&file);
    QString headerLine;
    while (!in.atEnd()) {
        headerLine = in.readLine().trimmed();
        if (!headerLine.isEmpty() && !headerLine.startsWith('#'))
            break;
    }

    if (headerLine.isEmpty()) {
        qWarning() << "CsvFile: empty CSV file";
        return dataset;
    }

    QStringList headers = headerLine.split(',', Qt::SkipEmptyParts);
    if (headers.contains("wavelength_nm") && headers.contains("value")) {
        dataset.name = QFileInfo(fileName).baseName();
        dataset.units = "relative spectral power";
        dataset.indices << "Set 1";

        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.isEmpty() || line.startsWith('#'))
                continue;

            QStringList parts = line.split(',', Qt::SkipEmptyParts);
            if (parts.size() < 2)
                continue;

            int wl = qRound(parts[0].toDouble());
            double v = parts[1].toDouble();

            dataset.data[wl] = QVector<double> { v };
        }

        dataset.loaded = !dataset.data.isEmpty();
        return dataset;
    }

    bool ok = false;
    headers.value(2).toDouble(&ok);
    if (headers.size() > 2 && ok) {
        QVector<int> wavelengths;
        for (int i = 2; i < headers.size(); ++i)
            wavelengths.append(headers[i].toInt());

        dataset.name = QFileInfo(fileName).baseName();
        dataset.units = "reflectance sensitivity";

        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.isEmpty() || line.startsWith('#'))
                continue;

            QStringList parts = line.split(',', Qt::KeepEmptyParts);
            if (parts.size() < wavelengths.size() + 2)
                continue;

            QString curveName = parts[1];
            dataset.indices << curveName;

            for (int i = 0; i < wavelengths.size(); ++i) {
                int wl = wavelengths[i];
                double v = parts[i + 2].toDouble();
                dataset.data[wl].append(v);
            }
        }

        dataset.loaded = !dataset.data.isEmpty();
        return dataset;
    }
    return dataset;
}


bool
CsvFile::write(const Dataset& dataset, const QString& fileName)
{
    return true;
}
