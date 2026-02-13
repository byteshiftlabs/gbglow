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
    
    // Channel structures (to be implemented)
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
        
        // Internal state (gnuboy compatible)
        int phase;
        int phase_increment;  // Pre-calculated frequency
        int cnt;      // Length counter (accumulated)
        int len;      // Length threshold
        int encnt;    // Envelope counter (accumulated)
        int enlen;    // Envelope threshold
        int endir;    // Envelope direction (-1 or +1)
        int envol;    // Current envelope volume
        int swcnt;    // Sweep counter
        int swlen;    // Sweep threshold
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
        
        // Internal state (gnuboy compatible)
        int phase;
        int phase_increment;
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
        
        // Internal state (gnuboy compatible)
        int phase;
        int phase_increment;
        int cnt;      // Length counter
        int len;      // Length threshold (uses different scale)
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
        
        // Internal state (gnuboy compatible)
        u16 lfsr;
        int phase;
        int phase_increment;
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
