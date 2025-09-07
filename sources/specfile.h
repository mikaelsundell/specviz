// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - present Mikael Sundell
// https://github.com/mikaelsundell/specviz

#pragma once

#include "specreader.h"
#include "ampasreader.h"
#include "argyllreader.h"
#include "sekonicreader.h"

#include <QFileInfo>
#include <memory>

class SpecFile {
    public:
        explicit SpecFile(const QString& fileName) {
            QString ext = QFileInfo(fileName).suffix().toLower();
            for (auto& factory : availableReaders()) {
                std::unique_ptr<SpecReader> candidate(factory());
                if (candidate) {
                    if (candidate->extensions().contains(ext)) {
                        reader = std::move(candidate);
                        dataset = reader->read(fileName);
                        break;
                    }
                }
            }
        }
        const SpecReader::Dataset& data() const { return dataset; }
        bool isLoaded() const { return dataset.loaded; }

    private:
        using ReaderFactory = std::function<SpecReader*()>;
        static QList<ReaderFactory> availableReaders() {
            return {
                []() { return new AmpasReader(); },
                []() { return new ArgyllReader(); },
                []() { return new SekonicReader(); }
            };
        }
        std::unique_ptr<SpecReader> reader;
        SpecReader::Dataset dataset;
};
