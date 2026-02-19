#pragma once

#include "../core/types.h"
#include <array>
#include <vector>

namespace emugbc {

// Forward declaration
class Memory;

/**
 * Audio Processing Unit (APU)
 * 
 * Game Boy sound system with 4 channels:
 * - Channel 1: Square wave with frequency sweep
 * - Channel 2: Square wave
 * - Channel 3: Programmable wave pattern
 * - Channel 4: White noise
 * 
 * Sound Registers:
 * 0xFF10-0xFF14: Channel 1 (Square with Sweep)
 * 0xFF16-0xFF19: Channel 2 (Square)
 * 0xFF1A-0xFF1E: Channel 3 (Wave)
 * 0xFF20-0xFF23: Channel 4 (Noise)
 * 0xFF24-0xFF25: Sound Control
 * 0xFF26: Sound On/Off
 * 0xFF30-0xFF3F: Wave Pattern RAM
 */
class APU {
public:
    explicit APU(Memory& memory);
    
    /**
     * Step the APU forward by the given number of cycles
     * @param cycles Number of CPU cycles to advance
     */
    void step(Cycles cycles);
    
    /**
     * Read from sound register
     * @param address Register address (0xFF10-0xFF3F)
     * @return Register value
     */
    u8 read_register(u16 address) const;
    
    /**
     * Write to sound register
     * @param address Register address (0xFF10-0xFF3F)
     * @param value Value to write
     */
    void write_register(u16 address, u8 value);
    
    /**
     * Get audio samples for current frame
     * Returns stereo samples (left, right) as 8-bit unsigned values (0-255)
     * Sample rate is 44100 Hz
     * @return Vector of stereo samples
     */
    const std::vector<std::pair<u8, u8>>& get_audio_buffer() const;
    
    /**
     * Clear audio buffer after it's been consumed
     */
    void clear_audio_buffer();
    
private:
    Memory& memory_;
    
    // Sound control registers
    u8 nr50_;  // 0xFF24: Channel control / ON-OFF / Volume (Vin)
    u8 nr51_;  // 0xFF25: Sound output terminal selection
    u8 nr52_;  // 0xFF26: Sound on/off
    
    // Wave pattern RAM (32 4-bit samples)
    std::array<u8, 16> wave_ram_;  // 0xFF30-0xFF3F
    
    // Audio output buffer (gnuboy: U8 format, 0-255 range, 128 = silence)
    std::vector<std::pair<u8, u8>> audio_buffer_;
    
    // Cycle accumulators
    Cycles cycle_accumulator_;      // For sample generation
    Cycles frame_sequencer_cycles_; // For length/envelope/sweep timing
    u8 frame_sequencer_step_;       // 0-7 for frame sequencer
    
    // Frame rate constants
    static constexpr int FRAMES_PER_SECOND = 60;
    
    // Sample rate constants - COMPATIBLE CON GNUBOY
    static constexpr int SAMPLE_RATE = 44100;
    static constexpr int CPU_CLOCK_HZ = 4194304;
    static constexpr int RATE = (1 << 21) / SAMPLE_RATE;  // ~47.5 (gnuboy scaling factor for freq calculations)
    static constexpr int CYCLES_PER_SAMPLE = CPU_CLOCK_HZ / SAMPLE_RATE;  // ~95 cycles per sample
    static constexpr int FRAME_SEQUENCER_RATE = 8192;  // 512 Hz
    static constexpr int CYCLES_PER_FRAME = CPU_CLOCK_HZ / FRAME_SEQUENCER_RATE;  // ~512 cycles
    
    // Register addresses
    static constexpr u16 REG_NR10 = 0xFF10;  // Channel 1 Sweep
    static constexpr u16 REG_NR11 = 0xFF11;  // Channel 1 Sound length/Wave pattern duty
    static constexpr u16 REG_NR12 = 0xFF12;  // Channel 1 Volume Envelope
    static constexpr u16 REG_NR13 = 0xFF13;  // Channel 1 Frequency lo
    static constexpr u16 REG_NR14 = 0xFF14;  // Channel 1 Frequency hi
    
    static constexpr u16 REG_NR21 = 0xFF16;  // Channel 2 Sound Length/Wave Pattern Duty
    static constexpr u16 REG_NR22 = 0xFF17;  // Channel 2 Volume Envelope
    static constexpr u16 REG_NR23 = 0xFF18;  // Channel 2 Frequency lo
    static constexpr u16 REG_NR24 = 0xFF19;  // Channel 2 Frequency hi
    
    static constexpr u16 REG_NR30 = 0xFF1A;  // Channel 3 Sound on/off
    static constexpr u16 REG_NR31 = 0xFF1B;  // Channel 3 Sound Length
    static constexpr u16 REG_NR32 = 0xFF1C;  // Channel 3 Select output level
    static constexpr u16 REG_NR33 = 0xFF1D;  // Channel 3 Frequency lo
    static constexpr u16 REG_NR34 = 0xFF1E;  // Channel 3 Frequency hi
    
    static constexpr u16 REG_NR41 = 0xFF20;  // Channel 4 Sound Length
    static constexpr u16 REG_NR42 = 0xFF21;  // Channel 4 Volume Envelope
    static constexpr u16 REG_NR43 = 0xFF22;  // Channel 4 Polynomial Counter
    static constexpr u16 REG_NR44 = 0xFF23;  // Channel 4 Counter/consecutive; Initial
    
    static constexpr u16 REG_NR50 = 0xFF24;  // Channel control / ON-OFF / Volume
    static constexpr u16 REG_NR51 = 0xFF25;  // Selection of Sound output terminal
    static constexpr u16 REG_NR52 = 0xFF26;  // Sound on/off
    
    static constexpr u16 WAVE_RAM_START = 0xFF30;
    static constexpr u16 WAVE_RAM_END = 0xFF3F;
    
    // NR52 bit masks
    static constexpr u8 NR52_POWER_BIT = 0x80;
    static constexpr u8 NR52_CH1_ON = 0x01;
    static constexpr u8 NR52_CH2_ON = 0x02;
    static constexpr u8 NR52_CH3_ON = 0x04;
    static constexpr u8 NR52_CH4_ON = 0x08;
    
    // NR51 panning bit masks
    static constexpr u8 NR51_CH1_RIGHT = 0x01;
    static constexpr u8 NR51_CH2_RIGHT = 0x02;
    static constexpr u8 NR51_CH3_RIGHT = 0x04;
    static constexpr u8 NR51_CH4_RIGHT = 0x08;
    static constexpr u8 NR51_CH1_LEFT = 0x10;
    static constexpr u8 NR51_CH2_LEFT = 0x20;
    static constexpr u8 NR51_CH3_LEFT = 0x40;
    static constexpr u8 NR51_CH4_LEFT = 0x80;
    
    // NR50 volume masks
    static constexpr u8 NR50_RIGHT_VOLUME_MASK = 0x07;
    static constexpr u8 NR50_LEFT_VOLUME_MASK = 0x70;
    static constexpr u8 NR50_LEFT_VOLUME_SHIFT = 4;
    
    // Bit shift constants for sample generation
    static constexpr int PHASE_SHIFT_SQUARE = 18;  // For square wave position (pos >> 18)
    static constexpr int PHASE_SHIFT_WAVE = 22;     // For wave RAM position (pos >> 22)
    static constexpr int PHASE_MASK_SQUARE = 7;     // Mask for 8-step duty cycle
    static constexpr int PHASE_MASK_WAVE = 15;      // Mask for 16-byte wave RAM
    static constexpr int WAVE_NIBBLE_BIT = 21;      // Bit to check for high/low nibble
    
    // Frequency calculation constants
    static constexpr int FREQ_DIVISOR_BASE = 2048;  // Base frequency divisor
    static constexpr int FREQ_SHIFT_SQUARE = 17;    // Shift for square wave frequency
    static constexpr int FREQ_SHIFT_WAVE = 21;      // Shift for wave frequency
    static constexpr int FREQ_CHECK_SHIFT_SQUARE = 4;  // Check shift for square
    static constexpr int FREQ_CHECK_SHIFT_WAVE = 3;    // Check shift for wave
    
    // Length counter constants
    static constexpr int LENGTH_SHIFT = 13;         // Shift for length counter calculation
    static constexpr int LENGTH_MAX_64 = 64;        // Max length for channels 1,2,4
    static constexpr int LENGTH_MAX_256 = 256;      // Max length for channel 3
    static constexpr int LENGTH_MASK = 0x3F;        // Mask for 6-bit length value
    
    // Envelope constants
    static constexpr int ENVELOPE_SHIFT = 15;       // Shift for envelope period
    static constexpr int ENVELOPE_MASK = 7;         // Mask for 3-bit period
    static constexpr int ENVELOPE_MIN = 0;          // Minimum envelope volume
    static constexpr int ENVELOPE_MAX = 15;         // Maximum envelope volume
    
    // Sweep constants
    static constexpr int SWEEP_SHIFT = 14;          // Shift for sweep period
    static constexpr int SWEEP_FREQUENCY_MAX = 2047; // Maximum frequency value
    
    // Register bit positions and masks
    static constexpr int DUTY_CYCLE_SHIFT = 6;
    static constexpr int DUTY_CYCLE_MASK = 0x03;
    static constexpr int SWEEP_PERIOD_SHIFT = 4;
    static constexpr int SWEEP_PERIOD_MASK = 0x07;
    static constexpr int SWEEP_DIRECTION_SHIFT = 3;
    static constexpr int SWEEP_DIRECTION_MASK = 0x01;
    static constexpr int SWEEP_SHIFT_MASK = 0x07;
    static constexpr int VOLUME_SHIFT = 4;
    static constexpr int VOLUME_MASK = 0x0F;
    static constexpr int ENVELOPE_DIRECTION_SHIFT = 3;
    static constexpr int FREQUENCY_HIGH_MASK = 0x07;
    static constexpr int FREQUENCY_LOW_MASK = 0xFF;
    static constexpr int FREQUENCY_HIGH_SHIFT = 8;
    static constexpr int LENGTH_ENABLE_BIT = 0x40;
    static constexpr int TRIGGER_BIT = 0x80;
    static constexpr int OUTPUT_LEVEL_SHIFT = 5;
    static constexpr int OUTPUT_LEVEL_MASK = 0x03;
    static constexpr int DAC_ENABLE_BIT = 0x80;
    static constexpr int DAC_ENABLE_MASK = 0xF8;
    
    // Channel 4 specific constants
    static constexpr int CLOCK_SHIFT_SHIFT = 4;
    static constexpr int CLOCK_SHIFT_MASK = 0x0F;
    static constexpr int WIDTH_MODE_SHIFT = 3;
    static constexpr int WIDTH_MODE_MASK = 0x01;
    static constexpr int DIVISOR_CODE_MASK = 0x07;
    static constexpr int LFSR_INIT = 0x7FFF;        // Initial LFSR value
    static constexpr int LFSR_BIT_MASK = 1;         // Bit 0 mask for LFSR
    static constexpr int LFSR_XOR_SHIFT = 1;        // Shift for XOR calculation
    static constexpr int LFSR_FEEDBACK_BIT = 14;    // Position for 15-bit feedback
    static constexpr int LFSR_WIDTH_BIT = 6;        // Position for 7-bit mode
    static constexpr int LFSR_PHASE_THRESHOLD = 18; // Threshold shift for LFSR advance
    
    // Sample amplitude constants
    static constexpr int SAMPLE_AMPLIFY_SHIFT = 2;  // Shift left 2 = multiply by 4
    static constexpr int SAMPLE_NOISE_MULTIPLY = 3;  // Noise multiply factor
    static constexpr int SAMPLE_VOLUME_SHIFT = 4;   // Final volume shift
    static constexpr int SAMPLE_MIN = -128;         // Minimum signed sample value
    static constexpr int SAMPLE_MAX = 127;          // Maximum signed sample value
    static constexpr int SAMPLE_ZERO = 128;         // Zero point in unsigned format
    static constexpr int SAMPLE_WAVE_OFFSET = 8;    // Offset for wave samples
    
    // Wave output level shifts
    static constexpr int WAVE_VOLUME_FULL_SHIFT = 0;   // 100% volume (no shift)
    static constexpr int WAVE_VOLUME_HALF_SHIFT = 1;   // 50% volume (shift 1)
    static constexpr int WAVE_VOLUME_QUARTER_SHIFT = 2; // 25% volume (shift 2)
    static constexpr int WAVE_VOLUME_BASE = 3;         // Base for shift calculation
    
    // Wave RAM constants
    static constexpr int WAVE_NIBBLE_MASK = 15;     // Mask for 4-bit nibble
    static constexpr int WAVE_NIBBLE_SHIFT = 4;     // Shift for high nibble
    
    // Read-only register return values
    static constexpr u8 REG_WRITE_ONLY = 0xFF;
    static constexpr u8 REG_NR10_UNUSED_BITS = 0x80;
    static constexpr u8 REG_NR11_UNUSED_BITS = 0x3F;
    static constexpr u8 REG_NR14_UNUSED_BITS = 0xBF;
    static constexpr u8 REG_NR21_UNUSED_BITS = 0x3F;
    static constexpr u8 REG_NR24_UNUSED_BITS = 0xBF;
    static constexpr u8 REG_NR30_UNUSED_BITS = 0x7F;
    static constexpr u8 REG_NR32_UNUSED_BITS = 0x9F;
    static constexpr u8 REG_NR34_UNUSED_BITS = 0xBF;
    static constexpr u8 REG_NR44_UNUSED_BITS = 0xBF;
    static constexpr u8 REG_NR52_UNUSED_BITS = 0xF0;
    
    // Reset value
    static constexpr u8 NR52_RESET_VALUE = 0xF0;    // Power off, bits 4-7 always 1
    
    // Channel structures (gnuboy compatible naming)
    struct Channel1 {
        // Sweep
        u8 sweep_period;
        u8 sweep_direction;
        u8 sweep_shift;
        
        // Sound length/Wave pattern
        u8 wave_duty;
        
        // Volume envelope
        u8 initial_volume;
        u8 envelope_direction;
        u8 envelope_period;
        u8 current_volume;
        
        // Frequency
        u16 frequency;
        bool trigger;
        bool length_enable;
        bool enabled;
        bool dac_enabled;
        
        // gnuboy compatible internal state
        bool on;      // Channel enabled (gnuboy: S1.on)
        int pos;      // Phase position (gnuboy: S1.pos)
        int freq;     // Phase increment (gnuboy: S1.freq)
        int cnt;      // Length counter accumulated
        int len;      // Length threshold
        int encnt;    // Envelope counter accumulated
        int enlen;    // Envelope threshold
        int endir;    // Envelope direction (-1 or +1)
        int envol;    // Current envelope volume
        int swcnt;    // Sweep counter accumulated
        int swlen;    // Sweep threshold
        int swfreq;   // Sweep frequency (gnuboy: S1.swfreq)
    } channel1_;
    
    struct Channel2 {
        // Sound length/Wave pattern
        u8 wave_duty;
        
        // Volume envelope
        u8 initial_volume;
        u8 envelope_direction;
        u8 envelope_period;
        
        // Frequency
        u16 frequency;
        bool trigger;
        bool length_enable;
        bool enabled;
        bool dac_enabled;
        
        // gnuboy compatible internal state
        bool on;      // Channel enabled (gnuboy: S2.on)
        int pos;      // Phase position (gnuboy: S2.pos)
        int freq;     // Phase increment (gnuboy: S2.freq)
        int cnt;      // Length counter
        int len;      // Length threshold
        int encnt;    // Envelope counter
        int enlen;    // Envelope threshold
        int endir;    // Envelope direction (-1 or +1)
        int envol;    // Current envelope volume
    } channel2_;
    
    struct Channel3 {
        // Sound on/off
        bool dac_enabled;
        
        // Output level
        u8 output_level;
        
        // Frequency
        u16 frequency;
        bool trigger;
        bool length_enable;
        bool enabled;
        
        // gnuboy compatible internal state
        bool on;      // Channel enabled (gnuboy: S3.on)
        int pos;      // Phase position (gnuboy: S3.pos)
        int freq;     // Phase increment (gnuboy: S3.freq)
        int cnt;      // Length counter
        int len;      // Length threshold
    } channel3_;
    
    struct Channel4 {
        // Volume envelope
        u8 initial_volume;
        u8 envelope_direction;
        u8 envelope_period;
        
        // Polynomial counter
        u8 clock_shift;
        u8 width_mode;
        u8 divisor_code;
        
        bool trigger;
        bool length_enable;
        bool enabled;
        bool dac_enabled;
        
        // gnuboy compatible internal state
        bool on;      // Channel enabled (gnuboy: S4.on)
        u16 lfsr;     // Linear feedback shift register
        int pos;      // Phase position (gnuboy: S4.pos)
        int freq;     // Phase increment (gnuboy: S4.freq)
        int cnt;      // Length counter
        int len;      // Length threshold
        int encnt;    // Envelope counter
        int enlen;    // Envelope threshold
        int endir;    // Envelope direction (-1 or +1)
        int envol;    // Current envelope volume
    } channel4_;
    
    /**
     * Generate a single audio sample
     * @return Stereo sample (left, right) in U8 format (0-255, 128 = silence)
     */
    std::pair<u8, u8> generate_sample();
    
    /**
     * Mix all channels to produce final output
     */
    void mix_channels();
    
    /**
     * Reset APU state
     */
    void reset();
};

} // namespace emugbc
