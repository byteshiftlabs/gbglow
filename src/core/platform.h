// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 gbglow Contributors
// This file is part of gbglow. See LICENSE for details.

#pragma once

#include <ctime>

namespace gbglow {

/**
 * Thread-safe localtime wrapper for the supported Ubuntu 24.04 environment.
 */
inline std::tm* portable_localtime(const std::time_t* timer, std::tm* buf) {
    return localtime_r(timer, buf);
}

} // namespace gbglow
