// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 gbglow Contributors
// This file is part of gbglow. See LICENSE for details.

/**
 * Screenshot - Screen capture utility
 * 
 * Captures Game Boy framebuffer and saves as PNG file.
 * Files saved to ~/Pictures/gbglow/ with timestamp naming.
 * 
 * Design:
 * - Triggered by F12 key press
 * - Captures raw 160x144 RGBA framebuffer
 * - Saves as PNG using stb_image_write
 * - Generates filename with timestamp
 * - Creates output directory if needed
 */

#pragma once

#include "../core/types.h"
#include <string>
#include <vector>

namespace gbglow {

/**
 * Screenshot capture utility
 */
class Screenshot {
public:
    Screenshot();
    ~Screenshot() = default;
    
    /**
     * Capture and save framebuffer to PNG file
     * @param framebuffer RGBA pixel data (160x144x4 bytes)
     * @param rom_name Name of currently loaded ROM (for filename)
     * @return true if saved successfully, false on error
     */
    bool capture(const std::vector<u8>& framebuffer, const std::string& rom_name);
    
    /**
     * Get path to last saved screenshot
     * @return Full path to last screenshot file
     */
    const std::string& get_last_screenshot_path() const;
    
private:
    // Last saved screenshot path
    std::string last_screenshot_path_;
    
    // Screenshot directory (~/Pictures/gbglow/)
    std::string screenshot_dir_;
    
    // Game Boy LCD dimensions
    static constexpr int LCD_WIDTH = 160;
    static constexpr int LCD_HEIGHT = 144;
    static constexpr int CHANNELS = 4;  // RGBA
    
    /**
     * Get screenshots directory path
     * Creates directory if it doesn't exist
     * @return Full path to screenshots directory
     */
    std::string get_screenshot_dir() const;
    
    /**
     * Generate filename with timestamp
     * Format: gbglow_ROMNAME_YYYYMMDD_HHMMSS.png
     * @param rom_name Name of ROM for filename
     * @return Generated filename (not full path)
     */
    std::string generate_filename(const std::string& rom_name) const;
    
    /**
     * Extract ROM name from path
     * Removes path and extension
     * @param rom_path Full path to ROM file
     * @return Just the ROM name
     */
    std::string extract_rom_name(const std::string& rom_path) const;
};

} // namespace gbglow
