// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 gbglow Contributors
// This file is part of gbglow. See LICENSE for details.

#pragma once

#include "constants.h"
#include "types.h"
#include "cpu.h"
#include "memory.h"
#include "../video/ppu.h"
#include "../video/display.h"
#include "../input/joypad.h"
#include "../cartridge/cartridge.h"
#include "../debug/debugger.h"
#include "../ui/recent_roms.h"

#include <memory>
#include <string>

namespace gbglow {

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
    Emulator(const Emulator&) = delete;
    Emulator& operator=(const Emulator&) = delete;
    Emulator(Emulator&&) = delete;
    Emulator& operator=(Emulator&&) = delete;
    
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
    PPU& ppu_for_testing();
    
    /**
     * Access to CPU for debugging
     * @return Reference to the CPU component
     */
    const CPU& cpu() const;
    CPU& cpu_for_testing();
    
    /**
     * Access to Joypad for input
     * @return Reference to the Joypad component
     */
    Joypad& joypad();
    const Joypad& joypad() const;
    
    /**
     * Get cartridge from memory
     * @return Pointer to cartridge (may be null)
     */
    Cartridge* cartridge();
    const Cartridge* cartridge() const;

    /**
     * Access to Memory for testing & diagnostics
     * @return Reference to Memory
     */
    const Memory& memory() const;
    Memory& memory_for_testing();
    
    /**
     * Get the save file path for current ROM
     * @return Path to .sav file based on loaded ROM path
     */
    std::string get_save_path() const;
    
    /**
     * Save emulator state to file
     * @param slot Slot number (0-9)
     * @return true if saved successfully
     */
    bool save_state(int slot);
    
    /**
     * Load emulator state from file
     * @param slot Slot number (0-9)
     * @return true if loaded successfully
     */
    [[nodiscard]] bool load_state(int slot);
    
    /**
     * Delete emulator state file
     * @param slot Slot number (0-9)
     * @return true if deleted successfully
     */
    bool delete_state(int slot);
    
    /**
     * Get state file path for a slot
     * @param slot Slot number (0-9)
     * @return Path to state file
     */
    std::string get_state_path(int slot) const;
    
    /**
     * Get debugger instance
     * @return Reference to the debugger
     */
    Debugger& debugger();
    const Debugger& debugger() const;
    
    /**
     * Get recent ROMs list
     * @return Reference to the recent ROMs manager
     */
    RecentRoms& recent_roms();
    const RecentRoms& recent_roms() const;
    
private:
    void attach_display_state(Display& display);
    bool run_emulation_cycles(Display& display, Cycles cycle_budget);
    Cycles execute_component_step();
    bool handle_pending_rom_request(Display& display);
    void handle_reset_request(Display& display);
    void handle_save_state_request(Display& display);
    void handle_load_state_request(Display& display);
    void handle_delete_state_request(Display& display);
    bool handle_debugger_step_request(Display& display);
    void handle_screenshot_request(Display& display);
    void update_display_frame(Display& display, bool clear_frame_ready);
    void update_paused_display(Display& display);
    void run_turbo_mode(Display& display);
    void run_normal_mode(Display& display, float speed_multiplier);
    void save_ram_on_exit();

    std::unique_ptr<Memory> memory_;
    std::unique_ptr<CPU> cpu_;
    std::unique_ptr<PPU> ppu_;
    std::unique_ptr<Debugger> debugger_;
    std::unique_ptr<RecentRoms> recent_roms_;
    // Note: Joypad is owned by Memory, accessed via memory_->joypad()
    
    std::string rom_path_;  // Path to loaded ROM file
    
    // Game Boy refresh rate: ~59.73 Hz
    static constexpr double FRAME_RATE = constants::emulator::kFrameRateHz;
    static constexpr double FRAME_TIME_MS = constants::emulator::kFrameTimeMs;
    
    // Cycles per frame
    static constexpr Cycles CYCLES_PER_FRAME = constants::emulator::kCyclesPerFrame;
};

} // namespace gbglow
