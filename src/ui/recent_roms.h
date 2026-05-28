// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 gbglow Contributors
// This file is part of gbglow. See LICENSE for details.

/**
 * RecentRoms - Recent ROM files list manager
 * 
 * Tracks recently opened ROM files for quick access.
 * Persists list to disk across emulator sessions.
 * 
 * Design:
 * - Stores up to MAX_RECENT_ROMS (10) file paths
 * - Saves to JSON config file in user config directory
 * - Updates timestamp on each ROM load
 * - Provides list for UI display
 */

#pragma once

#include "../core/constants.h"

#include <string>
#include <vector>
#include <ctime>

namespace gbglow {

/**
 * Entry in the recent ROMs list
 */
struct RecentRomEntry {
    std::string file_path;      // Full path to the ROM file
    std::string display_name;   // Filename only (for UI display)
    std::time_t last_played = 0;  // Timestamp of last play
    
    RecentRomEntry() = default;
    RecentRomEntry(const std::string& path, std::time_t time);
};

/**
 * Manages the list of recently opened ROM files
 */
class RecentRoms {
public:
    RecentRoms();
    ~RecentRoms();
    RecentRoms(const RecentRoms&) = delete;
    RecentRoms& operator=(const RecentRoms&) = delete;
    RecentRoms(RecentRoms&&) = delete;
    RecentRoms& operator=(RecentRoms&&) = delete;
    
    // Add or update a ROM in the recent list
    // Moves to top if already exists, adds if new
    void add_rom(const std::string& rom_path);
    
    // Get the list of recent ROMs (newest first)
    const std::vector<RecentRomEntry>& get_roms() const;
    
    // Clear all recent ROMs
    void clear();
    
    // Check if list is empty
    bool is_empty() const;
    
private:
    // Maximum number of recent ROMs to track
    static constexpr size_t MAX_RECENT_ROMS = constants::recent_roms::kMaxEntries;
    
    // List of recent ROM entries (sorted newest first)
    std::vector<RecentRomEntry> recent_roms_;
    bool dirty_ = false;
    
    // Path to the config file
    std::string config_path_;
    
    // Load recent ROMs from config file
    void load_from_file();
    
    // Save recent ROMs to config file
    void save_to_file();
    
    // Get the config directory path (creates if needed)
    std::string get_config_dir() const;
};

} // namespace gbglow
