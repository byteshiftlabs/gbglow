// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 gbglow Contributors
// This file is part of gbglow. See LICENSE for details.

#pragma once

#include "types.h"

namespace gbglow {

class Memory;

/**
 * Timer System
 * 
 * Implements Game Boy hardware timers with four configurable frequencies
 * 
 * Registers:
 * DIV  (0xFF04): Divider register - Increments at 16384 Hz, write resets to 0
 * TIMA (0xFF05): Timer counter - Configurable frequency, triggers interrupt on overflow
 * TMA  (0xFF06): Timer modulo - Reload value for TIMA after overflow
 * TAC  (0xFF07): Timer control - Enable/disable and clock select
 * 
 * TAC bit layout:
 * Bit 2: Timer enable (1=enabled, 0=disabled)
 * Bit 1-0: Clock select
 *   00: 4096 Hz (1024 cycles)
 *   01: 262144 Hz (16 cycles)
 *   10: 65536 Hz (64 cycles)
 *   11: 16384 Hz (256 cycles)
 */
class Timer {
public:
    explicit Timer(Memory& memory);
    
    // Update timer state with elapsed cycles
    void step(Cycles cycles);
    
    // Read timer registers
    u8 read_div() const;
    u8 read_tima() const;
    u8 read_tma() const;
    u8 read_tac() const;
    
    // Write timer registers
    void write_div(u8 value);
    void write_tima(u8 value);
    void write_tma(u8 value);
    void write_tac(u8 value);
    
private:
    Memory& memory_;
    
    // Internal counters
    u16 div_counter_;   // DIV increments every 256 cycles
    u16 tima_counter_;  // TIMA increments based on clock select
    
    // Timer registers
    u8 div_;    // Divider register (0xFF04)
    u8 tima_;   // Timer counter (0xFF05)
    u8 tma_;    // Timer modulo (0xFF06)
    u8 tac_;    // Timer control (0xFF07)
    
    // Register addresses
    static constexpr u16 REG_DIV = 0xFF04;
    static constexpr u16 REG_TIMA = 0xFF05;
    static constexpr u16 REG_TMA = 0xFF06;
    static constexpr u16 REG_TAC = 0xFF07;
    static constexpr u16 REG_IF = 0xFF0F;
    
    // TAC bit masks
    static constexpr u8 TAC_ENABLE_BIT = 0x04;
    static constexpr u8 TAC_CLOCK_SELECT_MASK = 0x03;
    
    // Clock frequencies (in CPU cycles per increment)
    static constexpr u16 CLOCK_FREQ_00 = 1024;  // 4096 Hz
    static constexpr u16 CLOCK_FREQ_01 = 16;    // 262144 Hz
    static constexpr u16 CLOCK_FREQ_10 = 64;    // 65536 Hz
    static constexpr u16 CLOCK_FREQ_11 = 256;   // 16384 Hz
    
    // DIV frequency (cycles per increment)
    static constexpr u16 DIV_FREQUENCY = 256;   // 16384 Hz
    
    // Timer interrupt bit in IF register
    static constexpr u8 TIMER_INT_BIT = 0x04;
    
    // Helper functions
    bool is_timer_enabled() const;
    u16 get_clock_frequency() const;
    void request_timer_interrupt();
};

} // namespace gbglow
