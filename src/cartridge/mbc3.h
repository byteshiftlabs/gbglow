// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 gbglow Contributors
// This file is part of gbglow. See LICENSE for details.

#pragma once

#include "cartridge.h"
#include <ctime>

namespace gbglow {

/**
 * MBC3 (Memory Bank Controller 3)
 * 
 * Used by many Pokémon games and other titles requiring:
 * - Up to 2MB ROM (128 banks of 16KB)
 * - Up to 32KB RAM (4 banks of 8KB)
 * - Real-Time Clock (RTC) for time-based events
 * 
 * Memory regions:
 * 0x0000-0x1FFF: RAM/RTC Enable
 * 0x2000-0x3FFF: ROM Bank Number (7 bits, 0x01-0x7F)
 * 0x4000-0x5FFF: RAM Bank Number (2 bits) or RTC Register Select
 * 0x6000-0x7FFF: Latch Clock Data
 * 
 * RTC Registers (accessed via RAM bank selection 0x08-0x0C):
 * 0x08: RTC Seconds (0-59)
 * 0x09: RTC Minutes (0-59)
 * 0x0A: RTC Hours (0-23)
 * 0x0B: RTC Days Lower (0-255)
 * 0x0C: RTC Days Upper (bit 0) + Halt flag (bit 6) + Day Carry (bit 7)
 */
class MBC3 : public Cartridge {
public:
    explicit MBC3(std::vector<u8> rom_data, size_t ram_size, bool has_rtc);
    
    u8 read(u16 address) const override;
    void write(u16 address, u8 value) override;
    
    /**
     * Save RAM and RTC state to file
     */
    bool save_ram_to_file(const std::string& path) override;
    
    /**
     * Load RAM and RTC state from file
     */
    bool load_ram_from_file(const std::string& path) override;
    
    /**
     * Update RTC if not halted
     * Should be called periodically (e.g., every frame)
     */
    void update_rtc();
    
private:
    // Banking registers
    bool ram_rtc_enabled_;
    u8 rom_bank_;           // 7-bit ROM bank (0x01-0x7F)
    u8 ram_rtc_select_;     // 0x00-0x03 for RAM banks, 0x08-0x0C for RTC registers
    
    // RTC support
    bool has_rtc_;
    bool rtc_latched_;
    u8 latch_data_last_;    // Previous value written to latch register
    
    // RTC registers (latched values)
    u8 rtc_seconds_;        // 0-59
    u8 rtc_minutes_;        // 0-59
    u8 rtc_hours_;          // 0-23
    u8 rtc_days_low_;       // Lower 8 bits of day counter
    u8 rtc_days_high_;      // Bit 0: Upper bit of day counter, Bit 6: Halt, Bit 7: Day carry
    
    // Internal RTC state (not latched)
    std::time_t rtc_base_time_;     // Base time for RTC calculations
    u32 rtc_base_days_;              // Base day offset
    
    // RTC register bit masks
    static constexpr u8 RTC_SECONDS_MAX = 59;
    static constexpr u8 RTC_MINUTES_MAX = 59;
    static constexpr u8 RTC_HOURS_MAX = 23;
    static constexpr u8 RTC_DAYS_HIGH_DAY_BIT = 0x01;      // Bit 0: Day counter bit 8
    static constexpr u8 RTC_DAYS_HIGH_HALT_BIT = 0x40;     // Bit 6: Halt flag
    static constexpr u8 RTC_DAYS_HIGH_CARRY_BIT = 0x80;    // Bit 7: Day overflow carry
    static constexpr u8 RTC_DAY_BIT_8_SHIFT = 8;           // Shift for day counter bit 8
    
    // RTC time calculation constants
    static constexpr u32 SECONDS_PER_MINUTE = 60;
    static constexpr u32 SECONDS_PER_HOUR = 3600;
    static constexpr u32 SECONDS_PER_DAY = 86400;
    static constexpr u8 MINUTES_PER_HOUR = 60;
    static constexpr u8 HOURS_PER_DAY = 24;
    static constexpr u8 DAYS_LOW_MASK = 0xFF;
    static constexpr u16 DAY_BIT_8_MASK = 0x100;
    static constexpr u16 MAX_DAY_COUNT = 511;
    
    // RAM/RTC select values
    static constexpr u8 RAM_BANK_0 = 0x00;
    static constexpr u8 RAM_BANK_1 = 0x01;
    static constexpr u8 RAM_BANK_2 = 0x02;
    static constexpr u8 RAM_BANK_3 = 0x03;
    static constexpr u8 RTC_REG_SECONDS = 0x08;
    static constexpr u8 RTC_REG_MINUTES = 0x09;
    static constexpr u8 RTC_REG_HOURS = 0x0A;
    static constexpr u8 RTC_REG_DAYS_LOW = 0x0B;
    static constexpr u8 RTC_REG_DAYS_HIGH = 0x0C;
    
    // Latch command values
    static constexpr u8 LATCH_WRITE_0 = 0x00;
    static constexpr u8 LATCH_WRITE_1 = 0x01;
    
    // Memory constants
    static constexpr u16 ROM_BANK_SIZE = 0x4000;        // 16KB per ROM bank
    static constexpr u16 RAM_BANK_SIZE = 0x2000;        // 8KB per RAM bank
    static constexpr u16 ROM_BANK_0_END = 0x4000;
    static constexpr u16 ROM_BANK_N_START = 0x4000;
    static constexpr u16 ROM_BANK_N_END = 0x8000;
    static constexpr u16 RAM_START = 0xA000;
    static constexpr u16 RAM_END = 0xC000;
    
    // Register address ranges
    static constexpr u16 REG_RAM_RTC_ENABLE_END = 0x2000;
    static constexpr u16 REG_ROM_BANK_END = 0x4000;
    static constexpr u16 REG_RAM_RTC_SELECT_END = 0x6000;
    static constexpr u16 REG_LATCH_CLOCK_END = 0x8000;
    
    // ROM bank constants
    static constexpr u8 ROM_BANK_MASK = 0x7F;           // 7-bit ROM bank number
    static constexpr u8 ROM_BANK_DEFAULT = 0x01;       // Default ROM bank (can't be 0)
    static constexpr u8 ROM_BANK_ZERO = 0x00;          // Invalid ROM bank value
    
    // RAM/RTC enable constants
    static constexpr u8 RAM_RTC_ENABLE_VALUE = 0x0A;
    static constexpr u8 RAM_RTC_ENABLE_MASK = 0x0F;
    
    // Memory access constants
    static constexpr u8 UNMAPPED_VALUE = 0xFF;          // Value returned for unmapped reads
    
    /**
     * Latch current RTC values
     * Called when latch sequence (0x00 then 0x01) is written to 0x6000-0x7FFF
     */
    void latch_rtc();
    
    /**
     * Read from RTC register
     */
    u8 read_rtc_register() const;
    
    /**
     * Write to RTC register (for setting time)
     */
    void write_rtc_register(u8 value);
};

} // namespace gbglow
