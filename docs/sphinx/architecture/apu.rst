APU (Audio Processing Unit)
===========================

The APU generates Game Boy audio using a hardware-accurate implementation.

Overview
--------

The Game Boy APU consists of 4 sound channels:

.. list-table:: Sound Channels
   :header-rows: 1

   * - Channel
     - Type
     - Features
   * - CH1
     - Square wave
     - Frequency sweep, volume envelope
   * - CH2
     - Square wave
     - Volume envelope
   * - CH3
     - Wave
     - 32-sample programmable waveform
   * - CH4
     - Noise
     - LFSR-based white noise, envelope

Architecture
------------

Sample Generation
~~~~~~~~~~~~~~~~~

The APU generates samples at 44100 Hz by:

1. Accumulating CPU cycles until enough for one sample (~95 cycles)
2. Reading channel state and computing waveform values
3. Mixing channels according to NR51 panning
4. Applying master volume from NR50
5. Converting to U8 format (0-255, 128 = silence)

.. code-block:: cpp

    // Core timing constant
    static constexpr int RATE = (1 << 21) / 44100;  // ~47.5
    
    // Square wave sample generation
    s = sqwave[duty][(pos >> 18) & 7] & envol;
    pos += freq;

Frequency Calculation
~~~~~~~~~~~~~~~~~~~~~

For square wave channels:

.. code-block:: cpp

    int d = 2048 - frequency;
    if (RATE > (d << 4)) freq = 0;  // Anti-aliasing
    else freq = (RATE << 17) / d;

For wave channel:

.. code-block:: cpp

    int d = 2048 - frequency;
    if (RATE > (d << 3)) freq = 0;
    else freq = (RATE << 21) / d;

Channel Scaling
~~~~~~~~~~~~~~~

Each channel is scaled before mixing:

- **CH1/CH2**: ``s <<= 2`` (multiply by 4)
- **CH3**: ``s <<= (3 - output_level)``
- **CH4**: ``s += s << 1`` (multiply by 3)

Register Map
------------

Control Registers
~~~~~~~~~~~~~~~~~

.. list-table::
   :header-rows: 1

   * - Address
     - Register
     - Description
   * - FF24
     - NR50
     - Master volume (bits 0-2: right, bits 4-6: left)
   * - FF25
     - NR51
     - Channel panning (bit N: CHn→right, bit N+4: CHn→left)
   * - FF26
     - NR52
     - Sound on/off (bit 7 controls power)

Channel 1 (Square + Sweep)
~~~~~~~~~~~~~~~~~~~~~~~~~~

.. list-table::
   :header-rows: 1

   * - Address
     - Register
     - Description
   * - FF10
     - NR10
     - Sweep (period, direction, shift)
   * - FF11
     - NR11
     - Length/Duty cycle
   * - FF12
     - NR12
     - Volume envelope
   * - FF13
     - NR13
     - Frequency low 8 bits
   * - FF14
     - NR14
     - Frequency high 3 bits + trigger

Channel 2 (Square)
~~~~~~~~~~~~~~~~~~

Same as CH1 but without sweep (FF16-FF19).

Channel 3 (Wave)
~~~~~~~~~~~~~~~~

.. list-table::
   :header-rows: 1

   * - Address
     - Register
     - Description
   * - FF1A
     - NR30
     - DAC enable (bit 7)
   * - FF1B
     - NR31
     - Length (256 - value)
   * - FF1C
     - NR32
     - Output level (0=mute, 1=100%, 2=50%, 3=25%)
   * - FF1D-FF1E
     - NR33-34
     - Frequency + trigger
   * - FF30-FF3F
     - Wave RAM
     - 32 4-bit samples

Channel 4 (Noise)
~~~~~~~~~~~~~~~~~

.. list-table::
   :header-rows: 1

   * - Address
     - Register
     - Description
   * - FF20
     - NR41
     - Length
   * - FF21
     - NR42
     - Volume envelope
   * - FF22
     - NR43
     - Polynomial counter (shift, width, divisor)
   * - FF23
     - NR44
     - Trigger + length enable

Implementation Notes
--------------------

Key Design Decisions
~~~~~~~~~~~~~~~~~~~~

1. **Single ``on`` flag**: Uses one flag per channel instead of separate
   ``enabled`` and ``dac_enabled`` flags.

2. **RATE-based timing**: All timing (length, envelope, sweep) uses RATE
   accumulators that increment each sample, rather than a separate frame
   sequencer.

3. **Inline frequency calculation**: Frequencies are recalculated inline
   when registers change.

SDL Integration
~~~~~~~~~~~~~~~

Audio is output via SDL2:

- Format: AUDIO_S16SYS (16-bit signed)
- Channels: 2 (stereo)
- Sample rate: 44100 Hz
- Buffer size: 1024 samples (~23ms)

U8 samples are converted to S16:

.. code-block:: cpp

    s16_sample = (u8_sample - 128) * 128;

Testing
-------

The APU is tested with:

1. **Pokémon Red Game Freak logo** - Tests sweep and envelope
2. **blargg's dmg_sound** - Comprehensive test ROMs

See Also
--------

- :doc:`memory` - Audio register access
- `docs/architecture/apu.md` - Detailed implementation notes
