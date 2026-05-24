// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 gbglow Contributors
// This file is part of gbglow. See LICENSE for details.

#pragma once

#include "../core/types.h"

namespace gbglow {

class Memory;

/**
 * Joypad Input Handler
 * 
 * Manages button and direction input for the Game Boy
 * Register P1/JOYP at 0xFF00
 * 
 * Bit layout:
 * Bit 7-6: Not used
 * Bit 5: Select button keys (0=select)
 * Bit 4: Select direction keys (0=select)
 * Bit 3: Down/Start (0=pressed)
 * Bit 2: Up/Select (0=pressed)
 * Bit 1: Left/B (0=pressed)
 * Bit 0: Right/A (0=pressed)
 */
class Joypad {
public:
    explicit Joypad(Memory& memory);
    Joypad(const Joypad&) = delete;
    Joypad& operator=(const Joypad&) = delete;
    Joypad(Joypad&&) = delete;
    Joypad& operator=(Joypad&&) = delete;
    
    // Button input (action buttons)
    void press_a();
    void release_a();
    void press_b();
    void release_b();
    void press_start();
    void release_start();
    void press_select();
    void release_select();
    
    // Direction input (d-pad)
    void press_up();
    void release_up();
    void press_down();
    void release_down();
    void press_left();
    void release_left();
    void press_right();
    void release_right();
    
    // Read joypad register
    u8 read_register() const;
    
    // Write to joypad register (only bits 4-5 writable)
    void write_register(u8 value);
    
private:
    Memory& memory_;
    
    // Button states (true = pressed)
    bool button_a_;
    bool button_b_;
    bool button_start_;
    bool button_select_;
    
    // Direction states (true = pressed)
    bool dpad_up_;
    bool dpad_down_;
    bool dpad_left_;
    bool dpad_right_;
    
    // Register selection bits
    bool select_buttons_;    // Bit 5 (0=selected)
    bool select_directions_; // Bit 4 (0=selected)
    
    // Register constants
    static constexpr u16 REG_JOYP = 0xFF00;
    static constexpr u16 REG_IF = 0xFF0F;
    
    // Bit masks
    static constexpr u8 BIT_SELECT_BUTTONS = 5;
    static constexpr u8 BIT_SELECT_DIRECTIONS = 4;
    static constexpr u8 BIT_DOWN_START = 3;
    static constexpr u8 BIT_UP_SELECT = 2;
    static constexpr u8 BIT_LEFT_B = 1;
    static constexpr u8 BIT_RIGHT_A = 0;
    
    // Input bit mask (bits 0-3)
    static constexpr u8 INPUT_MASK = 0x0F;
    
    // Selection bit mask (bits 4-5)
    static constexpr u8 SELECT_MASK = 0x30;
    
    // Unused bits mask (bits 6-7)
    static constexpr u8 UNUSED_MASK = 0xC0;
    
    // Joypad interrupt bit
    static constexpr u8 JOYPAD_INT_BIT = 0x10;
    
    // Bit value constants
    static constexpr u8 BIT_NOT_PRESSED = 1;
    static constexpr u8 BIT_PRESSED = 0;
    
    // Request joypad interrupt
    void request_interrupt();
};

} // namespace gbglow
