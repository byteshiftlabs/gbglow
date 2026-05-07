// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 gbglow Contributors
// This file is part of gbglow. See LICENSE for details.

#pragma once

#include <ctime>

namespace gbglow {

/**
 * Thread-safe localtime wrapper.
 * Uses localtime_r on POSIX, localtime_s on MSVC.
 */
inline std::tm* portable_localtime(const std::time_t* timer, std::tm* buf) {
#ifdef _WIN32
    return (localtime_s(buf, timer) == 0) ? buf : nullptr;
#else
    return localtime_r(timer, buf);
#endif
}

} // namespace gbglow
