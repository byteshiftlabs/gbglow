// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 gbglow Contributors
// This file is part of gbglow. See LICENSE for details.

/**
 * RecentRoms - Recent ROM files list manager
 * 
 * Implementation of recent ROMs tracking with JSON persistence.
 */

#include "recent_roms.h"

#include <fstream>
#include <algorithm>
#include <filesystem>
#include <iostream>

// Simple JSON writing (no external dependencies)
namespace {

std::string escape_json_string(const std::string& str) {
    std::string result;
    for (char c : str) {
        switch (c) {
            case '"':  result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default:   result += c; break;
        }
    }
    return result;
}

} // anonymous namespace

namespace gbglow {

RecentRomEntry::RecentRomEntry(const std::string& path, std::time_t time)
    : file_path(path)
    , last_played(time)
{
    // Extract just the filename for display
    size_t last_slash = path.find_last_of("/\\");
    display_name = (last_slash != std::string::npos) 
        ? path.substr(last_slash + 1) 
        : path;
}

RecentRoms::RecentRoms()
{
    config_path_ = get_config_dir() + "/recent_roms.json";
    load_from_file();
}

RecentRoms::~RecentRoms()
{
    save_to_file();
}

void RecentRoms::add_rom(const std::string& rom_path)
{
    // Check if ROM already exists in list
    auto it = std::find_if(recent_roms_.begin(), recent_roms_.end(),
        [&rom_path](const RecentRomEntry& entry) {
            return entry.file_path == rom_path;
        });
    
    // Remove if exists (we'll re-add at front)
    if (it != recent_roms_.end()) {
        recent_roms_.erase(it);
    }
    
    // Add to front with current timestamp
    std::time_t now = std::time(nullptr);
    recent_roms_.insert(recent_roms_.begin(), RecentRomEntry(rom_path, now));
    
    // Keep only MAX_RECENT_ROMS
    if (recent_roms_.size() > MAX_RECENT_ROMS) {
        recent_roms_.resize(MAX_RECENT_ROMS);
    }
    
    // Save immediately
    save_to_file();
}

const std::vector<RecentRomEntry>& RecentRoms::get_roms() const
{
    return recent_roms_;
}

void RecentRoms::clear()
{
    recent_roms_.clear();
    save_to_file();
}

bool RecentRoms::is_empty() const
{
    return recent_roms_.empty();
}

void RecentRoms::load_from_file()
{
    std::ifstream file(config_path_);
    if (!file.is_open()) {
        // No config file yet - this is normal on first run
        return;
    }
    
    // Simple JSON parsing (just enough for our needs)
    // Format: { "roms": [ {"path": "...", "time": 123456}, ... ] }
    std::string line;
    bool in_roms_array = false;
    
    while (std::getline(file, line)) {
        // Look for start of roms array
        if (line.find("\"roms\"") != std::string::npos) {
            in_roms_array = true;
            continue;
        }
        
        if (!in_roms_array) continue;
        
        // Parse ROM entry
        size_t path_pos = line.find("\"path\"");
        size_t time_pos = line.find("\"time\"");
        
        if (path_pos != std::string::npos && time_pos != std::string::npos) {
            // Extract path
            size_t path_start = line.find('"', path_pos + 7);
            size_t path_end = line.find('"', path_start + 1);
            std::string path = line.substr(path_start + 1, path_end - path_start - 1);
            
            // Extract timestamp
            size_t time_start = line.find(':', time_pos + 6);
            size_t time_end = line.find_first_of(",}", time_start);
            std::string time_str = line.substr(time_start + 1, time_end - time_start - 1);
            std::time_t time = std::stoll(time_str);
            
            // Only add if file still exists
            if (std::filesystem::exists(path)) {
                recent_roms_.emplace_back(path, time);
            }
        }
        
        // Check for end of array
        if (line.find(']') != std::string::npos) {
            break;
        }
    }
}

void RecentRoms::save_to_file()
{
    // Create config directory if it doesn't exist
    std::filesystem::create_directories(get_config_dir());
    
    std::ofstream file(config_path_);
    if (!file.is_open()) {
        std::cerr << "Warning: Could not save recent ROMs to " << config_path_ << std::endl;
        return;
    }
    
    // Write JSON manually (simple format, no dependencies)
    file << "{\n";
    file << "  \"roms\": [\n";
    
    for (size_t i = 0; i < recent_roms_.size(); ++i) {
        const auto& entry = recent_roms_[i];
        file << "    {\n";
        file << "      \"path\": \"" << escape_json_string(entry.file_path) << "\",\n";
        file << "      \"time\": " << entry.last_played << "\n";
        file << "    }";
        
        if (i < recent_roms_.size() - 1) {
            file << ",";
        }
        file << "\n";
    }
    
    file << "  ]\n";
    file << "}\n";
}

std::string RecentRoms::get_config_dir() const
{
    // Use XDG_CONFIG_HOME if set, otherwise ~/.config
    const char* xdg_config = std::getenv("XDG_CONFIG_HOME");
    std::string base_dir;
    
    if (xdg_config) {
        base_dir = xdg_config;
    } else {
        const char* home = std::getenv("HOME");
#ifdef _WIN32
        if (!home) home = std::getenv("USERPROFILE");
#endif
        if (home) {
            base_dir = std::string(home) + "/.config";
        } else {
            // Fallback to current directory if HOME not set
            base_dir = ".";
        }
    }
    
    return base_dir + "/gbglow";
}

} // namespace gbglow
