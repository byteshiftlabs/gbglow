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

#include <iostream>
#include <string>

int main(int argc, const char* argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <rom_file>" << std::endl;
        std::cerr << "\nExample: " << argv[0] << " tetris.gb" << std::endl;
        std::cerr << "\nControls:" << std::endl;
        std::cerr << "  Arrow keys = D-pad" << std::endl;
        std::cerr << "  Z = A button" << std::endl;
        std::cerr << "  X = B button" << std::endl;
        std::cerr << "  Enter = Start" << std::endl;
        std::cerr << "  Shift = Select" << std::endl;
        std::cerr << "  Ctrl+O = Open ROM dialog" << std::endl;
        std::cerr << "  Ctrl+R = Reset emulator" << std::endl;
        std::cerr << "  F1-F9 = Save states" << std::endl;
        std::cerr << "  Shift+F1-F9 = Load states" << std::endl;
        std::cerr << "  F11 = Toggle debugger" << std::endl;
        std::cerr << "  F12 = Capture screenshot" << std::endl;
        std::cerr << "  ESC = Exit" << std::endl;
        return 1;
    }
    
    std::string rom_path = argv[1];
    
    gbglow::Emulator emulator;
    
    if (!emulator.load_rom(rom_path))
    {
        std::cerr << "Failed to load ROM: " << rom_path << std::endl;
        return 1;
    }
    
    std::cout << "Loaded ROM: " << rom_path << std::endl;
    
    // Run emulator with display (game loop)
    emulator.run("gbglow - Game Boy Color Emulator");
    
    return 0;
}
