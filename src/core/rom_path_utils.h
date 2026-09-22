// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 gbglow Contributors
// This file is part of gbglow. See LICENSE for details.

#pragma once

#include <filesystem>
#include <string>

namespace gbglow {

/**
 * Derive a sibling path for a loaded ROM by replacing its extension with a
 * new suffix - used for the .sav file and the numbered save-state files.
 *
 * Matches the extension against the filename only, via std::filesystem, so a
 * ROM with no extension inside a dotted directory (e.g.
 * "/home/user/my.roms/pokemon") keeps its directory intact instead of having
 * the directory name mistaken for the extension.
 *
 * @param rom_path Path to the loaded ROM file.
 * @param suffix Suffix to append after stripping the extension, e.g. ".sav"
 *               or ".slot1.state".
 * @return rom_path with its extension replaced by suffix, or an empty string
 *         if rom_path is empty.
 */
inline std::string derive_rom_sibling_path(const std::string& rom_path, const std::string& suffix) {
    if (rom_path.empty()) {
        return {};
    }

    std::filesystem::path path(rom_path);
    path.replace_extension();
    return path.string() + suffix;
}

} // namespace gbglow
