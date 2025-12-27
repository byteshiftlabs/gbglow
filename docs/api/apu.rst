APU
===

Audio Processing Unit for sound generation.

Generates audio using four channels:

- **Channel 1**: Square wave with frequency sweep
- **Channel 2**: Square wave (no sweep)
- **Channel 3**: Programmable wave pattern
- **Channel 4**: White noise (LFSR-based)

Output is stereo at 44,100 Hz sample rate.

**Header:** ``audio/apu.h``

**Namespace:** ``gbcrush``


Sound Registers
---------------

::

    0xFF10-0xFF14 : Channel 1 (Square with Sweep)
    0xFF16-0xFF19 : Channel 2 (Square)
    0xFF1A-0xFF1E : Channel 3 (Wave)
    0xFF20-0xFF23 : Channel 4 (Noise)
    0xFF24        : NR50 - Channel control / Volume
    0xFF25        : NR51 - Sound output terminal selection
    0xFF26        : NR52 - Sound on/off
    0xFF30-0xFF3F : Wave Pattern RAM (32 4-bit samples)


Constructor
-----------

.. cpp:function:: explicit APU::APU(Memory& memory)

   Creates an APU connected to the specified memory system.
   
   :param memory: Reference to Memory instance


Execution
---------

.. cpp:function:: void APU::step(Cycles cycles)

   Advances APU state and generates audio samples.
   
   :param cycles: Number of M-cycles to advance


Register Access
---------------

.. cpp:function:: u8 APU::read_register(u16 address) const

   Reads from a sound register.
   
   :param address: Register address (0xFF10-0xFF3F)
   :return: Register value
   
   Note: Some register bits are write-only and return 1 when read.

.. cpp:function:: void APU::write_register(u16 address, u8 value)

   Writes to a sound register.
   
   :param address: Register address (0xFF10-0xFF3F)
   :param value: Value to write
   
   Writing to NR52 bit 7 enables/disables all sound.


Audio Output
------------

.. cpp:function:: const std::vector<std::pair<u8, u8>>& APU::get_audio_buffer() const

   Returns the audio sample buffer.
   
   :return: Vector of stereo samples (left, right)
   
   Samples are 8-bit unsigned (0-255, 128 = silence).

.. cpp:function:: void APU::clear_audio_buffer()

   Clears the audio buffer after samples have been consumed.
   
   Call this after copying samples to your audio backend.


Serialization
-------------

.. cpp:function:: void APU::serialize(std::vector<u8>& data) const

   Serializes APU state for save states.
   
   :param data: Vector to append serialized data to

.. cpp:function:: void APU::deserialize(const u8* data, size_t& offset)

   Restores APU state from serialized data.
   
   :param data: Serialized data buffer
   :param offset: Current offset (updated after reading)
