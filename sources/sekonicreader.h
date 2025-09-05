// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - present Mikael Sundell
// https://github.com/mikaelsundell/specviz

#pragma once

#include "specreader.h"

class SeconicReader : public SpecReader {
    public:
        Dataset read(const QString& fileName) override;
        QStringList extensions() override {
            return {"csv", "txt"};
        }
};
