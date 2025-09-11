// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - present Mikael Sundell
// https://github.com/mikaelsundell/specviz

#pragma once

#include "specfile.h"

class ArgyllFile : public SpecFile {
public:
    Dataset read(const QString& fileName) override;
    bool write(const Dataset& dataset, const QString& fileName) override;
    QStringList extensions() override { return { "agryll", "cgats", "sp", "ti3" }; }
};
