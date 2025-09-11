// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - present Mikael Sundell
// https://github.com/mikaelsundell/specviz

#pragma once

#include "ampasfile.h"
#include "argyllfile.h"

#include <QFileInfo>
#include <memory>

class SpecIO {
public:
    SpecIO(const QString& fileName);
    static QStringList availableExtensions();

    bool isLoaded() const { return dataset.loaded; }
    const SpecFile::Dataset& data() const { return dataset; }

    static bool write(const SpecFile::Dataset& dataset, const QString& fileName);

private:
    using FileFactory = std::function<SpecFile*()>;
    static QList<FileFactory> availableFiletypes();
    std::unique_ptr<SpecFile> file;
    SpecFile::Dataset dataset;
};
