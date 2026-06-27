// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 gbglow Contributors
// This file is part of gbglow. See LICENSE for details.

#include "apu.h"
#include "../core/memory.h"
#include "../core/io_registers.h"
#include <algorithm>
#include <iostream>
#include <mutex>

namespace gbglow {

using namespace io_reg;

// Noise table generation constants
static constexpr int NOISE7_TABLE_SIZE = 16;      // 128 bits = 16 bytes
static constexpr int NOISE15_TABLE_SIZE = 4096;   // 32768 bits = 4096 bytes
static constexpr int BITS_PER_BYTE = 8;
static constexpr u16 LFSR7_INIT = 0x7F;           // Initial state for 7-bit LFSR
static constexpr u16 LFSR15_INIT = 0x7FFF;        // Initial state for 15-bit LFSR
static constexpr int LFSR7_FEEDBACK_BIT = 6;      // Feedback position for 7-bit
static constexpr int LFSR15_FEEDBACK_BIT = 14;    // Feedback position for 15-bit

// Noise table lookup constants
static constexpr int NOISE_BYTE_SHIFT = 20;       // Shift for byte index
static constexpr int NOISE_BIT_SHIFT = 17;        // Shift for bit position
static constexpr int NOISE7_BYTE_MASK = 15;       // Mask for 7-bit table (16 bytes)
static constexpr int NOISE15_BYTE_MASK = 4095;    // Mask for 15-bit table (4096 bytes)
static constexpr int NOISE_BIT_MASK = 7;          // Mask for bit position (0-7)

// Noise frequency calculation constants
static constexpr int NOISE_FREQ_BASE = 1 << 14;   // Base frequency for noise channel
static constexpr int NOISE_FREQ_DIV_2 = NOISE_FREQ_BASE;
static constexpr int NOISE_FREQ_DIV_3 = NOISE_FREQ_BASE / 3;
static constexpr int NOISE_FREQ_DIV_4 = NOISE_FREQ_BASE / 4;
static constexpr int NOISE_FREQ_DIV_5 = NOISE_FREQ_BASE / 5;
static constexpr int NOISE_FREQ_DIV_6 = NOISE_FREQ_BASE / 6;
static constexpr int NOISE_FREQ_DIV_7 = NOISE_FREQ_BASE / 7;
static constexpr int NOISE_AMPLIFY_FACTOR = 3;    // Noise amplitude multiplier

// Precomputed noise tables for Channel 4 (LFSR-based pseudo-random noise)
static u8 noise7[NOISE7_TABLE_SIZE];
static u8 noise15[NOISE15_TABLE_SIZE];

static std::once_flag noise_tables_flag;

// Generate precomputed noise tables using LFSR algorithm
static void init_noise_tables() {
    
    // Generate 7-bit LFSR noise table
    u16 lfsr = LFSR7_INIT;
    for (int i = 0; i < NOISE7_TABLE_SIZE; i++) {
        u8 byte = 0;
        for (int bit = 0; bit < BITS_PER_BYTE; bit++) {
            int feedback = ((lfsr & 1) ^ ((lfsr >> 1) & 1));
            lfsr = (lfsr >> 1) | (feedback << LFSR7_FEEDBACK_BIT);
            byte |= ((lfsr & 1) << (BITS_PER_BYTE - 1 - bit));
        }
        noise7[i] = byte;
    }
    
    // Generate 15-bit LFSR noise table
    lfsr = LFSR15_INIT;
    for (int i = 0; i < NOISE15_TABLE_SIZE; i++) {
        u8 byte = 0;
        for (int bit = 0; bit < BITS_PER_BYTE; bit++) {
            int feedback = ((lfsr & 1) ^ ((lfsr >> 1) & 1));
            lfsr = (lfsr >> 1) | (feedback << LFSR15_FEEDBACK_BIT);
            byte |= ((lfsr & 1) << (BITS_PER_BYTE - 1 - bit));
        }
        noise15[i] = byte;
    }
}

APU::APU(Memory& memory)
    : memory_(memory)
    , nr50_(0)
    , nr51_(0)
    , nr52_(0)
    , cycle_accumulator_(0)
{
    // Initialize noise lookup tables (thread-safe, only done once)
    std::call_once(noise_tables_flag, init_noise_tables);
    
    // Initialize Wave RAM with default pattern (similar to Game Boy boot state)
    // This is a simple triangle-ish wave pattern
    wave_ram_ = {{
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
        0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
    }};
    audio_buffer_.reserve(SAMPLE_RATE / FRAMES_PER_SECOND);  // Reserve space for one frame
    reset();
}

void APU::reset() {
    // Initialize channel states
    channel1_ = {};
    channel2_ = {};
    channel3_ = {};
    channel4_ = {};
    
    // Reset control registers
    nr50_ = 0;
    nr51_ = 0;
    nr52_ = NR52_RESET_VALUE;
    
    cycle_accumulator_ = 0;
}

void APU::step(Cycles cycles) {
    // All audio processing stops when APU power is off (NR52 bit 7)
    if ((nr52_ & NR52_POWER_BIT) == 0) {
        return;
    }
    
    cycle_accumulator_ += cycles;
    
    // Hardware-accurate approach: length counters, envelopes, and sweep are applied
    // per-sample inside generate_sample() using RATE-based accumulators.
    // This eliminates the need for a separate 512Hz frame sequencer.
    
    // Generate audio samples as CPU cycles accumulate
    while (cycle_accumulator_ >= CYCLES_PER_SAMPLE) {
        auto sample = generate_sample();
        audio_buffer_.push_back(sample);
        cycle_accumulator_ -= CYCLES_PER_SAMPLE;
    }
}

// Square wave duty cycle patterns for each of the 4 duty cycle settings.
// Each pattern has 8 steps: -1 = high, 0 = low.
// These patterns match the hardware audio behavior for compatibility.
static const int sqwave[4][8] = {
    {  0, 0,-1, 0, 0, 0, 0, 0 },  // 12.5% duty
    {  0,-1,-1, 0, 0, 0, 0, 0 },  // 25% duty
    { -1,-1,-1,-1, 0, 0, 0, 0 },  // 50% duty
    { -1, 0, 0,-1,-1,-1,-1,-1 }   // 75% duty
};


std::pair<u8, u8> APU::generate_sample() {
    int left = 0;
    int right = 0;
    
    process_channel1_sample(left, right);
    process_channel2_sample(left, right);
    process_channel3_sample(left, right);
    process_channel4_sample(left, right);
    
    // Apply master volume from NR50 (bits 0-2 for right, bits 4-6 for left)
    left *= ((nr50_ >> NR50_LEFT_VOLUME_SHIFT) & NR50_RIGHT_VOLUME_MASK);
    right *= (nr50_ & NR50_RIGHT_VOLUME_MASK);
    left >>= SAMPLE_VOLUME_SHIFT;
    right >>= SAMPLE_VOLUME_SHIFT;
    
    // Clamp to signed 8-bit range
    if (left > SAMPLE_MAX) {
        left = SAMPLE_MAX;
    } else if (left < SAMPLE_MIN) {
        left = SAMPLE_MIN;
    }
    
    if (right > SAMPLE_MAX) {
        right = SAMPLE_MAX;
    } else if (right < SAMPLE_MIN) {
        right = SAMPLE_MIN;
    }
    
    // Convert from signed to unsigned for SDL audio
    // In this format, SAMPLE_ZERO (128) represents silence (0 amplitude)
    return {static_cast<u8>(left + SAMPLE_ZERO), static_cast<u8>(right + SAMPLE_ZERO)};
}

void APU::process_channel1_sample(int& left, int& right) {
    if (!channel1_.on) {
        return;
    }
    
    int sample = sqwave[channel1_.wave_duty][(channel1_.phase_position >> PHASE_SHIFT_SQUARE) & PHASE_MASK_SQUARE] & channel1_.envelope_volume;
    channel1_.phase_position += channel1_.phase_increment;
    
    update_channel1_length_counter();
    update_channel1_envelope();
    update_channel1_sweep();
    
    sample <<= SAMPLE_AMPLIFY_SHIFT;
    if (nr51_ & NR51_CH1_RIGHT) right += sample;
    if (nr51_ & NR51_CH1_LEFT) left += sample;
}

void APU::update_channel1_length_counter() {
    if (!channel1_.length_enable) {
        return;
    }
    
    channel1_.length_counter += RATE;
    if (channel1_.length_counter >= channel1_.length_threshold) {
        channel1_.on = false;
    }
}

void APU::update_channel1_envelope() {
    if (!channel1_.envelope_threshold) {
        return;
    }
    
    channel1_.envelope_counter += RATE;
    if (channel1_.envelope_counter < channel1_.envelope_threshold) {
        return;
    }
    
    channel1_.envelope_counter -= channel1_.envelope_threshold;
    channel1_.envelope_volume += channel1_.envelope_direction_state;
    
    if (channel1_.envelope_volume < ENVELOPE_MIN) {
        channel1_.envelope_volume = ENVELOPE_MIN;
    }
    if (channel1_.envelope_volume > ENVELOPE_MAX) {
        channel1_.envelope_volume = ENVELOPE_MAX;
    }
}

void APU::update_channel1_sweep() {
    if (!channel1_.sweep_threshold) {
        return;
    }
    
    channel1_.sweep_counter += RATE;
    if (channel1_.sweep_counter < channel1_.sweep_threshold) {
        return;
    }
    
    channel1_.sweep_counter -= channel1_.sweep_threshold;
    int frequency = channel1_.sweep_frequency;
    int shift = channel1_.sweep_shift;
    
    if (channel1_.sweep_direction) {
        frequency -= (frequency >> shift);
    } else {
        frequency += (frequency >> shift);
    }
    
    if (frequency > SWEEP_FREQUENCY_MAX) {
        channel1_.on = false;
        return;
    }
    
    channel1_.sweep_frequency = frequency;
    int divisor = FREQ_DIVISOR_BASE - frequency;
    if (RATE > (divisor << FREQ_CHECK_SHIFT_SQUARE)) {
        channel1_.phase_increment = 0;
    } else {
        channel1_.phase_increment = (RATE << FREQ_SHIFT_SQUARE) / divisor;
    }
}

void APU::process_channel2_sample(int& left, int& right) {
    if (!channel2_.on) {
        return;
    }
    
    int sample = sqwave[channel2_.wave_duty][(channel2_.phase_position >> PHASE_SHIFT_SQUARE) & PHASE_MASK_SQUARE] & channel2_.envelope_volume;
    channel2_.phase_position += channel2_.phase_increment;
    
    update_channel2_length_counter();
    update_channel2_envelope();
    
    sample <<= SAMPLE_AMPLIFY_SHIFT;
    if (nr51_ & NR51_CH2_RIGHT) right += sample;
    if (nr51_ & NR51_CH2_LEFT) left += sample;
}

void APU::update_channel2_length_counter() {
    if (!channel2_.length_enable) {
        return;
    }
    
    channel2_.length_counter += RATE;
    if (channel2_.length_counter >= channel2_.length_threshold) {
        channel2_.on = false;
    }
}

void APU::update_channel2_envelope() {
    if (!channel2_.envelope_threshold) {
        return;
    }
    
    channel2_.envelope_counter += RATE;
    if (channel2_.envelope_counter < channel2_.envelope_threshold) {
        return;
    }
    
    channel2_.envelope_counter -= channel2_.envelope_threshold;
    channel2_.envelope_volume += channel2_.envelope_direction_state;
    
    if (channel2_.envelope_volume < ENVELOPE_MIN) {
        channel2_.envelope_volume = ENVELOPE_MIN;
    }
    if (channel2_.envelope_volume > ENVELOPE_MAX) {
        channel2_.envelope_volume = ENVELOPE_MAX;
    }
}

void APU::process_channel3_sample(int& left, int& right) {
    if (!channel3_.on) {
        return;
    }
    
    int sample = wave_ram_[(channel3_.phase_position >> PHASE_SHIFT_WAVE) & PHASE_MASK_WAVE];
    
    if (channel3_.phase_position & (1 << WAVE_NIBBLE_BIT)) {
        sample &= WAVE_NIBBLE_MASK;
    } else {
        sample >>= WAVE_NIBBLE_SHIFT;
    }
    sample -= SAMPLE_WAVE_OFFSET;
    
    channel3_.phase_position += channel3_.phase_increment;
    
    update_channel3_length_counter();
    
    if (channel3_.output_level > 0) {
        sample <<= (WAVE_VOLUME_BASE - channel3_.output_level);
    } else {
        sample = 0;
    }
    
    if (nr51_ & NR51_CH3_RIGHT) right += sample;
    if (nr51_ & NR51_CH3_LEFT) left += sample;
}

void APU::update_channel3_length_counter() {
    if (!channel3_.length_enable) {
        return;
    }
    
    channel3_.length_counter += RATE;
    if (channel3_.length_counter >= channel3_.length_threshold) {
        channel3_.on = false;
    }
}

void APU::process_channel4_sample(int& left, int& right) {
    if (!channel4_.on) {
        return;
    }
    
    int sample = calculate_noise_sample();
    channel4_.phase_position += channel4_.phase_increment;
    
    update_channel4_length_counter();
    update_channel4_envelope();
    
    sample += sample << 1;
    if (nr51_ & NR51_CH4_RIGHT) right += sample;
    if (nr51_ & NR51_CH4_LEFT) left += sample;
}

int APU::calculate_noise_sample() {
    int byte_index;
    int bit_pos;
    int sample;
    
    if (channel4_.width_mode) {
        byte_index = (channel4_.phase_position >> NOISE_BYTE_SHIFT) & NOISE7_BYTE_MASK;
        bit_pos = (NOISE_BIT_MASK - ((channel4_.phase_position >> NOISE_BIT_SHIFT) & NOISE_BIT_MASK));
        sample = (noise7[byte_index] >> bit_pos) & 1;
    } else {
        byte_index = (channel4_.phase_position >> NOISE_BYTE_SHIFT) & NOISE15_BYTE_MASK;
        bit_pos = (NOISE_BIT_MASK - ((channel4_.phase_position >> NOISE_BIT_SHIFT) & NOISE_BIT_MASK));
        sample = (noise15[byte_index] >> bit_pos) & 1;
    }
    
    return (-sample) & channel4_.envelope_volume;
}

void APU::update_channel4_length_counter() {
    if (!channel4_.length_enable) {
        return;
    }
    
    channel4_.length_counter += RATE;
    if (channel4_.length_counter >= channel4_.length_threshold) {
        channel4_.on = false;
    }
}

void APU::update_channel4_envelope() {
    if (!channel4_.envelope_threshold) {
        return;
    }
    
    channel4_.envelope_counter += RATE;
    if (channel4_.envelope_counter < channel4_.envelope_threshold) {
        return;
    }
    
    channel4_.envelope_counter -= channel4_.envelope_threshold;
    channel4_.envelope_volume += channel4_.envelope_direction_state;
    
    if (channel4_.envelope_volume < ENVELOPE_MIN) {
        channel4_.envelope_volume = ENVELOPE_MIN;
    }
    if (channel4_.envelope_volume > ENVELOPE_MAX) {
        channel4_.envelope_volume = ENVELOPE_MAX;
    }
}

int APU::calculate_noise_frequency(u8 divisor_code, u8 clock_shift) {
    static const int freq_table[8] = {
        NOISE_FREQ_BASE * 2,
        NOISE_FREQ_DIV_2,
        NOISE_FREQ_DIV_2 / 2,
        NOISE_FREQ_DIV_3,
        NOISE_FREQ_DIV_4,
        NOISE_FREQ_DIV_5,
        NOISE_FREQ_DIV_6,
        NOISE_FREQ_DIV_7
    };
    
    int frequency = (freq_table[divisor_code] >> clock_shift) * RATE;
    if (frequency >> LFSR_PHASE_THRESHOLD) {
        frequency = 1 << LFSR_PHASE_THRESHOLD;
    }
    return frequency;
}

u8 APU::read_register(u16 address) const {
    // Read from APU registers (0xFF10-0xFF3F)
    // Some bits are write-only and return 1 when read
    
    // Channel 1 registers
    if (address == REG_NR10) {
        return REG_NR10_UNUSED_BITS | (channel1_.sweep_period << SWEEP_PERIOD_SHIFT) | 
               (channel1_.sweep_direction << SWEEP_DIRECTION_SHIFT) | channel1_.sweep_shift;
    }
    if (address == REG_NR11) {
        return (channel1_.wave_duty << DUTY_CYCLE_SHIFT) | REG_NR11_UNUSED_BITS;
    }
    if (address == REG_NR12) {
        return (channel1_.initial_volume << VOLUME_SHIFT) | 
               (channel1_.envelope_direction << ENVELOPE_DIRECTION_SHIFT) | channel1_.envelope_period;
    }
    if (address == REG_NR13) {
        return REG_WRITE_ONLY;  // Write-only
    }
    if (address == REG_NR14) {
        return REG_NR14_UNUSED_BITS | (channel1_.length_enable ? LENGTH_ENABLE_BIT : 0x00);
    }
    
    // Channel 2 registers
    if (address == REG_NR21) {
        return (channel2_.wave_duty << DUTY_CYCLE_SHIFT) | REG_NR21_UNUSED_BITS;
    }
    if (address == REG_NR22) {
        return (channel2_.initial_volume << VOLUME_SHIFT) | 
               (channel2_.envelope_direction << ENVELOPE_DIRECTION_SHIFT) | channel2_.envelope_period;
    }
    if (address == REG_NR23) {
        return REG_WRITE_ONLY;  // Write-only
    }
    if (address == REG_NR24) {
        return REG_NR24_UNUSED_BITS | (channel2_.length_enable ? LENGTH_ENABLE_BIT : 0x00);
    }
    
    // Channel 3 registers
    if (address == REG_NR30) {
        return REG_NR30_UNUSED_BITS | (channel3_.dac_enabled ? DAC_ENABLE_BIT : 0x00);
    }
    if (address == REG_NR31) {
        return REG_WRITE_ONLY;  // Write-only
    }
    if (address == REG_NR32) {
        return REG_NR32_UNUSED_BITS | (channel3_.output_level << OUTPUT_LEVEL_SHIFT);
    }
    if (address == REG_NR33) {
        return REG_WRITE_ONLY;  // Write-only
    }
    if (address == REG_NR34) {
        return REG_NR34_UNUSED_BITS | (channel3_.length_enable ? LENGTH_ENABLE_BIT : 0x00);
    }
    
    // Channel 4 registers
    if (address == REG_NR41) {
        return REG_WRITE_ONLY;  // Write-only
    }
    if (address == REG_NR42) {
        return (channel4_.initial_volume << VOLUME_SHIFT) | 
               (channel4_.envelope_direction << ENVELOPE_DIRECTION_SHIFT) | channel4_.envelope_period;
    }
    if (address == REG_NR43) {
        return (channel4_.clock_shift << CLOCK_SHIFT_SHIFT) | 
               (channel4_.width_mode << WIDTH_MODE_SHIFT) | channel4_.divisor_code;
    }
    if (address == REG_NR44) {
        return REG_NR44_UNUSED_BITS | (channel4_.length_enable ? LENGTH_ENABLE_BIT : 0x00);
    }
    
    // Control registers
    if (address == REG_NR50) {
        return nr50_;
    }
    if (address == REG_NR51) {
        return nr51_;
    }
    if (address == REG_NR52) {
        // NR52: bits 7-4 are read/write, bits 3-0 show channel on/off status
        u8 status = nr52_ & REG_NR52_UNUSED_BITS;
        if (channel1_.on) status |= NR52_CH1_ON;
        if (channel2_.on) status |= NR52_CH2_ON;
        if (channel3_.on) status |= NR52_CH3_ON;
        if (channel4_.on) status |= NR52_CH4_ON;
        return status;
    }
    
    // Wave RAM (0xFF30-0xFF3F)
    if (address >= WAVE_RAM_START && address <= WAVE_RAM_END) {
        // Hardware behavior: when Channel 3 is active, only the currently
        // playing byte can be read (others return 0xFF)
        if (channel3_.on) {
            int current_byte_index = (channel3_.phase_position >> PHASE_SHIFT_WAVE) & PHASE_MASK_WAVE;
            return wave_ram_[current_byte_index];
        }
        return wave_ram_[address - WAVE_RAM_START];
    }
    
    return REG_WRITE_ONLY;
}

void APU::write_register(u16 address, u8 value) {
    // Hardware behavior: all register writes are ignored when APU is powered off,
    // except writes to NR52 itself (which can turn the APU back on)
    if ((nr52_ & NR52_POWER_BIT) == 0 && address != REG_NR52) {
        return;
    }

    // Wave RAM (0xFF30-0xFF3F)
    // Hardware behavior: can only be written when Channel 3 is off
    if (address >= WAVE_RAM_START && address <= WAVE_RAM_END) {
        if (!channel3_.on) {
            wave_ram_[address - WAVE_RAM_START] = value;
        }
        return;
    }

    // Channel 1: Sweep register (NR10)
    if (address == REG_NR10) {
        channel1_.sweep_period = (value >> SWEEP_PERIOD_SHIFT) & SWEEP_PERIOD_MASK;
        channel1_.sweep_direction = (value >> SWEEP_DIRECTION_SHIFT) & SWEEP_DIRECTION_MASK;
        channel1_.sweep_shift = value & SWEEP_SHIFT_MASK;
        
        // Convert sweep period to cycle threshold
        channel1_.sweep_threshold = channel1_.sweep_period << SWEEP_SHIFT;
        
        // Initialize sweep frequency to current frequency
        channel1_.sweep_frequency = channel1_.frequency;
        return;
    }
    // Channel 1: Sound length and wave duty (NR11)
    if (address == REG_NR11) {
        channel1_.wave_duty = (value >> DUTY_CYCLE_SHIFT) & DUTY_CYCLE_MASK;
        
        // Convert length (0-63) to cycle count
        channel1_.length_threshold = (LENGTH_MAX_64 - (value & LENGTH_MASK)) << LENGTH_SHIFT;
        return;
    }
    // Channel 1: Volume envelope (NR12)
    if (address == REG_NR12) {
        // Set current and initial volume
        channel1_.envelope_volume = (value >> VOLUME_SHIFT) & VOLUME_MASK;
        channel1_.initial_volume = channel1_.envelope_volume;
        
        // Store envelope parameters for trigger
        channel1_.envelope_direction = (value >> ENVELOPE_DIRECTION_SHIFT) & SWEEP_DIRECTION_MASK;
        channel1_.envelope_period = value & ENVELOPE_MASK;
        
        // Envelope direction: convert 0/1 to -1/+1
        channel1_.envelope_direction_state = channel1_.envelope_direction;
        channel1_.envelope_direction_state |= channel1_.envelope_direction_state - 1;  // 0 becomes -1, 1 stays 1
        
        // Convert envelope period to cycle threshold
        channel1_.envelope_threshold = channel1_.envelope_period << ENVELOPE_SHIFT;
        channel1_.dac_enabled = (value & DAC_ENABLE_MASK) != 0;
        return;
    }
    // Channel 1: Frequency low byte (NR13)
    if (address == REG_NR13) {
        channel1_.frequency = (channel1_.frequency & (FREQUENCY_HIGH_MASK << FREQUENCY_HIGH_SHIFT)) | value;
        
        // Recalculate phase increment from 11-bit frequency value
        int divisor = FREQ_DIVISOR_BASE - channel1_.frequency;
        if (RATE > (divisor << FREQ_CHECK_SHIFT_SQUARE)) {
            channel1_.phase_increment = 0;
        } else {
            channel1_.phase_increment = (RATE << FREQ_SHIFT_SQUARE) / divisor;
        }
        return;
    }
    // Channel 1: Frequency high byte and trigger (NR14)
    if (address == REG_NR14) {
        channel1_.frequency = (channel1_.frequency & FREQUENCY_LOW_MASK) | ((value & FREQUENCY_HIGH_MASK) << FREQUENCY_HIGH_SHIFT);
        channel1_.length_enable = (value & LENGTH_ENABLE_BIT) != 0;
        
        // Recalculate phase increment
        int divisor = FREQ_DIVISOR_BASE - channel1_.frequency;
        if (RATE > (divisor << FREQ_CHECK_SHIFT_SQUARE)) {
            channel1_.phase_increment = 0;
        } else {
            channel1_.phase_increment = (RATE << FREQ_SHIFT_SQUARE) / divisor;
        }
        
        // Trigger: restart channel (bit 7)
        if (value & TRIGGER_BIT) {
            channel1_.sweep_counter = 0;
            channel1_.sweep_frequency = channel1_.frequency;
            channel1_.envelope_volume = channel1_.initial_volume;
            channel1_.envelope_direction_state = (channel1_.envelope_direction) ? 1 : -1;
            channel1_.envelope_threshold = channel1_.envelope_period << ENVELOPE_SHIFT;
            if (!channel1_.on) channel1_.phase_position = 0;
            channel1_.on = channel1_.dac_enabled;
            channel1_.length_counter = 0;
            channel1_.envelope_counter = 0;
        }
        return;
    }

    // Channel 2: Sound length and wave duty (NR21)
    if (address == REG_NR21) {
        channel2_.wave_duty = (value >> DUTY_CYCLE_SHIFT) & DUTY_CYCLE_MASK;
        
        // Convert length (0-63) to cycle count
        channel2_.length_threshold = (LENGTH_MAX_64 - (value & LENGTH_MASK)) << LENGTH_SHIFT;
        return;
    }
    // Channel 2: Volume envelope (NR22)
    if (address == REG_NR22) {
        // Set current and initial volume
        channel2_.envelope_volume = (value >> VOLUME_SHIFT) & VOLUME_MASK;
        channel2_.initial_volume = channel2_.envelope_volume;
        
        // Store envelope parameters for trigger
        channel2_.envelope_direction = (value >> ENVELOPE_DIRECTION_SHIFT) & SWEEP_DIRECTION_MASK;
        channel2_.envelope_period = value & ENVELOPE_MASK;
        
        // Envelope direction: convert 0/1 to -1/+1
        channel2_.envelope_direction_state = channel2_.envelope_direction;
        channel2_.envelope_direction_state |= channel2_.envelope_direction_state - 1;
        
        // Convert envelope period to cycle threshold
        channel2_.envelope_threshold = channel2_.envelope_period << ENVELOPE_SHIFT;
        channel2_.dac_enabled = (value & DAC_ENABLE_MASK) != 0;
        return;
    }
    // Channel 2: Frequency low byte (NR23)
    if (address == REG_NR23) {
        channel2_.frequency = (channel2_.frequency & (FREQUENCY_HIGH_MASK << FREQUENCY_HIGH_SHIFT)) | value;
        
        // Recalculate phase increment
        int divisor = FREQ_DIVISOR_BASE - channel2_.frequency;
        if (RATE > (divisor << FREQ_CHECK_SHIFT_SQUARE)) {
            channel2_.phase_increment = 0;
        } else {
            channel2_.phase_increment = (RATE << FREQ_SHIFT_SQUARE) / divisor;
        }
        return;
    }
    // Channel 2: Frequency high byte and trigger (NR24)
    if (address == REG_NR24) {
        channel2_.frequency = (channel2_.frequency & FREQUENCY_LOW_MASK) | ((value & FREQUENCY_HIGH_MASK) << FREQUENCY_HIGH_SHIFT);
        channel2_.length_enable = (value & LENGTH_ENABLE_BIT) != 0;
        
        // Recalculate phase increment
        int divisor = FREQ_DIVISOR_BASE - channel2_.frequency;
        if (RATE > (divisor << FREQ_CHECK_SHIFT_SQUARE)) {
            channel2_.phase_increment = 0;
        } else {
            channel2_.phase_increment = (RATE << FREQ_SHIFT_SQUARE) / divisor;
        }
        
        // Trigger: restart channel
        if (value & TRIGGER_BIT) {
            channel2_.envelope_volume = channel2_.initial_volume;
            channel2_.envelope_direction_state = (channel2_.envelope_direction) ? 1 : -1;
            channel2_.envelope_threshold = channel2_.envelope_period << ENVELOPE_SHIFT;
            if (!channel2_.on) channel2_.phase_position = 0;
            channel2_.on = channel2_.dac_enabled;
            channel2_.length_counter = 0;
            channel2_.envelope_counter = 0;
        }
        return;
    }

    // Channel 3: DAC power (NR30)
    if (address == REG_NR30) {
        channel3_.dac_enabled = (value & DAC_ENABLE_BIT) != 0;
        
        // Turning off DAC disables the channel
        if (!channel3_.dac_enabled) {
            channel3_.on = false;
        }
        return;
    }
    // Channel 3: Sound length (NR31)
    if (address == REG_NR31) {
        // Convert 8-bit length value to cycle count
        channel3_.length_threshold = (LENGTH_MAX_256 - value) << LENGTH_SHIFT;
        return;
    }
    // Channel 3: Output level/volume (NR32)
    if (address == REG_NR32) {
        // 00=mute, 01=100%, 10=50%, 11=25%
        channel3_.output_level = (value >> OUTPUT_LEVEL_SHIFT) & OUTPUT_LEVEL_MASK;
        return;
    }
    // Channel 3: Frequency low byte (NR33)
    if (address == REG_NR33) {
        channel3_.frequency = (channel3_.frequency & (FREQUENCY_HIGH_MASK << FREQUENCY_HIGH_SHIFT)) | value;
        
        // Recalculate phase increment (note: different shift than CH1/CH2)
        int divisor = FREQ_DIVISOR_BASE - channel3_.frequency;
        if (RATE > (divisor << FREQ_CHECK_SHIFT_WAVE)) {
            channel3_.phase_increment = 0;
        } else {
            channel3_.phase_increment = (RATE << FREQ_SHIFT_WAVE) / divisor;
        }
        return;
    }
    // Channel 3: Frequency high byte and trigger (NR34)
    if (address == REG_NR34) {
        channel3_.frequency = (channel3_.frequency & FREQUENCY_LOW_MASK) | ((value & FREQUENCY_HIGH_MASK) << FREQUENCY_HIGH_SHIFT);
        channel3_.length_enable = (value & LENGTH_ENABLE_BIT) != 0;
        
        // Recalculate phase increment
        int divisor = FREQ_DIVISOR_BASE - channel3_.frequency;
        if (RATE > (divisor << FREQ_CHECK_SHIFT_WAVE)) {
            channel3_.phase_increment = 0;
        } else {
            channel3_.phase_increment = (RATE << FREQ_SHIFT_WAVE) / divisor;
        }
        
        // Trigger: restart channel
        if (value & TRIGGER_BIT) {
            if (!channel3_.on) channel3_.phase_position = 0;
            channel3_.length_counter = 0;
            channel3_.on = channel3_.dac_enabled;
        }
        return;
    }

    // Channel 4: Sound length (NR41)
    if (address == REG_NR41) {
        // Convert length (0-63) to cycle count
        channel4_.length_threshold = (LENGTH_MAX_64 - (value & LENGTH_MASK)) << LENGTH_SHIFT;
        return;
    }
    // Channel 4: Volume envelope (NR42)
    if (address == REG_NR42) {
        // Set current and initial volume
        channel4_.envelope_volume = (value >> VOLUME_SHIFT) & VOLUME_MASK;
        channel4_.initial_volume = channel4_.envelope_volume;
        
        // Store envelope parameters for trigger
        channel4_.envelope_direction = (value >> ENVELOPE_DIRECTION_SHIFT) & SWEEP_DIRECTION_MASK;
        channel4_.envelope_period = value & ENVELOPE_MASK;
        
        // Envelope direction: convert 0/1 to -1/+1
        channel4_.envelope_direction_state = channel4_.envelope_direction;
        channel4_.envelope_direction_state |= channel4_.envelope_direction_state - 1;
        
        // Convert envelope period to cycle threshold
        channel4_.envelope_threshold = channel4_.envelope_period << ENVELOPE_SHIFT;
        channel4_.dac_enabled = (value & DAC_ENABLE_MASK) != 0;
        return;
    }
    // Channel 4: Polynomial counter/LFSR control (NR43)
    if (address == REG_NR43) {
        channel4_.clock_shift = (value >> CLOCK_SHIFT_SHIFT) & CLOCK_SHIFT_MASK;
        channel4_.width_mode = (value >> WIDTH_MODE_SHIFT) & WIDTH_MODE_MASK;
        channel4_.divisor_code = value & DIVISOR_CODE_MASK;
        channel4_.phase_increment = calculate_noise_frequency(channel4_.divisor_code, channel4_.clock_shift);
        return;
    }
    // Channel 4: Length enable and trigger (NR44)
    if (address == REG_NR44) {
        channel4_.length_enable = (value & LENGTH_ENABLE_BIT) != 0;
        
        // Trigger: restart channel
        if (value & TRIGGER_BIT) {
            channel4_.envelope_volume = channel4_.initial_volume;
            channel4_.envelope_direction_state = (channel4_.envelope_direction) ? 1 : -1;
            channel4_.envelope_threshold = channel4_.envelope_period << ENVELOPE_SHIFT;
            channel4_.on = channel4_.dac_enabled;
            channel4_.phase_position = 0;
            channel4_.length_counter = 0;
            channel4_.envelope_counter = 0;
            channel4_.lfsr = LFSR_INIT;
        }
        return;
    }

    // Master volume (NR50)
    if (address == REG_NR50) {
        nr50_ = value;  // Bits 0-2: right vol, bits 4-6: left vol
        return;
    }
    
    // Sound panning (NR51)
    if (address == REG_NR51) {
        nr51_ = value;  // Each bit enables a channel on left/right output
        return;
    }
    
    // Sound power control (NR52)
    if (address == REG_NR52) {
        bool power_on = (value & TRIGGER_BIT) != 0;
        if (!power_on) {
            // Turning off power resets all APU state
            reset();
        }
        nr52_ = value & TRIGGER_BIT;  // Only bit 7 is writable
        return;
    }
}

const std::vector<std::pair<u8, u8>>& APU::get_audio_buffer() const {
    return audio_buffer_;
}

void APU::clear_audio_buffer() {
    audio_buffer_.clear();
}

// ============================================================================
// Serialization for Save States
// ============================================================================

namespace {
    // Helper to serialize a 16-bit value
    void serialize_u16(u16 value, std::vector<u8>& data) {
        data.push_back(static_cast<u8>(value & 0xFF));
        data.push_back(static_cast<u8>((value >> 8) & 0xFF));
    }
    
    // Helper to serialize a 32-bit value (for int)
    void serialize_i32(int value, std::vector<u8>& data) {
        data.push_back(static_cast<u8>(value & 0xFF));
        data.push_back(static_cast<u8>((value >> 8) & 0xFF));
        data.push_back(static_cast<u8>((value >> 16) & 0xFF));
        data.push_back(static_cast<u8>((value >> 24) & 0xFF));
    }
    
    // Helper to deserialize a 16-bit value
    u16 deserialize_u16(const u8* data, size_t& offset) {
        u16 value = static_cast<u16>(data[offset]) | (static_cast<u16>(data[offset + 1]) << 8);
        offset += 2;
        return value;
    }
    
    // Helper to deserialize a 32-bit value (for int)
    int deserialize_i32(const u8* data, size_t& offset) {
        int value = static_cast<int>(data[offset]) |
                   (static_cast<int>(data[offset + 1]) << 8) |
                   (static_cast<int>(data[offset + 2]) << 16) |
                   (static_cast<int>(data[offset + 3]) << 24);
        offset += 4;
        return value;
    }
}

void APU::serialize(std::vector<u8>& data) const
{
    // Control registers
    data.push_back(nr50_);
    data.push_back(nr51_);
    data.push_back(nr52_);
    
    // Wave RAM
    data.insert(data.end(), wave_ram_.begin(), wave_ram_.end());
    
    // Cycle accumulator
    serialize_i32(static_cast<int>(cycle_accumulator_), data);
    
    // Channel 1 state
    data.push_back(channel1_.sweep_period);
    data.push_back(channel1_.sweep_direction);
    data.push_back(channel1_.sweep_shift);
    data.push_back(channel1_.wave_duty);
    data.push_back(channel1_.initial_volume);
    data.push_back(channel1_.envelope_direction);
    data.push_back(channel1_.envelope_period);
    serialize_u16(channel1_.frequency, data);
    data.push_back(channel1_.length_enable ? 1 : 0);
    data.push_back(channel1_.dac_enabled ? 1 : 0);
    data.push_back(channel1_.on ? 1 : 0);
    serialize_i32(channel1_.phase_position, data);
    serialize_i32(channel1_.phase_increment, data);
    serialize_i32(channel1_.length_counter, data);
    serialize_i32(channel1_.length_threshold, data);
    serialize_i32(channel1_.envelope_counter, data);
    serialize_i32(channel1_.envelope_threshold, data);
    serialize_i32(channel1_.envelope_direction_state, data);
    serialize_i32(channel1_.envelope_volume, data);
    serialize_i32(channel1_.sweep_counter, data);
    serialize_i32(channel1_.sweep_threshold, data);
    serialize_i32(channel1_.sweep_frequency, data);
    
    // Channel 2 state
    data.push_back(channel2_.wave_duty);
    data.push_back(channel2_.initial_volume);
    data.push_back(channel2_.envelope_direction);
    data.push_back(channel2_.envelope_period);
    serialize_u16(channel2_.frequency, data);
    data.push_back(channel2_.length_enable ? 1 : 0);
    data.push_back(channel2_.dac_enabled ? 1 : 0);
    data.push_back(channel2_.on ? 1 : 0);
    serialize_i32(channel2_.phase_position, data);
    serialize_i32(channel2_.phase_increment, data);
    serialize_i32(channel2_.length_counter, data);
    serialize_i32(channel2_.length_threshold, data);
    serialize_i32(channel2_.envelope_counter, data);
    serialize_i32(channel2_.envelope_threshold, data);
    serialize_i32(channel2_.envelope_direction_state, data);
    serialize_i32(channel2_.envelope_volume, data);
    
    // Channel 3 state
    data.push_back(channel3_.output_level);
    serialize_u16(channel3_.frequency, data);
    data.push_back(channel3_.length_enable ? 1 : 0);
    data.push_back(channel3_.dac_enabled ? 1 : 0);
    data.push_back(channel3_.on ? 1 : 0);
    serialize_i32(channel3_.phase_position, data);
    serialize_i32(channel3_.phase_increment, data);
    serialize_i32(channel3_.length_counter, data);
    serialize_i32(channel3_.length_threshold, data);
    
    // Channel 4 state
    data.push_back(channel4_.initial_volume);
    data.push_back(channel4_.envelope_direction);
    data.push_back(channel4_.envelope_period);
    data.push_back(channel4_.clock_shift);
    data.push_back(channel4_.width_mode);
    data.push_back(channel4_.divisor_code);
    data.push_back(channel4_.length_enable ? 1 : 0);
    data.push_back(channel4_.dac_enabled ? 1 : 0);
    data.push_back(channel4_.on ? 1 : 0);
    serialize_u16(channel4_.lfsr, data);
    serialize_i32(channel4_.phase_position, data);
    serialize_i32(channel4_.phase_increment, data);
    serialize_i32(channel4_.length_counter, data);
    serialize_i32(channel4_.length_threshold, data);
    serialize_i32(channel4_.envelope_counter, data);
    serialize_i32(channel4_.envelope_threshold, data);
    serialize_i32(channel4_.envelope_direction_state, data);
    serialize_i32(channel4_.envelope_volume, data);
}

void APU::deserialize(const u8* data, size_t data_size, size_t& offset)
{
    // Component sizes matching serialize() output
    constexpr size_t CTRL_BYTES = 3;       // nr50, nr51, nr52
    constexpr size_t WAVE_RAM_BYTES = 16;  // wave_ram_ array
    constexpr size_t CYCLE_ACC_BYTES = 4;  // cycle_accumulator_ (i32)
    constexpr size_t CH1_BYTES = 56;       // 7 u8 + 1 u16 + 3 bool + 11 i32
    constexpr size_t CH2_BYTES = 41;       // 4 u8 + 1 u16 + 3 bool + 8 i32
    constexpr size_t CH3_BYTES = 22;       // 1 u8 + 1 u16 + 3 bool + 4 i32
    constexpr size_t CH4_BYTES = 43;       // 6 u8 + 3 bool + 1 u16 + 8 i32
    constexpr size_t APU_STATE_SIZE = CTRL_BYTES + WAVE_RAM_BYTES + CYCLE_ACC_BYTES
        + CH1_BYTES + CH2_BYTES + CH3_BYTES + CH4_BYTES;
    if (offset + APU_STATE_SIZE > data_size) {
        std::cerr << "APU deserialize: expected " << APU_STATE_SIZE
                  << " bytes at offset " << offset
                  << ", but only " << (data_size - offset) << " remain" << std::endl;
        return;
    }

    // Control registers
    nr50_ = data[offset++];
    nr51_ = data[offset++];
    nr52_ = data[offset++];
    
    // Wave RAM
    std::copy(data + offset, data + offset + wave_ram_.size(), wave_ram_.begin());
    offset += wave_ram_.size();
    
    // Cycle accumulator
    const int restored_cycle_accumulator = deserialize_i32(data, offset);
    cycle_accumulator_ = static_cast<Cycles>(std::clamp(restored_cycle_accumulator, 0, CYCLES_PER_SAMPLE - 1));
    
    // Channel 1 state
    channel1_.sweep_period = data[offset++];
    channel1_.sweep_direction = data[offset++];
    channel1_.sweep_shift = data[offset++];
    channel1_.wave_duty = data[offset++];
    channel1_.initial_volume = data[offset++];
    channel1_.envelope_direction = data[offset++];
    channel1_.envelope_period = data[offset++];
    channel1_.frequency = deserialize_u16(data, offset);
    channel1_.length_enable = data[offset++] != 0;
    channel1_.dac_enabled = data[offset++] != 0;
    channel1_.on = data[offset++] != 0;
    channel1_.phase_position = deserialize_i32(data, offset);
    channel1_.phase_increment = deserialize_i32(data, offset);
    channel1_.length_counter = deserialize_i32(data, offset);
    channel1_.length_threshold = deserialize_i32(data, offset);
    channel1_.envelope_counter = deserialize_i32(data, offset);
    channel1_.envelope_threshold = deserialize_i32(data, offset);
    channel1_.envelope_direction_state = deserialize_i32(data, offset);
    channel1_.envelope_volume = deserialize_i32(data, offset);
    channel1_.sweep_counter = deserialize_i32(data, offset);
    channel1_.sweep_threshold = deserialize_i32(data, offset);
    channel1_.sweep_frequency = deserialize_i32(data, offset);
    
    // Channel 2 state
    channel2_.wave_duty = data[offset++];
    channel2_.initial_volume = data[offset++];
    channel2_.envelope_direction = data[offset++];
    channel2_.envelope_period = data[offset++];
    channel2_.frequency = deserialize_u16(data, offset);
    channel2_.length_enable = data[offset++] != 0;
    channel2_.dac_enabled = data[offset++] != 0;
    channel2_.on = data[offset++] != 0;
    channel2_.phase_position = deserialize_i32(data, offset);
    channel2_.phase_increment = deserialize_i32(data, offset);
    channel2_.length_counter = deserialize_i32(data, offset);
    channel2_.length_threshold = deserialize_i32(data, offset);
    channel2_.envelope_counter = deserialize_i32(data, offset);
    channel2_.envelope_threshold = deserialize_i32(data, offset);
    channel2_.envelope_direction_state = deserialize_i32(data, offset);
    channel2_.envelope_volume = deserialize_i32(data, offset);
    
    // Channel 3 state
    channel3_.output_level = data[offset++];
    channel3_.frequency = deserialize_u16(data, offset);
    channel3_.length_enable = data[offset++] != 0;
    channel3_.dac_enabled = data[offset++] != 0;
    channel3_.on = data[offset++] != 0;
    channel3_.phase_position = deserialize_i32(data, offset);
    channel3_.phase_increment = deserialize_i32(data, offset);
    channel3_.length_counter = deserialize_i32(data, offset);
    channel3_.length_threshold = deserialize_i32(data, offset);
    
    // Channel 4 state
    channel4_.initial_volume = data[offset++];
    channel4_.envelope_direction = data[offset++];
    channel4_.envelope_period = data[offset++];
    channel4_.clock_shift = data[offset++];
    channel4_.width_mode = data[offset++];
    channel4_.divisor_code = data[offset++];
    channel4_.length_enable = data[offset++] != 0;
    channel4_.dac_enabled = data[offset++] != 0;
    channel4_.on = data[offset++] != 0;
    channel4_.lfsr = deserialize_u16(data, offset);
    channel4_.phase_position = deserialize_i32(data, offset);
    channel4_.phase_increment = deserialize_i32(data, offset);
    channel4_.length_counter = deserialize_i32(data, offset);
    channel4_.length_threshold = deserialize_i32(data, offset);
    channel4_.envelope_counter = deserialize_i32(data, offset);
    channel4_.envelope_threshold = deserialize_i32(data, offset);
    channel4_.envelope_direction_state = deserialize_i32(data, offset);
    channel4_.envelope_volume = deserialize_i32(data, offset);

    nr52_ &= NR52_POWER_BIT;

    channel1_.sweep_period &= SWEEP_PERIOD_MASK;
    channel1_.sweep_direction &= SWEEP_DIRECTION_MASK;
    channel1_.sweep_shift &= SWEEP_SHIFT_MASK;
    channel1_.wave_duty &= DUTY_CYCLE_MASK;
    channel1_.initial_volume &= VOLUME_MASK;
    channel1_.envelope_direction &= SWEEP_DIRECTION_MASK;
    channel1_.envelope_period &= ENVELOPE_MASK;
    channel1_.frequency = std::min<u16>(channel1_.frequency, SWEEP_FREQUENCY_MAX);
    channel1_.phase_position = std::max(channel1_.phase_position, 0);
    channel1_.length_counter = std::max(channel1_.length_counter, 0);
    channel1_.length_threshold = std::max(channel1_.length_threshold, 0);
    channel1_.envelope_counter = std::max(channel1_.envelope_counter, 0);
    channel1_.envelope_threshold = channel1_.envelope_period << ENVELOPE_SHIFT;
    channel1_.envelope_direction_state = channel1_.envelope_direction ? 1 : -1;
    channel1_.envelope_volume = std::clamp(channel1_.envelope_volume, ENVELOPE_MIN, ENVELOPE_MAX);
    channel1_.sweep_counter = std::max(channel1_.sweep_counter, 0);
    channel1_.sweep_threshold = channel1_.sweep_period << SWEEP_SHIFT;
    channel1_.sweep_frequency = std::clamp(channel1_.sweep_frequency, 0, SWEEP_FREQUENCY_MAX);
    {
        const int divisor = FREQ_DIVISOR_BASE - channel1_.frequency;
        channel1_.phase_increment = (RATE > (divisor << FREQ_CHECK_SHIFT_SQUARE))
            ? 0
            : (RATE << FREQ_SHIFT_SQUARE) / divisor;
    }

    channel2_.wave_duty &= DUTY_CYCLE_MASK;
    channel2_.initial_volume &= VOLUME_MASK;
    channel2_.envelope_direction &= SWEEP_DIRECTION_MASK;
    channel2_.envelope_period &= ENVELOPE_MASK;
    channel2_.frequency = std::min<u16>(channel2_.frequency, SWEEP_FREQUENCY_MAX);
    channel2_.phase_position = std::max(channel2_.phase_position, 0);
    channel2_.length_counter = std::max(channel2_.length_counter, 0);
    channel2_.length_threshold = std::max(channel2_.length_threshold, 0);
    channel2_.envelope_counter = std::max(channel2_.envelope_counter, 0);
    channel2_.envelope_threshold = channel2_.envelope_period << ENVELOPE_SHIFT;
    channel2_.envelope_direction_state = channel2_.envelope_direction ? 1 : -1;
    channel2_.envelope_volume = std::clamp(channel2_.envelope_volume, ENVELOPE_MIN, ENVELOPE_MAX);
    {
        const int divisor = FREQ_DIVISOR_BASE - channel2_.frequency;
        channel2_.phase_increment = (RATE > (divisor << FREQ_CHECK_SHIFT_SQUARE))
            ? 0
            : (RATE << FREQ_SHIFT_SQUARE) / divisor;
    }

    channel3_.output_level &= OUTPUT_LEVEL_MASK;
    channel3_.frequency = std::min<u16>(channel3_.frequency, SWEEP_FREQUENCY_MAX);
    channel3_.phase_position = std::max(channel3_.phase_position, 0);
    channel3_.length_counter = std::max(channel3_.length_counter, 0);
    channel3_.length_threshold = std::max(channel3_.length_threshold, 0);
    {
        const int divisor = FREQ_DIVISOR_BASE - channel3_.frequency;
        channel3_.phase_increment = (RATE > (divisor << FREQ_CHECK_SHIFT_WAVE))
            ? 0
            : (RATE << FREQ_SHIFT_WAVE) / divisor;
    }

    channel4_.initial_volume &= VOLUME_MASK;
    channel4_.envelope_direction &= SWEEP_DIRECTION_MASK;
    channel4_.envelope_period &= ENVELOPE_MASK;
    channel4_.clock_shift &= CLOCK_SHIFT_MASK;
    channel4_.width_mode &= WIDTH_MODE_MASK;
    channel4_.divisor_code &= DIVISOR_CODE_MASK;
    channel4_.lfsr &= LFSR_INIT;
    channel4_.phase_position = std::max(channel4_.phase_position, 0);
    channel4_.length_counter = std::max(channel4_.length_counter, 0);
    channel4_.length_threshold = std::max(channel4_.length_threshold, 0);
    channel4_.envelope_counter = std::max(channel4_.envelope_counter, 0);
    channel4_.envelope_threshold = channel4_.envelope_period << ENVELOPE_SHIFT;
    channel4_.envelope_direction_state = channel4_.envelope_direction ? 1 : -1;
    channel4_.envelope_volume = std::clamp(channel4_.envelope_volume, ENVELOPE_MIN, ENVELOPE_MAX);
    channel4_.phase_increment = calculate_noise_frequency(channel4_.divisor_code, channel4_.clock_shift);

    const bool power_on = (nr52_ & NR52_POWER_BIT) != 0;
    channel1_.on = power_on && channel1_.dac_enabled && channel1_.on;
    channel2_.on = power_on && channel2_.dac_enabled && channel2_.on;
    channel3_.on = power_on && channel3_.dac_enabled && channel3_.on;
    channel4_.on = power_on && channel4_.dac_enabled && channel4_.on;
    
    // Clear audio buffer on load
    audio_buffer_.clear();
}

} // namespace gbglow
