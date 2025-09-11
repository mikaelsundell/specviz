// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - present Mikael Sundell
// https://github.com/mikaelsundell/specviz

#include "specio.h"

#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

SpecIO::SpecIO(const QString& fileName)
{
    QString ext = QFileInfo(fileName).suffix().toLower();
    for (auto& factory : availableFiletypes()) {
        std::unique_ptr<SpecFile> candidate(factory());
        if (candidate) {
            if (candidate->extensions().contains(ext)) {
                file = std::move(candidate);
                dataset = file->read(fileName);
                break;
            }
        }
    }
}

QStringList
SpecIO::availableExtensions()
{
    QStringList exts;
    for (auto& factory : availableFiletypes()) {
        std::unique_ptr<SpecFile> file(factory());
        exts.append(file->extensions());
    }
    return exts;
}

bool
SpecIO::write(const SpecFile::Dataset& dataset, const QString& fileName)
{
    QString ext = QFileInfo(fileName).suffix().toLower();
    for (auto& factory : availableFiletypes()) {
        std::unique_ptr<SpecFile> candidate(factory());
        if (candidate && candidate->extensions().contains(ext)) {
            return candidate->write(dataset, fileName);
        }
    }
    return false;
}

QList<SpecIO::FileFactory>
SpecIO::availableFiletypes()
{
    return { []() { return new AmpasFile(); }, []() { return new ArgyllFile(); } };
}
