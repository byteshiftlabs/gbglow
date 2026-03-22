// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 gbglow Contributors
// This file is part of gbglow. See LICENSE for details.

#include "constants.h"

namespace gbglow::constants {

namespace application {
const char kName[] = "gbglow";
const char kConfigDirectoryName[] = "gbglow";
const char kKeybindingsFileName[] = "keybindings.conf";
const char kRecentRomsFileName[] = "recent_roms.json";
const char kOpenRomDialogTitle[] = "Open ROM";
const char kScreenshotDirectoryName[] = "gbglow";
}

namespace savestate {
const char kHeader[] = "GBGLOW_STATE";
const std::size_t kHeaderLength = sizeof(kHeader) - 1;
}

}  // namespace gbglow::constants