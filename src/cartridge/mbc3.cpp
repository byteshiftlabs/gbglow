// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 gbglow Contributors
// This file is part of gbglow. See LICENSE for details.

#include "mbc3.h"
#include <algorithm>
#include <fstream>

namespace gbglow {

MBC3::MBC3(std::vector<u8> rom_data, size_t ram_size, bool has_rtc)
    : Cartridge(std::move(rom_data), ram_size)
    , ram_rtc_enabled_(false)
    , rom_bank_(ROM_BANK_DEFAULT)
    , ram_rtc_select_(RAM_BANK_0)
    , has_rtc_(has_rtc)
    , rtc_latched_(false)
    , latch_data_last_(UNMAPPED_VALUE)
    , rtc_seconds_(0)
    , rtc_minutes_(0)
    , rtc_hours_(0)
    , rtc_days_low_(0)
    , rtc_days_high_(0)
    , rtc_base_time_(std::time(nullptr))
    , rtc_base_days_(0)
{
}

u8 MBC3::read(u16 address) const
{
    // ROM Bank 0 (0x0000-0x3FFF)
    if (address < ROM_BANK_0_END) {
        return rom_[address];
    }
    
    // ROM Bank N (0x4000-0x7FFF)
    if (address < ROM_BANK_N_END) {
        const size_t rom_offset = (rom_bank_ * ROM_BANK_SIZE) + (address - ROM_BANK_N_START);
        if (rom_offset < rom_.size()) {
            return rom_[rom_offset];
        }
        return UNMAPPED_VALUE;
    }
    
    // RAM or RTC (0xA000-0xBFFF)
    if (address >= RAM_START && address < RAM_END) {
        if (!ram_rtc_enabled_) {
            return UNMAPPED_VALUE;
        }
        
        // Check if RTC register is selected
        if (has_rtc_ && ram_rtc_select_ >= RTC_REG_SECONDS && ram_rtc_select_ <= RTC_REG_DAYS_HIGH) {
            return read_rtc_register();
        }
        
        // RAM bank access
        if (ram_rtc_select_ <= RAM_BANK_3 && !ram_.empty()) {
            const size_t ram_offset = (ram_rtc_select_ * RAM_BANK_SIZE) + (address - RAM_START);
            if (ram_offset < ram_.size()) {
                return ram_[ram_offset];
            }
        }
        
        return UNMAPPED_VALUE;
    }
    
    return UNMAPPED_VALUE;
}

void MBC3::write(u16 address, u8 value)
{
    // RAM/RTC Enable (0x0000-0x1FFF)
    if (address < REG_RAM_RTC_ENABLE_END) {
        ram_rtc_enabled_ = ((value & RAM_RTC_ENABLE_MASK) == RAM_RTC_ENABLE_VALUE);
        return;
    }
    
    // ROM Bank Number (0x2000-0x3FFF)
    if (address < REG_ROM_BANK_END) {
        rom_bank_ = value & ROM_BANK_MASK;
        if (rom_bank_ == ROM_BANK_ZERO) {
            rom_bank_ = ROM_BANK_DEFAULT;
        }
        return;
    }
    
    // RAM Bank Number or RTC Register Select (0x4000-0x5FFF)
    if (address < REG_RAM_RTC_SELECT_END) {
        ram_rtc_select_ = value;
        return;
    }
    
    // Latch Clock Data (0x6000-0x7FFF)
    if (address < REG_LATCH_CLOCK_END) {
        if (has_rtc_) {
            // Latch on 0x00 -> 0x01 transition
            if (latch_data_last_ == LATCH_WRITE_0 && value == LATCH_WRITE_1) {
                latch_rtc();
            }
            latch_data_last_ = value;
        }
        return;
    }
    
    // RAM or RTC Write (0xA000-0xBFFF)
    if (address >= RAM_START && address < RAM_END) {
        if (!ram_rtc_enabled_) {
            return;
        }
        
        // Check if RTC register is selected
        if (has_rtc_ && ram_rtc_select_ >= RTC_REG_SECONDS && ram_rtc_select_ <= RTC_REG_DAYS_HIGH) {
            write_rtc_register(value);
            return;
        }
        
        // RAM bank access
        if (ram_rtc_select_ <= RAM_BANK_3 && !ram_.empty()) {
            const size_t ram_offset = (ram_rtc_select_ * RAM_BANK_SIZE) + (address - RAM_START);
            if (ram_offset < ram_.size()) {
                ram_[ram_offset] = value;
            }
        }
    }
}

void MBC3::latch_rtc()
{
    if (!rtc_latched_) {
        // Calculate elapsed time since base time
        const std::time_t current_time = std::time(nullptr);
        const std::time_t elapsed_seconds = current_time - rtc_base_time_;
        
        // Check if RTC is halted
        if ((rtc_days_high_ & RTC_DAYS_HIGH_HALT_BIT) == 0) {
            // RTC is running, update latched values
            const u32 total_seconds = static_cast<u32>(elapsed_seconds);
            
            rtc_seconds_ = total_seconds % SECONDS_PER_MINUTE;
            rtc_minutes_ = (total_seconds / SECONDS_PER_MINUTE) % MINUTES_PER_HOUR;
            rtc_hours_ = (total_seconds / SECONDS_PER_HOUR) % HOURS_PER_DAY;
            
            const u32 elapsed_days = total_seconds / SECONDS_PER_DAY;
            const u32 total_days = rtc_base_days_ + elapsed_days;
            
            rtc_days_low_ = total_days & DAYS_LOW_MASK;
            
            // Handle day counter overflow (bit 8)
            if (total_days & DAY_BIT_8_MASK) {
                rtc_days_high_ |= RTC_DAYS_HIGH_DAY_BIT;
            } else {
                rtc_days_high_ &= ~RTC_DAYS_HIGH_DAY_BIT;
            }
            
            // Set carry flag if day counter exceeded max
            if (total_days > MAX_DAY_COUNT) {
                rtc_days_high_ |= RTC_DAYS_HIGH_CARRY_BIT;
            }
        }
        
        rtc_latched_ = true;
    }
}

u8 MBC3::read_rtc_register() const
{
    switch (ram_rtc_select_) {
        case RTC_REG_SECONDS:
            return rtc_seconds_;
        case RTC_REG_MINUTES:
            return rtc_minutes_;
        case RTC_REG_HOURS:
            return rtc_hours_;
        case RTC_REG_DAYS_LOW:
            return rtc_days_low_;
        case RTC_REG_DAYS_HIGH:
            return rtc_days_high_;
        default:
            return UNMAPPED_VALUE;
    }
}

void MBC3::write_rtc_register(u8 value)
{
    // Writing to RTC registers updates the internal state
    // and resets the base time for future calculations
    rtc_latched_ = false;
    
    switch (ram_rtc_select_) {
        case RTC_REG_SECONDS:
            rtc_seconds_ = std::min(value, RTC_SECONDS_MAX);
            break;
        case RTC_REG_MINUTES:
            rtc_minutes_ = std::min(value, RTC_MINUTES_MAX);
            break;
        case RTC_REG_HOURS:
            rtc_hours_ = std::min(value, RTC_HOURS_MAX);
            break;
        case RTC_REG_DAYS_LOW:
            rtc_days_low_ = value;
            break;
        case RTC_REG_DAYS_HIGH:
            rtc_days_high_ = value;
            break;
    }
    
    // Update base time and days when RTC registers are modified
    rtc_base_time_ = std::time(nullptr);
    const u32 current_days = rtc_days_low_ | ((rtc_days_high_ & RTC_DAYS_HIGH_DAY_BIT) << RTC_DAY_BIT_8_SHIFT);
    rtc_base_days_ = current_days;
}

void MBC3::update_rtc()
{
    // RTC update is handled during latching
    // This function exists for future frame-based updates if needed
}

bool MBC3::save_ram_to_file(const std::string& path) {
    // Only save if cartridge has battery
    if (!has_battery_) {
        return false;
    }
    
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }
    
    // Write RAM data first
    if (!ram_.empty()) {
        if (!file.write(reinterpret_cast<const char*>(ram_.data()), ram_.size())) {
            return false;
        }
    }
    
    // Write RTC state if present
    if (has_rtc_) {
        // Save latched RTC registers
        file.put(rtc_seconds_);
        file.put(rtc_minutes_);
        file.put(rtc_hours_);
        file.put(rtc_days_low_);
        file.put(rtc_days_high_);
        
        // Save base time for calculations
        const auto base_time_value = static_cast<int64_t>(rtc_base_time_);
        file.write(reinterpret_cast<const char*>(&base_time_value), sizeof(base_time_value));
        
        // Save base days
        file.write(reinterpret_cast<const char*>(&rtc_base_days_), sizeof(rtc_base_days_));
        
        if (!file) {
            return false;
        }
    }
    
    return true;
}

bool MBC3::load_ram_from_file(const std::string& path) {
    // Only load if cartridge has battery
    if (!has_battery_) {
        return false;
    }
    
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        // File doesn't exist - not an error, just no save data yet
        return false;
    }
    
    // Read RAM data first
    if (!ram_.empty()) {
        if (!file.read(reinterpret_cast<char*>(ram_.data()), ram_.size())) {
            // File exists but wrong size or read error
            return false;
        }
    }
    
    // Read RTC state if present
    if (has_rtc_) {
        // Read latched RTC registers
        int c;
        if ((c = file.get()) == EOF) return true;  // Partial save file (RAM only)
        rtc_seconds_ = static_cast<u8>(c);
        
        if ((c = file.get()) == EOF) return true;
        rtc_minutes_ = static_cast<u8>(c);
        
        if ((c = file.get()) == EOF) return true;
        rtc_hours_ = static_cast<u8>(c);
        
        if ((c = file.get()) == EOF) return true;
        rtc_days_low_ = static_cast<u8>(c);
        
        if ((c = file.get()) == EOF) return true;
        rtc_days_high_ = static_cast<u8>(c);
        
        // Read base time
        int64_t base_time_value = 0;
        if (!file.read(reinterpret_cast<char*>(&base_time_value), sizeof(base_time_value))) {
            return true;  // Partial save (no timestamp)
        }
        rtc_base_time_ = static_cast<std::time_t>(base_time_value);
        
        // Read base days
        if (!file.read(reinterpret_cast<char*>(&rtc_base_days_), sizeof(rtc_base_days_))) {
            return true;  // Partial save (no base days)
        }
    }
    
    return true;
}

} // namespace gbglow
