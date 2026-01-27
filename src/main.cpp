/**
 * EmuGBC - Game Boy Color Emulator
 * 
 * A crystal-clear, educational Game Boy Color emulator implementation.
 * Every line of code is designed to teach how the hardware works.
 * 
 * Main entry point - demonstrates basic ROM loading and execution.
 */

#include "core/emulator.h"
#include "video/ppu.h"

#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <rom_file>" << std::endl;
        std::cerr << "\nExample: " << argv[0] << " tetris.gb" << std::endl;
        return 1;
    }
    
    std::string rom_path = argv[1];
    
    emugbc::Emulator emulator;
    
    if (!emulator.load_rom(rom_path))
    {
        std::cerr << "Failed to load ROM: " << rom_path << std::endl;
        return 1;
    }
    
    std::cout << "Loaded ROM: " << rom_path << std::endl;
    std::cout << "Running emulator..." << std::endl;
    
    // Run a few frames for demonstration
    // In a real application, this would be a game loop with timing control
    for (int frame = 0; frame < 10; ++frame)
    {
        emulator.run_frame();
        
        // Check if frame is ready for display
        if (emulator.ppu().frame_ready())
        {
            std::cout << "\n=== Frame " << frame << " ===" << std::endl;
            emulator.ppu().render_to_terminal();
            
            // Clear the frame ready flag
            const_cast<emugbc::PPU&>(emulator.ppu()).clear_frame_ready();
        }
    }
    
    std::cout << "\nEmulator test complete." << std::endl;
    
    return 0;
}
