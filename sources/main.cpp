// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - present Mikael Sundell
// https://github.com/mikaelsundell/specviz

#include "specviz.h"
#include <QApplication>

int
main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    Specviz specviz;
    specviz.setArguments(QCoreApplication::arguments());
    specviz.show();
    return app.exec();
}
