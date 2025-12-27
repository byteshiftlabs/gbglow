CPU
===

Sharp LR35902 CPU emulation.

The Game Boy CPU is a modified Z80 running at 4.194304 MHz. Key differences
from the Z80 include no shadow registers, different interrupt handling,
and modified instruction set (256 base opcodes + 256 CB-prefixed opcodes).

**Header:** ``core/cpu.h``

**Namespace:** ``emugbc``


Constructor
-----------

.. cpp:function:: explicit CPU::CPU(Memory& memory)

   Creates a CPU connected to the specified memory system.
   
   :param memory: Reference to the Memory instance


Execution
---------

.. cpp:function:: Cycles CPU::step()

   Executes one instruction.
   
   :return: Number of M-cycles consumed (4-24 cycles depending on instruction)
   
   One M-cycle equals 4 T-states (clock cycles).

.. cpp:function:: void CPU::handle_interrupts()

   Checks for pending interrupts and services them if IME is enabled.
   
   Interrupt priority (highest to lowest):
   
   1. VBlank (0x0040)
   2. LCD STAT (0x0048)
   3. Timer (0x0050)
   4. Serial (0x0058)
   5. Joypad (0x0060)

.. cpp:function:: void CPU::reset()

   Resets CPU to post-boot ROM state.
   
   Register values after reset (DMG mode):
   
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


Register Access
---------------

.. cpp:function:: const Registers& CPU::registers() const
.. cpp:function:: Registers& CPU::registers()

   :return: Reference to CPU registers
   
   See :doc:`registers` for register structure details.


Interrupt Control
-----------------

.. cpp:function:: bool CPU::ime() const

   :return: Current state of Interrupt Master Enable flag

.. cpp:function:: void CPU::set_ime(bool value)

   :param value: New IME state
   
   When IME is disabled, no interrupts are serviced regardless of IE register.

.. cpp:function:: void CPU::request_interrupt(u8 interrupt_bit)

   Requests an interrupt by setting the corresponding bit in IF register.
   
   :param interrupt_bit: Interrupt bit to set
   
   +------+------------------+
   | Bit  | Interrupt        |
   +======+==================+
   | 0x01 | VBlank           |
   +------+------------------+
   | 0x02 | LCD STAT         |
   +------+------------------+
   | 0x04 | Timer            |
   +------+------------------+
   | 0x08 | Serial           |
   +------+------------------+
   | 0x10 | Joypad           |
   +------+------------------+


CPU State
---------

.. cpp:function:: bool CPU::is_halted() const

   :return: ``true`` if CPU is in HALT mode

.. cpp:function:: void CPU::set_halted(bool value)

   :param value: Halted state
   
   HALT mode stops CPU execution until an interrupt occurs.


Serialization
-------------

.. cpp:function:: void CPU::serialize(std::vector<u8>& data) const

   Serializes CPU state for save states.
   
   :param data: Vector to append serialized data to

.. cpp:function:: void CPU::deserialize(const u8* data, size_t& offset)

   Restores CPU state from serialized data.
   
   :param data: Serialized data buffer
   :param offset: Current offset (updated after reading)
