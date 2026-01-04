# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2024 - present Mikael Sundell
# https://github.com/mikaelsundell/specviz

cmake_minimum_required (VERSION 3.27)
project(colorchecker LANGUAGES CXX)

add_executable (colorchecker main.cpp)

set_property (TARGET colorchecker PROPERTY CXX_STANDARD 17)
