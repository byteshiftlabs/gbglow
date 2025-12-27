Registers
=========

CPU register set for the Sharp LR35902.

Contains eight 8-bit registers that can be paired into four 16-bit
registers, plus two dedicated 16-bit registers (SP, PC).

**Header:** ``core/registers.h``

**Namespace:** ``emugbc``

.. note::

   This is a plain data structure with no explicit constructor. Use
   ``reset()`` to initialize registers to post-boot ROM values.


8-bit Registers
---------------

.. cpp:member:: u8 Registers::a

   Accumulator register. Primary register for arithmetic and logic operations.

.. cpp:member:: u8 Registers::f

   Flags register. Only upper 4 bits are used:
   
   +------+------+----------------------------------------+
   | Bit  | Flag | Description                            |
   +======+======+========================================+
   | 7    | Z    | Zero flag - set if result is zero      |
   +------+------+----------------------------------------+
   | 6    | N    | Subtract flag - set if subtraction     |
   +------+------+----------------------------------------+
   | 5    | H    | Half-carry flag - carry from bit 3     |
   +------+------+----------------------------------------+
   | 4    | C    | Carry flag - carry from bit 7          |
   +------+------+----------------------------------------+
   | 3-0  | —    | Always 0                               |
   +------+------+----------------------------------------+

.. cpp:member:: u8 Registers::b

   General purpose register B.

.. cpp:member:: u8 Registers::c

   General purpose register C.

.. cpp:member:: u8 Registers::d

   General purpose register D.

.. cpp:member:: u8 Registers::e

   General purpose register E.

.. cpp:member:: u8 Registers::h

   General purpose register H (high byte of HL).

.. cpp:member:: u8 Registers::l

   General purpose register L (low byte of HL).


16-bit Registers
----------------

.. cpp:member:: u16 Registers::sp

   Stack Pointer. Points to the top of the stack in memory.
   Initial value: 0xFFFE

.. cpp:member:: u16 Registers::pc

   Program Counter. Points to the next instruction to execute.
   Initial value: 0x0100 (after boot ROM)


Flag Constants
--------------

.. cpp:enum:: Registers::Flags : u8

   Bit masks for the flags register.
   
   .. cpp:enumerator:: FLAG_Z = 0x80
   
      Zero flag (bit 7)
   
   .. cpp:enumerator:: FLAG_N = 0x40
   
      Subtract flag (bit 6)
   
   .. cpp:enumerator:: FLAG_H = 0x20
   
      Half-carry flag (bit 5)
   
   .. cpp:enumerator:: FLAG_C = 0x10
   
      Carry flag (bit 4)


16-bit Pair Accessors
---------------------

.. cpp:function:: u16 Registers::af() const

   :return: Combined AF register (A << 8 | F)

.. cpp:function:: u16 Registers::bc() const

   :return: Combined BC register (B << 8 | C)

.. cpp:function:: u16 Registers::de() const

   :return: Combined DE register (D << 8 | E)

.. cpp:function:: u16 Registers::hl() const

   :return: Combined HL register (H << 8 | L)

.. cpp:function:: void Registers::set_af(u16 value)

   Sets the AF register pair.
   
   :param value: 16-bit value (high byte → A, low byte → F)
   
   Note: Lower 4 bits of F are always forced to 0.

.. cpp:function:: void Registers::set_bc(u16 value)

   Sets the BC register pair.
   
   :param value: 16-bit value (high byte → B, low byte → C)

.. cpp:function:: void Registers::set_de(u16 value)

   Sets the DE register pair.
   
   :param value: 16-bit value (high byte → D, low byte → E)

.. cpp:function:: void Registers::set_hl(u16 value)

   Sets the HL register pair.
   
   :param value: 16-bit value (high byte → H, low byte → L)


Flag Operations
---------------

.. cpp:function:: bool Registers::get_flag(Flags flag) const

   Gets the state of a flag.
   
   :param flag: Flag bit mask (FLAG_Z, FLAG_N, FLAG_H, or FLAG_C)
   :return: ``true`` if flag is set

.. cpp:function:: void Registers::set_flag(Flags flag, bool value)

   Sets or clears a flag.
   
   :param flag: Flag bit mask
   :param value: ``true`` to set, ``false`` to clear


Initialization
--------------

.. cpp:function:: void Registers::reset()

   Resets all registers to post-boot ROM values (DMG mode):
   
   +----------+--------+
   | Register | Value  |
   +==========+========+
   | A        | 0x01   |
   +----------+--------+
   | F        | 0xB0   |
   +----------+--------+
   | B        | 0x00   |
   +----------+--------+
   | C        | 0x13   |
   +----------+--------+
   | D        | 0x00   |
   +----------+--------+
   | E        | 0xD8   |
   +----------+--------+
   | H        | 0x01   |
   +----------+--------+
   | L        | 0x4D   |
   +----------+--------+
   | SP       | 0xFFFE |
   +----------+--------+
   | PC       | 0x0100 |
   +----------+--------+
