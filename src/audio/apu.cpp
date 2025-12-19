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
    audio_buffer_.reserve(SAMPLE_RATE / 60);  // Reserve space for one frame at 60fps
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
    nr52_ = 0xF0;  // Power off, but bits 4-7 are always 1
    
    cycle_accumulator_ = 0;
    frame_sequencer_cycles_ = 0;
    frame_sequencer_step_ = 0;
}

// Helper functions to calculate phase increments
namespace {
    // RATE calculation (gnuboy compatible)
    static constexpr int RATE = (1 << 21) / 44100;  // ~47.5 cycles/sample
    
    // Calculate phase increment for square wave channels (Ch1, Ch2)
    // Usa método de gnuboy: (RATE << 17) / divisor
    int calc_square_phase_inc(u16 frequency) {
        if (frequency == 0) return 0;
        
        int divisor = 2048 - frequency;
        if (divisor <= 0) return 0;
        
        // Filtro anti-aliasing (igual que gnuboy)
        if (RATE > (divisor << 4)) return 0;
        
        // Fórmula gnuboy: (RATE << 17) / divisor
        return (RATE << 17) / divisor;
    }
    
    // Calculate phase increment for wave channel (Ch3)
    // Usa método de gnuboy: (RATE << 21) / divisor
    int calc_wave_phase_inc(u16 frequency) {
        if (frequency == 0 || frequency >= 2047) return 0;
        
        int divisor = 2048 - frequency;
        
        // Filtro anti-aliasing
        if (RATE > (divisor << 3)) return 0;
        
        // Fórmula gnuboy: (RATE << 21) / divisor
        return (RATE << 21) / divisor;
    }
    
    // Calculate phase increment for noise channel (Ch4)
    // Usa tabla de divisores y método de gnuboy
    int calc_noise_phase_inc(u8 clock_shift, u8 divisor_code) {
        // Tabla de divisores de gnuboy (sound.c línea 37)
        static const int freqtab[8] = {
            (1<<14)*2,  // divisor 8
            (1<<14),    // divisor 16
            (1<<14)/2,  // divisor 32
            (1<<14)/3,  // divisor 48
            (1<<14)/4,  // divisor 64
            (1<<14)/5,  // divisor 80
            (1<<14)/6,  // divisor 96
            (1<<14)/7   // divisor 112
        };
        
        int shift = clock_shift & 0x0F;
        int freq = (freqtab[divisor_code & 7] >> shift) * RATE;
        
        // Limitar frecuencia máxima
        if (freq >> 18) freq = 1 << 18;
        
        return freq;
    }
}  // end anonymous namespace

void APU::step(Cycles cycles) {
    // Check if APU is powered on
    if ((nr52_ & NR52_POWER_BIT) == 0) {
        return;
    }
    
    cycle_accumulator_ += cycles;
    frame_sequencer_cycles_ += cycles;
    
    // Frame sequencer runs at 512 Hz (every ~8192 cycles)
    // GNUBOY: Length y envelope ahora se manejan por sample, no por frame sequencer
    // Sweep se maneja aquí en el frame sequencer (steps 2 y 6)
    while (frame_sequencer_cycles_ >= CYCLES_PER_FRAME) {
        frame_sequencer_cycles_ -= CYCLES_PER_FRAME;
        
        // Sweep para Channel 1 (Step 2, 6 @ 128 Hz)
        if (frame_sequencer_step_ == 2 || frame_sequencer_step_ == 6) {
            if (channel1_.enabled && channel1_.sweep_period > 0 && channel1_.sweep_shift > 0) {
                // Calculate new frequency
                int delta = channel1_.frequency >> channel1_.sweep_shift;
                int new_freq;
                
                if (channel1_.sweep_direction) {
                    // Decrease frequency (negate)
                    new_freq = channel1_.frequency - delta;
                } else {
                    // Increase frequency
                    new_freq = channel1_.frequency + delta;
                }
                
                // Check overflow (frequency > 2047 disables channel)
                if (new_freq > 2047) {
                    channel1_.enabled = false;
                } else if (new_freq >= 0 && channel1_.sweep_shift > 0) {
                    channel1_.frequency = static_cast<u16>(new_freq);
                    channel1_.phase_increment = calc_square_phase_inc(channel1_.frequency);
                }
            }
        }
        
        // Advance frame sequencer (0-7 loop)
        frame_sequencer_step_ = (frame_sequencer_step_ + 1) & 0x07;
    }
    
    // Generate samples when we've accumulated enough cycles
    while (cycle_accumulator_ >= CYCLES_PER_SAMPLE) {
        auto sample = generate_sample();
        audio_buffer_.push_back(sample);
        cycle_accumulator_ -= CYCLES_PER_SAMPLE;
    }
}

std::pair<u8, u8> APU::generate_sample() {
    // GNUBOY: Acumular como int signed, luego convertir a U8 (0-255)
    int left = 0;
    int right = 0;
    
    // Square wave duty cycle patterns - GNUBOY METHOD
    // Usa valores {0, -1} y aplica volumen con AND bitwise
    static const int duty_patterns[4][8] = {
        {  0, 0,-1, 0, 0, 0, 0, 0 },  // 12.5%
        {  0,-1,-1, 0, 0, 0, 0, 0 },  // 25%
        { -1,-1,-1,-1, 0, 0, 0, 0 },  // 50%
        { -1, 0, 0,-1,-1,-1,-1,-1 }   // 75% (invertida)
    };
    
    // Channel 1: Square wave with sweep
    if (channel1_.enabled && channel1_.dac_enabled) {
        int duty_index = channel1_.wave_duty & 0x03;
        
        // Usar bit 18 (gnuboy method)
        int phase_index = (channel1_.phase >> 18) & 7;
        int wave_pattern = duty_patterns[duty_index][phase_index];
        
        // GNUBOY: s = pattern & volume (usa envol en lugar de current_volume)
        int s = wave_pattern & channel1_.envol;
        channel1_.phase += channel1_.phase_increment;
        
        // Length counter (timing continuo gnuboy)
        if (channel1_.length_enable && ((channel1_.cnt += RATE) >= channel1_.len)) {
            channel1_.enabled = false;
        }
        
        // Envelope (timing continuo gnuboy)
        if (channel1_.enlen && (channel1_.encnt += RATE) >= channel1_.enlen) {
            channel1_.encnt -= channel1_.enlen;
            channel1_.envol += channel1_.endir;
            if (channel1_.envol < 0) {
                channel1_.envol = 0;
                if (channel1_.endir < 0) channel1_.enabled = false;  // Deshabilitar si decrece a 0
            }
            if (channel1_.envol > 15) channel1_.envol = 15;
        }
        
        s <<= 2;  // Escalar por 4
        
        // Mix to left/right based on NR51
        if (nr51_ & 0x01) right += s;
        if (nr51_ & 0x10) left += s;
    }
    
    // Channel 2: Square wave
    if (channel2_.enabled && channel2_.dac_enabled) {
        int duty_index = channel2_.wave_duty & 0x03;
        
        // Usar bit 18 (gnuboy method)
        int phase_index = (channel2_.phase >> 18) & 7;
        int wave_pattern = duty_patterns[duty_index][phase_index];
        
        // GNUBOY: s = pattern & volume
        int s = wave_pattern & channel2_.envol;
        channel2_.phase += channel2_.phase_increment;
        
        // Length counter (timing continuo gnuboy)
        if (channel2_.length_enable && ((channel2_.cnt += RATE) >= channel2_.len)) {
            channel2_.enabled = false;
        }
        
        // Envelope (timing continuo gnuboy)
        if (channel2_.enlen && (channel2_.encnt += RATE) >= channel2_.enlen) {
            channel2_.encnt -= channel2_.enlen;
            channel2_.envol += channel2_.endir;
            if (channel2_.envol < 0) {
                channel2_.envol = 0;
                if (channel2_.endir < 0) channel2_.enabled = false;  // Deshabilitar si decrece a 0
            }
            if (channel2_.envol > 15) channel2_.envol = 15;
        }
        
        s <<= 2;  // Escalar por 4
        
        // Mix to left/right based on NR51
        if (nr51_ & 0x02) right += s;
        if (nr51_ & 0x20) left += s;
    }
    
    // Channel 3: Wave pattern
    if (channel3_.enabled && channel3_.dac_enabled) {
        // GNUBOY: Con (RATE << 21), tenemos 32 samples en bits 21-25
        // Bits 22-25 = índice del byte (0-15)
        // Bit 21 = selección de nibble (0=high, 1=low)
        int sample_index = (channel3_.phase >> 21) & 31;  // 0-31 samples
        int wave_byte_index = sample_index >> 1;          // Dividir por 2 para obtener byte (0-15)
        bool use_low_nibble = sample_index & 1;           // Bit 0 del sample_index
        
        u8 wave_byte = wave_ram_[wave_byte_index];
        u8 wave_sample;
        
        // High nibble = samples pares (0,2,4...), Low nibble = samples impares (1,3,5...)
        if (use_low_nibble) {
            wave_sample = wave_byte & 0x0F;
        } else {
            wave_sample = wave_byte >> 4;
        }
        
        channel3_.phase += channel3_.phase_increment;
        
        // Length counter (timing continuo gnuboy)
        if (channel3_.length_enable && ((channel3_.cnt += RATE) >= channel3_.len)) {
            channel3_.enabled = false;
        }
        
        // GNUBOY: Aplicar output level y luego centrar/escalar
        int s = wave_sample;
        
        if (channel3_.output_level == 0) {
            s = 0;  // Mute
        } else {
            // 1=no shift (100%), 2=shift right 1 (50%), 3=shift right 2 (25%)
            s >>= (channel3_.output_level - 1);
        }
        
        // GNUBOY: Centrar en 0 y escalar: (sample - 8) << 1
        s = (s - 8) << 1;
        
        // Mix to left/right based on NR51
        if (nr51_ & 0x04) right += s;
        if (nr51_ & 0x40) left += s;
    }
    
    // Channel 4: Noise
    if (channel4_.enabled && channel4_.dac_enabled) {
        // Advance LFSR clock
        channel4_.phase += channel4_.phase_increment;
        
        while (channel4_.phase >= 65536) {
            channel4_.phase -= 65536;
            
            // Step LFSR (15-bit or 7-bit mode)
            int xor_result = (channel4_.lfsr & 0x01) ^ ((channel4_.lfsr >> 1) & 0x01);
            channel4_.lfsr >>= 1;
            channel4_.lfsr |= (xor_result << 14);
            
            if (channel4_.width_mode) {
                // 7-bit mode
                channel4_.lfsr &= ~0x40;
                channel4_.lfsr |= (xor_result << 6);
            }
        }
        
        // Length counter (timing continuo gnuboy)
        if (channel4_.length_enable && ((channel4_.cnt += RATE) >= channel4_.len)) {
            channel4_.enabled = false;
        }
        
        // Envelope (timing continuo gnuboy)
        if (channel4_.enlen > 0 && ((channel4_.encnt += RATE) >= channel4_.enlen)) {
            channel4_.encnt -= channel4_.enlen;
            channel4_.envol += channel4_.endir;
            
            // Clamp 0-15 y deshabilitar si decrece a 0
            if (channel4_.envol < 0) {
                channel4_.envol = 0;
                if (channel4_.endir < 0) channel4_.enabled = false;
            }
            if (channel4_.envol > 15) channel4_.envol = 15;
        }
        
        // Noise output based on LFSR bit 0 (GNUBOY METHOD)
        int s = (channel4_.lfsr & 0x01) ? 0 : -1;
        s = s & channel4_.envol;
        // GNUBOY: multiplicar por 3 para ruido (s += s << 1)
        s += s << 1;
        
        // Mix to left/right based on NR51
        if (nr51_ & 0x08) right += s;
        if (nr51_ & 0x80) left += s;
    }
    
    // Apply master volume from NR50 (0-7 scale)
    int left_volume = (nr50_ >> 4) & 0x07;
    int right_volume = nr50_ & 0x07;
    
    // GNUBOY: Divide by (16 - master_volume) for attenuation
    // This gives proper volume scaling without overflow
    // Volume 7 = divide by 9, Volume 0 = divide by 16 (max attenuation)
    if (left_volume) {
        left /= (16 - left_volume);
    } else {
        left = 0;  // Volume 0 = silence
    }
    
    if (right_volume) {
        right /= (16 - right_volume);
    } else {
        right = 0;  // Volume 0 = silence
    }
    
    // GNUBOY: Convert to U8 format (0-255 range, 128 = silence)
    // Clamp to signed byte range first (-128 to 127)
    left = std::max(-128, std::min(127, left));
    right = std::max(-128, std::min(127, right));
    
    // Add 128 to convert from signed (-128..127) to unsigned (0..255)
    u8 left_u8 = static_cast<u8>(left + 128);
    u8 right_u8 = static_cast<u8>(right + 128);
    
    return {left_u8, right_u8};
}

u8 APU::read_register(u16 address) const {
    // Channel 1 registers
    if (address == REG_NR10) {
        return 0x80 | (channel1_.sweep_period << 4) | 
               (channel1_.sweep_direction << 3) | channel1_.sweep_shift;
    }
    if (address == REG_NR11) {
        return (channel1_.wave_duty << 6) | 0x3F;
    }
    if (address == REG_NR12) {
        return (channel1_.initial_volume << 4) | 
               (channel1_.envelope_direction << 3) | channel1_.envelope_period;
    }
    if (address == REG_NR13) {
        return 0xFF;  // Write-only
    }
    if (address == REG_NR14) {
        return 0xBF | (channel1_.length_enable ? 0x40 : 0x00);
    }
    
    // Channel 2 registers
    if (address == REG_NR21) {
        return (channel2_.wave_duty << 6) | 0x3F;
    }
    if (address == REG_NR22) {
        return (channel2_.initial_volume << 4) | 
               (channel2_.envelope_direction << 3) | channel2_.envelope_period;
    }
    if (address == REG_NR23) {
        return 0xFF;  // Write-only
    }
    if (address == REG_NR24) {
        return 0xBF | (channel2_.length_enable ? 0x40 : 0x00);
    }
    
    // Channel 3 registers
    if (address == REG_NR30) {
        return 0x7F | (channel3_.dac_enabled ? 0x80 : 0x00);
    }
    if (address == REG_NR31) {
        return 0xFF;  // Write-only
    }
    if (address == REG_NR32) {
        return 0x9F | (channel3_.output_level << 5);
    }
    if (address == REG_NR33) {
        return 0xFF;  // Write-only
    }
    if (address == REG_NR34) {
        return 0xBF | (channel3_.length_enable ? 0x40 : 0x00);
    }
    
    // Channel 4 registers
    if (address == REG_NR41) {
        return 0xFF;  // Write-only
    }
    if (address == REG_NR42) {
        return (channel4_.initial_volume << 4) | 
               (channel4_.envelope_direction << 3) | channel4_.envelope_period;
    }
    if (address == REG_NR43) {
        return (channel4_.clock_shift << 4) | 
               (channel4_.width_mode << 3) | channel4_.divisor_code;
    }
    if (address == REG_NR44) {
        return 0xBF | (channel4_.length_enable ? 0x40 : 0x00);
    }
    
    // Control registers
    if (address == REG_NR50) {
        return nr50_;
    }
    if (address == REG_NR51) {
        return nr51_;
    }
    if (address == REG_NR52) {
        u8 status = nr52_ & 0xF0;
        if (channel1_.enabled) status |= NR52_CH1_ON;
        if (channel2_.enabled) status |= NR52_CH2_ON;
        if (channel3_.enabled) status |= NR52_CH3_ON;
        if (channel4_.enabled) status |= NR52_CH4_ON;
        return status;
    }
    
    // Wave RAM
    if (address >= WAVE_RAM_START && address <= WAVE_RAM_END) {
        // IMPORTANTE: Wave RAM solo es accesible cuando Channel 3 está apagado o DAC deshabilitado
        // Cuando está activo, solo se puede acceder al byte siendo reproducido actualmente
        // Esto es crítico para sonidos como el de Game Freak en Pokemon
        if (channel3_.enabled && channel3_.dac_enabled) {
            // Cuando está activo, retornar el byte que se está leyendo ahora
            // gnuboy: bits 22-25 de phase contienen el índice del byte (0-15)
            int current_byte_index = (channel3_.phase >> 22) & 0x0F;
            return wave_ram_[current_byte_index];
        }
        return wave_ram_[address - WAVE_RAM_START];
    }
    
    return 0xFF;
}

void APU::write_register(u16 address, u8 value) {
    // If APU is off, ignore all writes except to NR52
    if ((nr52_ & NR52_POWER_BIT) == 0 && address != REG_NR52) {
        return;
    }
    
    // Channel 1 registers
    if (address == REG_NR10) {
        channel1_.sweep_period = (value >> 4) & 0x07;
        channel1_.sweep_direction = (value >> 3) & 0x01;
        channel1_.sweep_shift = value & 0x07;
        return;
    }
    if (address == REG_NR11) {
        channel1_.wave_duty = (value >> 6) & 0x03;
        // gnuboy: len = (64 - (value & 0x3F)) << 13
        channel1_.len = (64 - (value & 0x3F)) << 13;
        return;
    }
    if (address == REG_NR12) {
        channel1_.initial_volume = (value >> 4) & 0x0F;
        channel1_.envelope_direction = (value >> 3) & 0x01;
        channel1_.envelope_period = value & 0x07;
        // gnuboy: enlen = (value & 7) << 15
        channel1_.enlen = channel1_.envelope_period << 15;
        // gnuboy: endir = (value>>3) & 1; endir |= endir - 1; (convierte 0->-1, 1->+1)
        channel1_.endir = channel1_.envelope_direction;
        channel1_.endir |= channel1_.endir - 1;
        channel1_.envol = channel1_.initial_volume;
        // DAC is enabled if top 5 bits are not all 0
        channel1_.dac_enabled = (value & 0xF8) != 0;
        if (!channel1_.dac_enabled) {
            channel1_.enabled = false;
        }
        return;
    }
    if (address == REG_NR13) {
        channel1_.frequency = (channel1_.frequency & 0x0700) | value;
        channel1_.phase_increment = calc_square_phase_inc(channel1_.frequency);
        return;
    }
    if (address == REG_NR14) {
        channel1_.frequency = (channel1_.frequency & 0x00FF) | ((value & 0x07) << 8);
        channel1_.phase_increment = calc_square_phase_inc(channel1_.frequency);
        channel1_.length_enable = (value & 0x40) != 0;
        
        if (value & 0x80) {  // Trigger
            channel1_.enabled = true;
            channel1_.phase = 0;
            // gnuboy: reset counters
            channel1_.cnt = 0;
            channel1_.encnt = 0;
            channel1_.swcnt = 0;  // Reset sweep counter on trigger
            channel1_.envol = channel1_.initial_volume;
            // Si len no fue configurado, usar default
            if (channel1_.len == 0) {
                channel1_.len = 64 << 13;
            }
            // Sweep overflow check on trigger
            if (channel1_.sweep_shift > 0) {
                int delta = channel1_.frequency >> channel1_.sweep_shift;
                int new_freq = channel1_.sweep_direction ? 
                    (channel1_.frequency - delta) : (channel1_.frequency + delta);
                if (new_freq > 2047) {
                    channel1_.enabled = false;
                }
            }
        }
        return;
    }
    
    // Channel 2 registers
    if (address == REG_NR21) {
        channel2_.wave_duty = (value >> 6) & 0x03;
        // gnuboy: len = (64 - (value & 0x3F)) << 13
        channel2_.len = (64 - (value & 0x3F)) << 13;
        return;
    }
    if (address == REG_NR22) {
        channel2_.initial_volume = (value >> 4) & 0x0F;
        channel2_.envelope_direction = (value >> 3) & 0x01;
        channel2_.envelope_period = value & 0x07;
        // gnuboy: enlen = (value & 7) << 15
        channel2_.enlen = channel2_.envelope_period << 15;
        // gnuboy: endir = (value>>3) & 1; endir |= endir - 1; (convierte 0->-1, 1->+1)
        channel2_.endir = channel2_.envelope_direction;
        channel2_.endir |= channel2_.endir - 1;
        channel2_.envol = channel2_.initial_volume;
        // DAC is enabled if top 5 bits are not all 0
        channel2_.dac_enabled = (value & 0xF8) != 0;
        if (!channel2_.dac_enabled) {
            channel2_.enabled = false;
        }
        return;
    }
    if (address == REG_NR23) {
        channel2_.frequency = (channel2_.frequency & 0x0700) | value;
        channel2_.phase_increment = calc_square_phase_inc(channel2_.frequency);
        return;
    }
    if (address == REG_NR24) {
        channel2_.frequency = (channel2_.frequency & 0x00FF) | ((value & 0x07) << 8);
        channel2_.phase_increment = calc_square_phase_inc(channel2_.frequency);
        channel2_.length_enable = (value & 0x40) != 0;
        
        if (value & 0x80) {  // Trigger
            channel2_.enabled = true;
            channel2_.phase = 0;
            // gnuboy: reset counters
            channel2_.cnt = 0;
            channel2_.encnt = 0;
            channel2_.envol = channel2_.initial_volume;
            // Si len no fue configurado, usar default
            if (channel2_.len == 0) {
                channel2_.len = 64 << 13;
            }
        }
        return;
    }
    
    // Channel 3 registers
    if (address == REG_NR30) {
        channel3_.dac_enabled = (value & 0x80) != 0;
        if (!channel3_.dac_enabled) {
            channel3_.enabled = false;
        }
        return;
    }
    if (address == REG_NR31) {
        // gnuboy: len = (256 - value) << 20 (different scale for wave channel)
        channel3_.len = (256 - value) << 20;
        return;
    }
    if (address == REG_NR32) {
        channel3_.output_level = (value >> 5) & 0x03;
        return;
    }
    if (address == REG_NR33) {
        channel3_.frequency = (channel3_.frequency & 0x0700) | value;
        channel3_.phase_increment = calc_wave_phase_inc(channel3_.frequency);
        return;
    }
    if (address == REG_NR34) {
        channel3_.frequency = (channel3_.frequency & 0x00FF) | ((value & 0x07) << 8);
        channel3_.phase_increment = calc_wave_phase_inc(channel3_.frequency);
        channel3_.length_enable = (value & 0x40) != 0;
        
        if (value & 0x80) {  // Trigger
            if (channel3_.dac_enabled) {
                channel3_.enabled = true;
                channel3_.phase = 0;
                // gnuboy: reset counter
                channel3_.cnt = 0;
                // Si len no fue configurado, usar default
                if (channel3_.len == 0) {
                    channel3_.len = 256 << 20;
                }
            }
        }
        return;
    }
    
    // Channel 4 registers
    if (address == REG_NR41) {
        // gnuboy: len = (64 - (value & 0x3F)) << 13
        channel4_.len = (64 - (value & 0x3F)) << 13;
        return;
    }
    if (address == REG_NR42) {
        channel4_.initial_volume = (value >> 4) & 0x0F;
        channel4_.envelope_direction = (value >> 3) & 0x01;
        channel4_.envelope_period = value & 0x07;
        // gnuboy: enlen = (value & 7) << 15
        channel4_.enlen = channel4_.envelope_period << 15;
        // gnuboy: endir = (value>>3) & 1; endir |= endir - 1; (convierte 0->-1, 1->+1)
        channel4_.endir = channel4_.envelope_direction;
        channel4_.endir |= channel4_.endir - 1;
        channel4_.envol = channel4_.initial_volume;
        // DAC is enabled if top 5 bits are not all 0
        channel4_.dac_enabled = (value & 0xF8) != 0;
        if (!channel4_.dac_enabled) {
            channel4_.enabled = false;
        }
        return;
    }
    if (address == REG_NR43) {
        channel4_.clock_shift = (value >> 4) & 0x0F;
        channel4_.width_mode = (value >> 3) & 0x01;
        channel4_.divisor_code = value & 0x07;
        channel4_.phase_increment = calc_noise_phase_inc(channel4_.clock_shift, channel4_.divisor_code);
        return;
    }
    if (address == REG_NR44) {
        channel4_.length_enable = (value & 0x40) != 0;
        
        if (value & 0x80) {  // Trigger
            channel4_.enabled = true;
            channel4_.lfsr = 0x7FFF;
            channel4_.phase = 0;
            // gnuboy: reset counters
            channel4_.cnt = 0;
            channel4_.encnt = 0;
            channel4_.envol = channel4_.initial_volume;
            // Si len no fue configurado, usar default
            if (channel4_.len == 0) {
                channel4_.len = 64 << 13;
            }
        }
        return;
    }
    
    // Control registers
    if (address == REG_NR50) {
        nr50_ = value;
        return;
    }
    if (address == REG_NR51) {
        nr51_ = value;
        return;
    }
    if (address == REG_NR52) {
        bool was_on = (nr52_ & NR52_POWER_BIT) != 0;
        bool now_on = (value & NR52_POWER_BIT) != 0;
        
        if (was_on && !now_on) {
            // Power off - reset all registers
            reset();
        } else if (!was_on && now_on) {
            // Power on
            nr52_ |= NR52_POWER_BIT;
        }
        return;
    }
    
    // Wave RAM
    if (address >= WAVE_RAM_START && address <= WAVE_RAM_END) {
        // IMPORTANTE: Wave RAM solo es escribible cuando Channel 3 está apagado o DAC deshabilitado
        // Cuando está activo, las escrituras solo afectan al byte siendo reproducido actualmente
        // Esto es crítico para sonidos como el de Game Freak en Pokemon
        if (channel3_.enabled && channel3_.dac_enabled) {
            // Cuando está activo, solo escribir al byte que se está leyendo ahora
            // gnuboy: bits 22-25 de phase contienen el índice del byte (0-15)
            int current_byte_index = (channel3_.phase >> 22) & 0x0F;
            wave_ram_[current_byte_index] = value;
        } else {
            // Cuando está inactivo, escritura normal
            wave_ram_[address - WAVE_RAM_START] = value;
        }
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
