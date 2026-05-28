// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 gbglow Contributors
// This file is part of gbglow. See LICENSE for details.

#pragma once

#include <iostream>
#include <string>

namespace gbglow::log {

inline void write(std::ostream& stream, const char* level, const std::string& message) {
    stream << "[" << level << "] " << message << std::endl;
}

inline void info(const std::string& message) {
    write(std::clog, "INFO", message);
}

inline void warning(const std::string& message) {
    write(std::clog, "WARN", message);
}

inline void error(const std::string& message) {
    write(std::cerr, "ERROR", message);
}

}  // namespace gbglow::log
