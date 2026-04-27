/**
 * Screenshot - Screen capture utility
 * 
 * Implementation of PNG screenshot saving using stb_image_write.
 */

#include "screenshot.h"

// Disable warnings for third-party STB library
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../../external/stb/stb_image_write.h"
#pragma GCC diagnostic pop

#include <iostream>
#include <filesystem>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace gbcrush {

Screenshot::Screenshot()
{
    screenshot_dir_ = get_screenshot_dir();
}

bool Screenshot::capture(const std::vector<u8>& framebuffer, const std::string& rom_name)
{
    // Validate framebuffer size
    const size_t expected_size = LCD_WIDTH * LCD_HEIGHT * CHANNELS;
    if (framebuffer.size() != expected_size) {
        std::cerr << "Invalid framebuffer size: " << framebuffer.size() 
                  << " (expected " << expected_size << ")" << std::endl;
        return false;
    }
    
    // Ensure screenshot directory exists
    try {
        std::filesystem::create_directories(screenshot_dir_);
    } catch (const std::exception& e) {
        std::cerr << "Failed to create screenshot directory: " << e.what() << std::endl;
        return false;
    }
    
    // Generate filename
    std::string filename = generate_filename(rom_name);
    last_screenshot_path_ = screenshot_dir_ + "/" + filename;
    
    // Write PNG file
    // stb_image_write expects data in row-major order, top to bottom
    int result = stbi_write_png(
        last_screenshot_path_.c_str(),
        LCD_WIDTH,
        LCD_HEIGHT,
        CHANNELS,
        framebuffer.data(),
        LCD_WIDTH * CHANNELS  // stride in bytes
    );
    
    if (result == 0) {
        std::cerr << "Failed to write screenshot: " << last_screenshot_path_ << std::endl;
        return false;
    }
    
    std::cout << "Screenshot saved: " << last_screenshot_path_ << std::endl;
    return true;
}

const std::string& Screenshot::get_last_screenshot_path() const
{
    return last_screenshot_path_;
}

std::string Screenshot::get_screenshot_dir() const
{
    // Use ~/Pictures/gbcrush/ on Linux/macOS
    const char* home = std::getenv("HOME");
    std::string base_dir;
    
    if (home) {
        base_dir = std::string(home) + "/Pictures/gbcrush";
    } else {
        // Fallback to current directory
        base_dir = "./screenshots";
    }
    
    return base_dir;
}

std::string Screenshot::generate_filename(const std::string& rom_name) const
{
    // Get current time
    std::time_t now = std::time(nullptr);
    std::tm* local_time = std::localtime(&now);
    
    // Format: gbcrush_ROMNAME_YYYYMMDD_HHMMSS.png
    std::ostringstream filename;
    filename << "gbcrush";
    
    // Add ROM name if provided
    if (!rom_name.empty()) {
        std::string clean_name = extract_rom_name(rom_name);
        if (!clean_name.empty()) {
            filename << "_" << clean_name;
        }
    }
    
    // Add timestamp
    filename << "_"
             << std::setfill('0')
             << std::setw(4) << (local_time->tm_year + 1900)
             << std::setw(2) << (local_time->tm_mon + 1)
             << std::setw(2) << local_time->tm_mday
             << "_"
             << std::setw(2) << local_time->tm_hour
             << std::setw(2) << local_time->tm_min
             << std::setw(2) << local_time->tm_sec
             << ".png";
    
    return filename.str();
}

std::string Screenshot::extract_rom_name(const std::string& rom_path) const
{
    // Find last slash (path separator)
    size_t last_slash = rom_path.find_last_of("/\\");
    std::string filename = (last_slash != std::string::npos) 
        ? rom_path.substr(last_slash + 1) 
        : rom_path;
    
    // Remove extension
    size_t last_dot = filename.find_last_of('.');
    if (last_dot != std::string::npos) {
        filename = filename.substr(0, last_dot);
    }
    
    // Replace spaces and special characters with underscores
    for (char& c : filename) {
        if (!std::isalnum(c) && c != '_' && c != '-') {
            c = '_';
        }
    }
    
    return filename;
}

} // namespace gbcrush
