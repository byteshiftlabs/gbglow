#pragma once

#include "types.h"
#include "cpu.h"
#include "memory.h"
#include "../video/ppu.h"
#include "../video/display.h"
#include "../input/joypad.h"
#include "../cartridge/cartridge.h"

#include <memory>
#include <string>

namespace emugbc {

/**
 * Main emulator class that coordinates all components
 * 
 * The Emulator ties together the CPU, Memory, PPU, and Cartridge
 * into a functioning Game Boy Color system. It manages execution
 * timing and component synchronization.
 */
class Emulator
{
public:
    Emulator();
    
    /**
     * Load a ROM file into the emulator
     * @param path Path to the ROM file
     * @return true if loaded successfully, false otherwise
     */
    bool load_rom(const std::string& path);
    
    /**
     * Reset emulator to initial state
     * Resets CPU registers and clears memory state
     */
    void reset();
    
    /**
     * Execute one complete frame worth of cycles
     * A frame is 70224 cycles at 4.194304 MHz (~59.7 fps)
     */
    void run_frame();
    
    /**
     * Execute a specific number of cycles
     * @param cycles Number of CPU cycles to execute
     */
    void run_cycles(Cycles cycles);
    
    /**
     * Run the emulator with display and input
     * This is the main game loop - runs until window is closed
     * @param window_title Title for the display window
     * @param scale_factor Window scale factor (default 4x)
     */
    void run(const std::string& window_title, int scale_factor = 4);
    
    /**
     * Access to PPU for rendering
     * @return Reference to the PPU component
     */
    const PPU& ppu() const;
    PPU& ppu();
    
    /**
     * Access to CPU for debugging
     * @return Reference to the CPU component
     */
    const CPU& cpu() const;
    CPU& cpu();
    
    /**
     * Access to Joypad for input
     * @return Reference to the Joypad component
     */
    Joypad& joypad();
    
private:
    std::unique_ptr<Memory> memory_;
    std::unique_ptr<CPU> cpu_;
    std::unique_ptr<PPU> ppu_;
    // Note: Joypad is owned by Memory, accessed via memory_->joypad()
    
    // Game Boy refresh rate: ~59.73 Hz
    static constexpr double FRAME_RATE = 59.73;
    static constexpr double FRAME_TIME_MS = 1000.0 / FRAME_RATE;  // ~16.74 ms
    
    // Cycles per frame
    static constexpr Cycles CYCLES_PER_FRAME = 70224;  // 154 scanlines × 456 dots
};

} // namespace emugbc
