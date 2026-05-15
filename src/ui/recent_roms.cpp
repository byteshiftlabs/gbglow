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
#include <iterator>
#include <regex>
#include <system_error>
#include <unordered_set>

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

std::string unescape_json_string(const std::string& str) {
    std::string result;
    result.reserve(str.size());

    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] != '\\' || i + 1 >= str.size()) {
            result += str[i];
            continue;
        }

        ++i;
        switch (str[i]) {
            case '"': result += '"'; break;
            case '\\': result += '\\'; break;
            case 'n': result += '\n'; break;
            case 'r': result += '\r'; break;
            case 't': result += '\t'; break;
            default:
                result += str[i];
                break;
        }
    }

    return result;
}

bool parse_recent_rom_entry(const std::string& object_text, gbglow::RecentRomEntry& entry) {
    static const std::regex path_pattern(R"regex("path"\s*:\s*"((?:\\.|[^"\\])*)")regex");
    static const std::regex time_pattern(R"regex("time"\s*:\s*([0-9]+))regex");

    std::smatch path_match;
    std::smatch time_match;
    if (!std::regex_search(object_text, path_match, path_pattern) ||
        !std::regex_search(object_text, time_match, time_pattern)) {
        return false;
    }

    try {
        entry = gbglow::RecentRomEntry(
            unescape_json_string(path_match[1].str()),
            static_cast<std::time_t>(std::stoll(time_match[1].str())));
    } catch (const std::exception&) {
        return false;
    }

    return true;
}

bool file_exists_noexcept(const std::string& path) {
    std::error_code error;
    return std::filesystem::exists(path, error) && !error;
}

} // anonymous namespace

namespace gbglow {

using namespace constants::application;

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
    config_path_ = get_config_dir() + "/" + kRecentRomsFileName;
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

    recent_roms_.clear();

    const std::string contents(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());

    const size_t roms_key = contents.find("\"roms\"");
    if (roms_key == std::string::npos) {
        return;
    }

    const size_t array_start = contents.find('[', roms_key);
    const size_t array_end = contents.find(']', array_start);
    if (array_start == std::string::npos || array_end == std::string::npos || array_end <= array_start) {
        return;
    }

    size_t pos = array_start;
    std::unordered_set<std::string> seen_paths;
    while (true) {
        const size_t object_start = contents.find('{', pos);
        if (object_start == std::string::npos || object_start >= array_end) {
            break;
        }

        const size_t object_end = contents.find('}', object_start);
        if (object_end == std::string::npos || object_end > array_end) {
            break;
        }

        RecentRomEntry entry;
        if (parse_recent_rom_entry(contents.substr(object_start, object_end - object_start + 1), entry) &&
            file_exists_noexcept(entry.file_path) &&
            seen_paths.insert(entry.file_path).second) {
            recent_roms_.push_back(entry);
            if (recent_roms_.size() >= MAX_RECENT_ROMS) {
                break;
            }
        }

        pos = object_end + 1;
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
        if (home) {
            base_dir = std::string(home) + "/.config";
        } else {
            // Fallback to current directory if HOME not set
            base_dir = ".";
        }
    }
    
    return base_dir + "/" + kConfigDirectoryName;
}

} // namespace gbglow
