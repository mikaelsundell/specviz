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

    QStringList headers = headerLine.split(',', Qt::KeepEmptyParts);
    dataset.header.insert("CREATED", QDateTime::currentDateTime().toString("ddd MMM dd HH:mm:ss yyyy"));
    dataset.name = QFileInfo(fileName).baseName();

    if (headers.size() == 2 && headers[0] == "wavelength_nm" && headers[1] == "value") {
        dataset.units = "relative spectral power";
        dataset.indices << "Set 1";

        int minWl = std::numeric_limits<int>::max();
        int maxWl = std::numeric_limits<int>::min();

        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.isEmpty() || line.startsWith('#'))
                continue;

            QStringList parts = line.split(',', Qt::KeepEmptyParts);
            if (parts.size() != 2)
                continue;

            bool ok1 = false, ok2 = false;
            int wl = qRound(parts[0].toDouble(&ok1));
            double v = parts[1].toDouble(&ok2);

            if (!ok1 || !ok2)
                continue;

            dataset.data[wl] = QVector<double> { v };
            minWl = std::min(minWl, wl);
            maxWl = std::max(maxWl, wl);
        }

        if (!dataset.data.isEmpty()) {
            dataset.header.insert("DESCRIPTOR", "CSV: wavelength_nm,value");
            dataset.header.insert("SPECTRAL_START_NM", minWl);
            dataset.header.insert("SPECTRAL_END_NM", maxWl);
            dataset.loaded = true;
        }

        return dataset;
    }

    if (headers.size() >= 3 && headers[0] == "no" && headers[1] == "name") {
        QVector<int> wavelengths;
        for (int i = 2; i < headers.size(); ++i) {
            bool ok = false;
            int wl = headers[i].toInt(&ok);
            if (!ok) {
                qWarning() << "CsvFile: invalid wavelength column:" << headers[i];
                return dataset;
            }
            wavelengths.append(wl);
        }

        dataset.units = "reflectance sensitivity";

        int minWl = *std::min_element(wavelengths.begin(), wavelengths.end());
        int maxWl = *std::max_element(wavelengths.begin(), wavelengths.end());

        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.isEmpty() || line.startsWith('#'))
                continue;

            QStringList parts = line.split(',', Qt::KeepEmptyParts);
            if (parts.size() != wavelengths.size() + 2)
                continue;

            QString curveName = parts[1];
            dataset.indices << curveName;

            for (int i = 0; i < wavelengths.size(); ++i) {
                bool ok = false;
                double v = parts[i + 2].toDouble(&ok);
                if (!ok)
                    v = 0.0;

                dataset.data[wavelengths[i]].append(v);
            }
        }

        if (!dataset.data.isEmpty()) {
            dataset.header.insert("DESCRIPTOR", "CSV: no,name + wavelength columns");
            dataset.header.insert("SPECTRAL_START_NM", minWl);
            dataset.header.insert("SPECTRAL_END_NM", maxWl);
            dataset.loaded = true;
        }

        return dataset;
    }
    qWarning() << "CsvFile: unsupported CSV format:" << fileName;
    return dataset;
}

bool
CsvFile::write(const Dataset& dataset, const QString& fileName)
{
    if (dataset.data.isEmpty()) {
        qWarning() << "CsvFile::write: dataset has no data";
        return false;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "CsvFile: cannot write file:" << fileName;
        return false;
    }

    QTextStream out(&file);
    out << "no,name";

    QList<int> wavelengths = dataset.data.keys();
    std::sort(wavelengths.begin(), wavelengths.end());

    for (int wl : wavelengths)
        out << "," << wl;

    out << "\n";
    const qsizetype numCurves = dataset.indices.isEmpty() ? (dataset.data.first().size()) : dataset.indices.size();
    for (qsizetype curve = 0; curve < numCurves; ++curve) {
        QString curveName;
        if (curve < dataset.indices.size())
            curveName = dataset.indices[curve];
        else
            curveName = QString("Set %1").arg(curve + 1);
        out << (curve + 1) << "," << curveName;
        for (int wl : wavelengths) {
            const QVector<double>& values = dataset.data.value(wl);
            double v = (curve < values.size()) ? values[curve] : 0.0;
            out << "," << QString::number(v, 'g', 10);
        }

        out << "\n";
    }

    return true;
}
