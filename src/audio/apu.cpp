#include "apu.h"
#include "../core/memory.h"
#include <cmath>
#include <algorithm>
#include <cstdio>

namespace emugbc {

APU::APU(Memory& memory)
    : memory_(memory)
    , nr50_(0)
    , nr51_(0)
    , nr52_(0)
    , cycle_accumulator_(0)
{
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
    nr52_ = NR52_RESET_VALUE;  // Power off, but bits 4-7 are always 1
    
    cycle_accumulator_ = 0;
    frame_sequencer_cycles_ = 0;
    frame_sequencer_step_ = 0;
}

void APU::step(Cycles cycles) {
    // All audio processing stops when APU power is off (NR52 bit 7)
    if ((nr52_ & NR52_POWER_BIT) == 0) {
        return;
    }
    
    cycle_accumulator_ += cycles;
    
    // gnuboy's approach: length counters, envelopes, and sweep are applied
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
// These patterns match gnuboy's implementation for audio compatibility.
static const int sqwave[4][8] = {
    {  0, 0,-1, 0, 0, 0, 0, 0 },  // 12.5% duty
    {  0,-1,-1, 0, 0, 0, 0, 0 },  // 25% duty
    { -1,-1,-1,-1, 0, 0, 0, 0 },  // 50% duty
    { -1, 0, 0,-1,-1,-1,-1,-1 }   // 75% duty
};

std::pair<u8, u8> APU::generate_sample() {
    // This function exactly replicates gnuboy's sound_mix() algorithm.
    // Each channel is processed independently, then mixed with panning.
    // Sample format: signed values (-128 to 127) converted to unsigned (0 to 255).
    int sample = 0;
    int left = 0;
    int right = 0;
    
    // Channel 1: Square wave with frequency sweep
    if (channel1_.on) {
        // Get square wave sample based on current phase position and duty cycle
        sample = sqwave[channel1_.wave_duty][(channel1_.pos >> PHASE_SHIFT_SQUARE) & PHASE_MASK_SQUARE] & channel1_.envol;
        channel1_.pos += channel1_.freq;
        
        // Length counter: disable channel when length expires
        if (channel1_.length_enable && ((channel1_.cnt += RATE) >= channel1_.len)) {
            channel1_.on = false;
        }
        
        // Volume envelope: adjust volume periodically
        if (channel1_.enlen && (channel1_.encnt += RATE) >= channel1_.enlen) {
            channel1_.encnt -= channel1_.enlen;
            channel1_.envol += channel1_.endir;
            if (channel1_.envol < ENVELOPE_MIN) channel1_.envol = ENVELOPE_MIN;
            if (channel1_.envol > ENVELOPE_MAX) channel1_.envol = ENVELOPE_MAX;
        }
        
        // Frequency sweep: periodically shift frequency up or down
        if (channel1_.swlen && (channel1_.swcnt += RATE) >= channel1_.swlen) {
            channel1_.swcnt -= channel1_.swlen;
            int frequency = channel1_.swfreq;
            int shift = channel1_.sweep_shift;
            
            // Apply sweep: subtract if direction=1, add if direction=0
            if (channel1_.sweep_direction) {
                frequency -= (frequency >> shift);
            } else {
                frequency += (frequency >> shift);
            }
            
            // Disable channel if frequency overflows
            if (frequency > SWEEP_FREQUENCY_MAX) {
                channel1_.on = false;
            } else {
                channel1_.swfreq = frequency;
                // Recalculate phase increment from new frequency
                int divisor = FREQ_DIVISOR_BASE - frequency;
                if (RATE > (divisor << FREQ_CHECK_SHIFT_SQUARE)) {
                    channel1_.freq = 0;
                } else {
                    channel1_.freq = (RATE << FREQ_SHIFT_SQUARE) / divisor;
                }
            }
        }
        
        // Amplify and apply panning (NR51 controls left/right output)
        sample <<= SAMPLE_AMPLIFY_SHIFT;
        if (nr51_ & NR51_CH1_RIGHT) right += sample;
        if (nr51_ & NR51_CH1_LEFT) left += sample;
    }
    
    // Channel 2: Square wave (no sweep)
    if (channel2_.on) {
        // Get square wave sample based on current phase position and duty cycle
        sample = sqwave[channel2_.wave_duty][(channel2_.pos >> PHASE_SHIFT_SQUARE) & PHASE_MASK_SQUARE] & channel2_.envol;
        channel2_.pos += channel2_.freq;
        
        // Length counter: disable channel when length expires
        if (channel2_.length_enable && ((channel2_.cnt += RATE) >= channel2_.len)) {
            channel2_.on = false;
        }
        
        // Volume envelope: adjust volume periodically
        if (channel2_.enlen && (channel2_.encnt += RATE) >= channel2_.enlen) {
            channel2_.encnt -= channel2_.enlen;
            channel2_.envol += channel2_.endir;
            if (channel2_.envol < ENVELOPE_MIN) channel2_.envol = ENVELOPE_MIN;
            if (channel2_.envol > ENVELOPE_MAX) channel2_.envol = ENVELOPE_MAX;
        }
        
        // Amplify and apply panning
        sample <<= SAMPLE_AMPLIFY_SHIFT;
        if (nr51_ & NR51_CH2_RIGHT) right += sample;
        if (nr51_ & NR51_CH2_LEFT) left += sample;
    }
    
    // Channel 3: Programmable wave pattern from wave RAM
    if (channel3_.on) {
        // Wave RAM stores 32 4-bit samples (16 bytes, 2 samples per byte)
        sample = wave_ram_[(channel3_.pos >> PHASE_SHIFT_WAVE) & PHASE_MASK_WAVE];
        
        // Extract high or low nibble based on phase position
        if (channel3_.pos & (1 << WAVE_NIBBLE_BIT)) {
            sample &= WAVE_NIBBLE_MASK;  // Low nibble
        } else {
            sample >>= WAVE_NIBBLE_SHIFT;  // High nibble
        }
        sample -= SAMPLE_WAVE_OFFSET;  // Convert to signed (-8 to 7)
        
        channel3_.pos += channel3_.freq;
        
        // Length counter: disable channel when length expires
        if (channel3_.length_enable && ((channel3_.cnt += RATE) >= channel3_.len)) {
            channel3_.on = false;
        }
        
        // Apply output level (volume shift from NR32 bits 5-6)
        if (channel3_.output_level > 0) {
            sample <<= (WAVE_VOLUME_BASE - channel3_.output_level);
        } else {
            sample = 0;  // Muted
        }
        
        // Apply panning
        if (nr51_ & NR51_CH3_RIGHT) right += sample;
        if (nr51_ & NR51_CH3_LEFT) left += sample;
    }
    
    // Channel 4: Pseudo-random noise using Linear Feedback Shift Register (LFSR)
    if (channel4_.on) {
        // Advance LFSR based on accumulated phase
        channel4_.pos += channel4_.freq;
        while (channel4_.pos >= (1 << LFSR_PHASE_THRESHOLD)) {
            channel4_.pos -= (1 << LFSR_PHASE_THRESHOLD);
            
            // Calculate XOR of bottom two bits
            int xor_result = (channel4_.lfsr & LFSR_BIT_MASK) ^ ((channel4_.lfsr >> LFSR_XOR_SHIFT) & LFSR_BIT_MASK);
            
            // Shift register right and insert XOR result at top
            channel4_.lfsr >>= LFSR_XOR_SHIFT;
            channel4_.lfsr |= (xor_result << LFSR_FEEDBACK_BIT);
            
            // Width mode: 7-bit LFSR instead of 15-bit
            if (channel4_.width_mode) {
                channel4_.lfsr &= ~(1 << LFSR_WIDTH_BIT);
                channel4_.lfsr |= (xor_result << LFSR_WIDTH_BIT);
            }
        }
        
        // Generate sample from LFSR: bit 0 determines output
        sample = (channel4_.lfsr & LFSR_BIT_MASK) ? 0 : -1;
        sample = sample & channel4_.envol;
        channel4_.pos += channel4_.freq;
        
        // Length counter: disable channel when length expires
        if (channel4_.length_enable && ((channel4_.cnt += RATE) >= channel4_.len)) {
            channel4_.on = false;
        }
        
        // Volume envelope: adjust volume periodically
        if (channel4_.enlen && (channel4_.encnt += RATE) >= channel4_.enlen) {
            channel4_.encnt -= channel4_.enlen;
            channel4_.envol += channel4_.endir;
            if (channel4_.envol < ENVELOPE_MIN) channel4_.envol = ENVELOPE_MIN;
            if (channel4_.envol > ENVELOPE_MAX) channel4_.envol = ENVELOPE_MAX;
        }
        
        // Amplify noise (multiply by 3) and apply panning
        sample += sample << 1;
        if (nr51_ & NR51_CH4_RIGHT) right += sample;
        if (nr51_ & NR51_CH4_LEFT) left += sample;
    }
    
    // Apply master volume from NR50 (bits 0-2 for right, bits 4-6 for left)
    left *= (nr50_ & NR50_RIGHT_VOLUME_MASK);
    right *= ((nr50_ >> NR50_LEFT_VOLUME_SHIFT) & NR50_RIGHT_VOLUME_MASK);
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
            int current_byte_index = (channel3_.pos >> PHASE_SHIFT_WAVE) & PHASE_MASK_WAVE;
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
        
        // Convert sweep period to cycle threshold (gnuboy algorithm)
        channel1_.swlen = channel1_.sweep_period << SWEEP_SHIFT;
        
        // Initialize sweep frequency to current frequency
        channel1_.swfreq = channel1_.frequency;
        return;
    }
    // Channel 1: Sound length and wave duty (NR11)
    if (address == REG_NR11) {
        channel1_.wave_duty = (value >> DUTY_CYCLE_SHIFT) & DUTY_CYCLE_MASK;
        
        // Convert length (0-63) to cycle count using gnuboy formula
        channel1_.len = (LENGTH_MAX_64 - (value & LENGTH_MASK)) << LENGTH_SHIFT;
        return;
    }
    // Channel 1: Volume envelope (NR12)
    if (address == REG_NR12) {
        // Set current and initial volume
        channel1_.envol = (value >> VOLUME_SHIFT) & VOLUME_MASK;
        channel1_.initial_volume = channel1_.envol;
        
        // Envelope direction: convert 0/1 to -1/+1 using gnuboy's trick
        channel1_.endir = (value >> ENVELOPE_DIRECTION_SHIFT) & SWEEP_DIRECTION_MASK;
        channel1_.endir |= channel1_.endir - 1;  // 0 becomes -1, 1 stays 1
        
        // Convert envelope period to cycle threshold
        channel1_.enlen = (value & ENVELOPE_MASK) << ENVELOPE_SHIFT;
        channel1_.dac_enabled = (value & DAC_ENABLE_MASK) != 0;
        return;
    }
    // Channel 1: Frequency low byte (NR13)
    if (address == REG_NR13) {
        channel1_.frequency = (channel1_.frequency & (FREQUENCY_HIGH_MASK << FREQUENCY_HIGH_SHIFT)) | value;
        
        // Recalculate phase increment from 11-bit frequency value
        int divisor = FREQ_DIVISOR_BASE - channel1_.frequency;
        if (RATE > (divisor << FREQ_CHECK_SHIFT_SQUARE)) {
            channel1_.freq = 0;
        } else {
            channel1_.freq = (RATE << FREQ_SHIFT_SQUARE) / divisor;
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
            channel1_.freq = 0;
        } else {
            channel1_.freq = (RATE << FREQ_SHIFT_SQUARE) / divisor;
        }
        
        // Trigger: restart channel (bit 7)
        if (value & TRIGGER_BIT) {
            channel1_.swcnt = 0;
            channel1_.swfreq = channel1_.frequency;
            channel1_.envol = channel1_.initial_volume;
            channel1_.endir = (channel1_.envelope_direction) ? 1 : -1;
            channel1_.enlen = channel1_.envelope_period << ENVELOPE_SHIFT;
            if (!channel1_.on) channel1_.pos = 0;
            channel1_.on = channel1_.dac_enabled;
            channel1_.cnt = 0;
            channel1_.encnt = 0;
        }
        return;
    }

    // Channel 2: Sound length and wave duty (NR21)
    if (address == REG_NR21) {
        channel2_.wave_duty = (value >> DUTY_CYCLE_SHIFT) & DUTY_CYCLE_MASK;
        
        // Convert length (0-63) to cycle count
        channel2_.len = (LENGTH_MAX_64 - (value & LENGTH_MASK)) << LENGTH_SHIFT;
        return;
    }
    // Channel 2: Volume envelope (NR22)
    if (address == REG_NR22) {
        // Set current and initial volume
        channel2_.envol = (value >> VOLUME_SHIFT) & VOLUME_MASK;
        channel2_.initial_volume = channel2_.envol;
        
        // Envelope direction: convert 0/1 to -1/+1
        channel2_.endir = (value >> ENVELOPE_DIRECTION_SHIFT) & SWEEP_DIRECTION_MASK;
        channel2_.endir |= channel2_.endir - 1;
        
        // Convert envelope period to cycle threshold
        channel2_.enlen = (value & ENVELOPE_MASK) << ENVELOPE_SHIFT;
        channel2_.dac_enabled = (value & DAC_ENABLE_MASK) != 0;
        return;
    }
    // Channel 2: Frequency low byte (NR23)
    if (address == REG_NR23) {
        channel2_.frequency = (channel2_.frequency & (FREQUENCY_HIGH_MASK << FREQUENCY_HIGH_SHIFT)) | value;
        
        // Recalculate phase increment
        int divisor = FREQ_DIVISOR_BASE - channel2_.frequency;
        if (RATE > (divisor << FREQ_CHECK_SHIFT_SQUARE)) {
            channel2_.freq = 0;
        } else {
            channel2_.freq = (RATE << FREQ_SHIFT_SQUARE) / divisor;
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
            channel2_.freq = 0;
        } else {
            channel2_.freq = (RATE << FREQ_SHIFT_SQUARE) / divisor;
        }
        
        // Trigger: restart channel
        if (value & TRIGGER_BIT) {
            channel2_.envol = channel2_.initial_volume;
            channel2_.endir = (channel2_.envelope_direction) ? 1 : -1;
            channel2_.enlen = channel2_.envelope_period << ENVELOPE_SHIFT;
            if (!channel2_.on) channel2_.pos = 0;
            channel2_.on = channel2_.dac_enabled;
            channel2_.cnt = 0;
            channel2_.encnt = 0;
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
        channel3_.len = (LENGTH_MAX_256 - value) << LENGTH_SHIFT;
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
            channel3_.freq = 0;
        } else {
            channel3_.freq = (RATE << FREQ_SHIFT_WAVE) / divisor;
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
            channel3_.freq = 0;
        } else {
            channel3_.freq = (RATE << FREQ_SHIFT_WAVE) / divisor;
        }
        
        // Trigger: restart channel
        if (value & TRIGGER_BIT) {
            if (!channel3_.on) channel3_.pos = 0;
            channel3_.cnt = 0;
            channel3_.on = channel3_.dac_enabled;
        }
        return;
    }

    // Channel 4: Sound length (NR41)
    if (address == REG_NR41) {
        // Convert length (0-63) to cycle count
        channel4_.len = (LENGTH_MAX_64 - (value & LENGTH_MASK)) << LENGTH_SHIFT;
        return;
    }
    // Channel 4: Volume envelope (NR42)
    if (address == REG_NR42) {
        // Set current and initial volume
        channel4_.envol = (value >> VOLUME_SHIFT) & VOLUME_MASK;
        channel4_.initial_volume = channel4_.envol;
        
        // Envelope direction: convert 0/1 to -1/+1
        channel4_.endir = (value >> ENVELOPE_DIRECTION_SHIFT) & SWEEP_DIRECTION_MASK;
        channel4_.endir |= channel4_.endir - 1;
        
        // Convert envelope period to cycle threshold
        channel4_.enlen = (value & ENVELOPE_MASK) << ENVELOPE_SHIFT;
        channel4_.dac_enabled = (value & DAC_ENABLE_MASK) != 0;
        return;
    }
    // Channel 4: Polynomial counter/LFSR control (NR43)
    if (address == REG_NR43) {
        channel4_.clock_shift = (value >> CLOCK_SHIFT_SHIFT) & CLOCK_SHIFT_MASK;
        channel4_.width_mode = (value >> WIDTH_MODE_SHIFT) & WIDTH_MODE_MASK;  // 0=15-bit, 1=7-bit
        channel4_.divisor_code = value & DIVISOR_CODE_MASK;
        
        // Calculate LFSR clock frequency from divisor and shift
        static const int freqtab[8] = {
            (1<<14)*2, (1<<14), (1<<14)/2, (1<<14)/3,
            (1<<14)/4, (1<<14)/5, (1<<14)/6, (1<<14)/7
        };
        channel4_.freq = (freqtab[channel4_.divisor_code] >> channel4_.clock_shift) * RATE;
        if (channel4_.freq >> LFSR_PHASE_THRESHOLD) channel4_.freq = 1 << LFSR_PHASE_THRESHOLD;
        return;
    }
    // Channel 4: Length enable and trigger (NR44)
    if (address == REG_NR44) {
        channel4_.length_enable = (value & LENGTH_ENABLE_BIT) != 0;
        
        // Trigger: restart channel
        if (value & TRIGGER_BIT) {
            channel4_.envol = channel4_.initial_volume;
            channel4_.endir = (channel4_.envelope_direction) ? 1 : -1;
            channel4_.enlen = channel4_.envelope_period << ENVELOPE_SHIFT;
            channel4_.on = channel4_.dac_enabled;
            channel4_.pos = 0;
            channel4_.cnt = 0;
            channel4_.encnt = 0;
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

} // namespace emugbc
