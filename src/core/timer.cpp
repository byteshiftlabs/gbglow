// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 gbglow Contributors
// This file is part of gbglow. See LICENSE for details.

#include "timer.h"
#include "memory.h"

namespace gbglow {

Timer::Timer(Memory& memory)
    : memory_(memory)
    , div_counter_(0)
    , tima_counter_(0)
    , div_(0)
    , tima_(0)
    , tma_(0)
    , tac_(0)
{
}

void Timer::step(Cycles cycles) {
    // Update DIV register - always increments regardless of timer enable
    div_counter_ += cycles;
    while (div_counter_ >= DIV_FREQUENCY) {
        div_counter_ -= DIV_FREQUENCY;
        div_++;
    }
    
    // Update TIMA register only if timer is enabled
    if (is_timer_enabled()) {
        tima_counter_ += cycles;
        u16 frequency = get_clock_frequency();
        
        while (tima_counter_ >= frequency) {
            tima_counter_ -= frequency;
            
            // Increment TIMA
            if (tima_ == 0xFF) {
                // Overflow: reload from TMA and trigger interrupt
                tima_ = tma_;
                request_timer_interrupt();
            } else {
                tima_++;
            }
        }
    }
}

u8 Timer::read_div() const {
    return div_;
}

u8 Timer::read_tima() const {
    return tima_;
}

u8 Timer::read_tma() const {
    return tma_;
}

u8 Timer::read_tac() const {
    return tac_;
}

void Timer::write_div(u8 value) {
    // Any write to DIV resets it to 0
    (void)value;  // Suppress unused parameter warning
    div_ = 0;
    div_counter_ = 0;
}

void Timer::write_tima(u8 value) {
    tima_ = value;
}

void Timer::write_tma(u8 value) {
    tma_ = value;
}

void Timer::write_tac(u8 value) {
    // Only lower 3 bits are used
    tac_ = value & 0x07;
    
    // Reset TIMA counter when changing configuration
    tima_counter_ = 0;
}

bool Timer::is_timer_enabled() const {
    return (tac_ & TAC_ENABLE_BIT) != 0;
}

u16 Timer::get_clock_frequency() const {
    u8 clock_select = tac_ & TAC_CLOCK_SELECT_MASK;
    
    switch (clock_select) {
        case 0x00:
            return CLOCK_FREQ_00;  // 4096 Hz
        case 0x01:
            return CLOCK_FREQ_01;  // 262144 Hz
        case 0x02:
            return CLOCK_FREQ_10;  // 65536 Hz
        case 0x03:
            return CLOCK_FREQ_11;  // 16384 Hz
        default:
            return CLOCK_FREQ_00;  // Unreachable
    }
}

void Timer::request_timer_interrupt() {
    // Set timer interrupt flag (bit 2 of IF register)
    u8 if_reg = memory_.read(REG_IF);
    memory_.write(REG_IF, if_reg | TIMER_INT_BIT);
}

void Timer::serialize(std::vector<u8>& data) const
{
    // Timer registers (4 bytes)
    data.push_back(div_);
    data.push_back(tima_);
    data.push_back(tma_);
    data.push_back(tac_);
    
    // Internal counters (4 bytes: 2 × u16)
    data.push_back(static_cast<u8>(div_counter_ & 0xFF));
    data.push_back(static_cast<u8>((div_counter_ >> 8) & 0xFF));
    data.push_back(static_cast<u8>(tima_counter_ & 0xFF));
    data.push_back(static_cast<u8>((tima_counter_ >> 8) & 0xFF));
}

void Timer::deserialize(const u8* data, size_t data_size, size_t& offset)
{
    constexpr size_t TIMER_STATE_SIZE = 8;  // 4 registers + 2 u16 counters
    if (offset + TIMER_STATE_SIZE > data_size) return;
    
    // Timer registers — restored directly, bypassing write_div() reset behavior
    div_ = data[offset++];
    tima_ = data[offset++];
    tma_ = data[offset++];
    tac_ = data[offset++];
    
    // Internal counters
    div_counter_ = static_cast<u16>(data[offset]) | (static_cast<u16>(data[offset + 1]) << 8);
    offset += 2;
    tima_counter_ = static_cast<u16>(data[offset]) | (static_cast<u16>(data[offset + 1]) << 8);
    offset += 2;
}

} // namespace gbglow
