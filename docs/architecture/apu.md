# Audio Processing Unit (APU)

## Overview

The emugbc APU implementation is based on the gnuboy audio emulation approach. This document describes the architecture, implementation details, and the key lessons learned during development.

## Architecture

### Channel Structure

The Game Boy has 4 sound channels:

| Channel | Type | Features |
|---------|------|----------|
| CH1 | Square wave | Frequency sweep, envelope |
| CH2 | Square wave | Envelope only |
| CH3 | Wave | Programmable 32-sample waveform |
| CH4 | Noise | LFSR-based, envelope |

### Key Constants

```cpp
static constexpr int SAMPLE_RATE = 44100;           // Output sample rate
static constexpr int CPU_CLOCK_HZ = 4194304;        // Game Boy CPU clock
static constexpr int RATE = (1 << 21) / SAMPLE_RATE; // ~47.5 (gnuboy timing factor)
static constexpr int CYCLES_PER_SAMPLE = 95;        // CPU cycles per audio sample
```

## Implementation Details

### Sample Generation (gnuboy method)

The core of the APU is `generate_sample()`, which produces one stereo sample per call. Key aspects:

1. **Timing via RATE accumulator**: Instead of a separate frame sequencer, all timing (length, envelope, sweep) uses RATE-based accumulators that increment each sample.

2. **Square wave formula**:
```cpp
s = sqwave[duty][(pos >> 18) & 7] & envol;
pos += freq;
```

3. **Frequency calculation**:
```cpp
int d = 2048 - frequency;
if (RATE > (d << 4)) freq = 0;
else freq = (RATE << 17) / d;
```

4. **Channel scaling** (before panning):
   - CH1/CH2: `s <<= 2` (multiply by 4)
   - CH3: `s <<= (3 - output_level)` (0=mute, 1=4x, 2=2x, 3=1x)
   - CH4: `s += s << 1` (multiply by 3)

5. **Master volume**:
```cpp
l *= (nr50 & 0x07);        // Left volume 0-7
r *= ((nr50 >> 4) & 0x07); // Right volume 0-7
l >>= 4;                   // Divide by 16
r >>= 4;
```

6. **Output format**: U8 (0-255, 128 = silence)

### SDL Audio Integration

The APU generates U8 samples, but SDL uses S16 format:

```cpp
// Convert U8 to S16 with moderate amplification
interleaved[i] = static_cast<i16>((sample - 128) * 128);
```

Buffer size: 1024 samples (~23ms at 44100 Hz) for smooth playback.

## Debugging History

### Original Issues

The initial APU implementation had several critical bugs that prevented proper audio playback, particularly noticeable with the Game Freak logo sound in Pokémon Red.

### Bug 1: Dual Enable Flags

**Problem**: Used separate `enabled` and `dac_enabled` flags that could desynchronize.

```cpp
// WRONG
if (channel1_.enabled && channel1_.dac_enabled) { ... }

// CORRECT (gnuboy)
if (channel1_.on) { ... }
```

**Solution**: Single `on` flag per channel, set on trigger, cleared by length counter or overflow.

### Bug 2: Incorrect Frequency Formula

**Problem**: Custom frequency calculation didn't match gnuboy's exact formula.

```cpp
// WRONG
int calc_square_phase_inc(u16 freq) {
    return (RATE << 17) / (2048 - freq);  // Missing overflow check
}

// CORRECT (gnuboy inline)
int d = 2048 - frequency;
if (RATE > (d << 4)) freq = 0;  // Anti-aliasing filter
else freq = (RATE << 17) / d;
```

**Solution**: Use gnuboy's exact inline formula with anti-aliasing check.

### Bug 3: Separate Frame Sequencer

**Problem**: Implemented a separate 512 Hz frame sequencer for timing.

```cpp
// WRONG
while (frame_sequencer_cycles_ >= CYCLES_PER_FRAME) {
    if (step == 2 || step == 6) { /* sweep */ }
    // ...
}
```

**Solution**: gnuboy handles all timing (length, envelope, sweep) per-sample using RATE accumulators:

```cpp
// CORRECT - in generate_sample()
if (channel1_.enlen && (channel1_.encnt += RATE) >= channel1_.enlen) {
    channel1_.encnt -= channel1_.enlen;
    channel1_.envol += channel1_.endir;
    // ...
}
```

### Bug 4: Envelope Direction Initialization

**Problem**: Incorrectly computed envelope direction on trigger.

```cpp
// WRONG (during trigger)
channel1_.endir = envelope_direction;
channel1_.endir |= channel1_.endir - 1;

// CORRECT
channel1_.endir = (envelope_direction) ? 1 : -1;
```

The `endir |= endir - 1` trick is used when reading from the register, not during trigger.

### Bug 5: Channel 3 Length Scale

**Problem**: Used wrong shift value for wave channel length.

```cpp
// WRONG (from sound_dirty)
channel3_.len = (256 - value) << 20;

// CORRECT (from sound_write)
channel3_.len = (256 - value) << 13;
```

gnuboy has inconsistent values; the `<< 13` from `sound_write()` is correct for register writes.

### Bug 6: Over-amplification

**Problem**: Initial S16 conversion used `* 256`, causing clipping and distortion.

```cpp
// WRONG - causes crackling
sample_s16 = (sample_u8 - 128) * 256;

// CORRECT - moderate amplification
sample_s16 = (sample_u8 - 128) * 128;
```

## Register Map

| Address | Register | Description |
|---------|----------|-------------|
| FF10 | NR10 | CH1 Sweep |
| FF11 | NR11 | CH1 Length/Duty |
| FF12 | NR12 | CH1 Envelope |
| FF13 | NR13 | CH1 Frequency lo |
| FF14 | NR14 | CH1 Frequency hi + Trigger |
| FF16 | NR21 | CH2 Length/Duty |
| FF17 | NR22 | CH2 Envelope |
| FF18 | NR23 | CH2 Frequency lo |
| FF19 | NR24 | CH2 Frequency hi + Trigger |
| FF1A | NR30 | CH3 DAC Enable |
| FF1B | NR31 | CH3 Length |
| FF1C | NR32 | CH3 Output Level |
| FF1D | NR33 | CH3 Frequency lo |
| FF1E | NR34 | CH3 Frequency hi + Trigger |
| FF20 | NR41 | CH4 Length |
| FF21 | NR42 | CH4 Envelope |
| FF22 | NR43 | CH4 Polynomial Counter |
| FF23 | NR44 | CH4 Trigger |
| FF24 | NR50 | Master Volume |
| FF25 | NR51 | Panning |
| FF26 | NR52 | Sound On/Off |
| FF30-FF3F | Wave RAM | 32 4-bit samples |

## Testing

The APU can be tested with:

1. **Pokémon Red Game Freak logo**: Tests CH1 sweep and envelope
2. **blargg's dmg_sound tests**: Comprehensive APU test ROMs
3. **Manual audio inspection**: Compare with gnuboy output

## References

- [gnuboy source code](https://github.com/rofl0r/gnuboy) - Reference implementation
- [Pan Docs](https://gbdev.io/pandocs/Audio.html) - Game Boy audio documentation
- [GBSOUND.txt](http://gbdev.gg8.se/wiki/articles/Gameboy_sound_hardware) - Detailed sound hardware info
