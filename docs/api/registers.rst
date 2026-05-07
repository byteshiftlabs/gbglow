Registers Class
===============

CPU register file for the Sharp LR35902.

File: ``src/core/registers.h``

Class Declaration
-----------------

.. code-block:: cpp

   class Registers {
   public:
       u8 a, f, b, c, d, e, h, l;
       u16 sp, pc;

       enum Flags : u8 {
           FLAG_Z = 0x80,   // Zero
           FLAG_N = 0x40,   // Subtract
           FLAG_H = 0x20,   // Half-carry
           FLAG_C = 0x10    // Carry
       };

       u16 af() const;
       u16 bc() const;
       u16 de() const;
       u16 hl() const;

       void set_af(u16 value);
       void set_bc(u16 value);
       void set_de(u16 value);
       void set_hl(u16 value);

       bool get_flag(Flags flag) const;
       void set_flag(Flags flag, bool value);
       void reset();
   };

Register Pairs
--------------

The eight 8-bit registers can be combined into four 16-bit pairs:

=====  ============  =======================================
Pair   Registers     Purpose
=====  ============  =======================================
AF     A + Flags     Accumulator and flag register
BC     B + C         General purpose / counter
DE     D + E         General purpose / destination
HL     H + L         Memory pointer (used by many opcodes)
=====  ============  =======================================

Flags Register
--------------

Only the upper four bits of ``F`` are used:

====  ======  ====================================
Bit   Flag    Meaning
====  ======  ====================================
7     Z       Set if result is zero
6     N       Set after subtraction instructions
5     H       Set on half-carry (bit 3 → 4)
4     C       Set on carry (bit 7 → carry)
3–0   —       Always zero
====  ======  ====================================

``set_af()`` masks the lower nibble to enforce this constraint.

reset()
-------

Initializes all registers to the post-boot ROM values matching the
original DMG hardware (``A=0x01``, ``F=0xB0``, ``SP=0xFFFE``,
``PC=0x0100``, etc.).
