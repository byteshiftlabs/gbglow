#pragma once

#include "types.h"
#include "cpu.h"
#include "memory.h"
#include "../video/ppu.h"
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
    
private:
    std::unique_ptr<Memory> memory_;
    std::unique_ptr<CPU> cpu_;
    std::unique_ptr<PPU> ppu_;
};

} // namespace emugbc
