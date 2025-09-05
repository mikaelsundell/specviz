// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - present Mikael Sundell
// https://github.com/mikaelsundell/specviz

#pragma once

#include "specdataset.h"

class AmpasReader {
    public:
    SpectralDataset read(const QString& fileName);
};
