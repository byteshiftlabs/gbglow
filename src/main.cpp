#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include "core/emulator.h"

using namespace emugbc;

int main(int argc, char* argv[]) {
    std::cout << "EmuGBC - Game Boy Color Emulator\n";
    std::cout << "================================\n\n";
    
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <rom_file> [frames]\n";
        std::cerr << "\nOptions:\n";
        std::cerr << "  frames: Number of frames to run (default: 60)\n";
        return 1;
    }
    
    std::string rom_path = argv[1];
    int max_frames = 60;  // Default: run for 1 second
    
    if (argc >= 3) {
        max_frames = std::atoi(argv[2]);
    }
    
    try {
        Emulator emu;
        
        // Load cartridge
        std::cout << "Loading ROM: " << rom_path << "\n";
        if (!emu.load_rom(rom_path)) {
            std::cerr << "Failed to load ROM file\n";
            return 1;
        }
        
        std::cout << "ROM loaded successfully!\n";
        std::cout << "Running emulation...\n\n";
        
        int frames_rendered = 0;
        auto start_time = std::chrono::steady_clock::now();
        
        // Main emulation loop
        for (int frame = 0; frame < max_frames; frame++) {
            emu.run_frame();
            
            if (emu.ppu().frame_ready()) {
                frames_rendered++;
                emu.ppu().clear_frame_ready();
                
                // Render every 10th frame to terminal
                if (frame % 10 == 0) {
                    std::cout << "\033[2J\033[H";  // Clear screen
                    std::cout << "Frame: " << frame << "/" << max_frames << "\n";
                    std::cout << "PC: 0x" << std::hex << emu.cpu().registers().pc;
                    std::cout << " | Scanline: " << std::dec << (int)emu.ppu().scanline() << "\n";
                    emu.ppu().render_to_terminal();
                }
                
                // Maintain ~60 fps timing
                std::this_thread::sleep_for(std::chrono::milliseconds(16));
            }
        }
        
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        std::cout << "\n\nEmulation complete!\n";
        std::cout << "Frames rendered: " << frames_rendered << "\n";
        std::cout << "Time: " << duration.count() << "ms\n";
        std::cout << "Final PC: 0x" << std::hex << emu.cpu().registers().pc << "\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
