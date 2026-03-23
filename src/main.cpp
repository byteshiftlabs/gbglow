// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 gbglow Contributors
// This file is part of gbglow. See LICENSE for details.

/**
 * gbglow - Game Boy Color Emulator
 * 
 * An educational Game Boy Color emulator implementation.
 * Every line of code is designed to teach how the hardware works.
 * 
 * Main entry point - demonstrates ROM loading and continuous execution.
 */

#include "core/emulator.h"
#include "core/logging.h"

#include <exception>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>

int main(int argc, const char* argv[])
{
    if (argc < 2)
    {
        std::ostringstream usage;
        usage << "Usage: " << argv[0] << " <rom_file>\n"
              << "Example: " << argv[0] << " tetris.gb\n\n"
              << "Controls:\n"
              << "  Arrow keys = D-pad\n"
              << "  Z = A button\n"
              << "  X = B button\n"
              << "  Enter = Start\n"
              << "  Shift = Select\n"
              << "  Ctrl+O = Open ROM dialog\n"
              << "  Ctrl+R = Reset emulator\n"
              << "  F1-F9 = Save states\n"
              << "  Shift+F1-F9 = Load states\n"
              << "  F11 = Toggle debugger\n"
              << "  F12 = Capture screenshot\n"
              << "  ESC = Exit";
        gbglow::log::error(usage.str());
        return 1;
    }
    
    const std::string rom_path = argv[1];

    std::error_code error;
    if (!std::filesystem::exists(rom_path, error) || error) {
        gbglow::log::error("ROM file does not exist: " + rom_path);
        return 1;
    }
    if (!std::filesystem::is_regular_file(rom_path, error) || error) {
        gbglow::log::error("ROM path is not a regular file: " + rom_path);
        return 1;
    }

    try {
        gbglow::Emulator emulator;

        if (!emulator.load_rom(rom_path))
        {
            gbglow::log::error("Failed to load ROM: " + rom_path);
            return 1;
        }

        gbglow::log::info("Loaded ROM: " + rom_path);

        // Run emulator with display (game loop)
        emulator.run("gbglow - Game Boy Color Emulator");
        return 0;
    } catch (const std::exception& exception) {
        gbglow::log::error(std::string("Fatal error: ") + exception.what());
        return 1;
    } catch (...) {
        gbglow::log::error("Fatal error: unknown exception");
        return 1;
    }
}
