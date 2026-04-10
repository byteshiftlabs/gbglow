CPU Architecture
================

The Sharp LR35902 CPU is a hybrid of Intel 8080 and Zilog Z80, with unique modifications.

Registers
---------

8-bit Registers
~~~~~~~~~~~~~~~

The CPU has 8 general-purpose 8-bit registers:

.. code-block:: cpp

   struct Registers {
       u8 a;  // Accumulator
       u8 f;  // Flags
       u8 b, c;
       u8 d, e;
       u8 h, l;
   };

Register Pairs
~~~~~~~~~~~~~~

8-bit registers can be combined for 16-bit operations:

* **AF** - Accumulator & Flags
* **BC** - General purpose pair
* **DE** - General purpose pair
* **HL** - Often used for memory addressing

16-bit Registers
~~~~~~~~~~~~~~~~

* **PC** - Program Counter (0x0100 after boot)
* **SP** - Stack Pointer (0xFFFE after boot)

Flags Register
--------------

The F register contains 4 flags (bits 7-4):

.. list-table::
   :header-rows: 1
   :widths: 10 10 80

   * - Bit
     - Flag
     - Description
   * - 7
     - Z
     - **Zero** - Set if result is zero
   * - 6
     - N
     - **Subtract** - Set if last operation was subtraction
   * - 5
     - H
     - **Half Carry** - Set if carry from bit 3 to bit 4
   * - 4
     - C
     - **Carry** - Set if carry from bit 7

Bits 0-3 are always zero.

Instruction Set
---------------

The CPU implements 256 base opcodes plus 256 CB-prefixed opcodes.

Instruction Categories
~~~~~~~~~~~~~~~~~~~~~~

Load Instructions
^^^^^^^^^^^^^^^^^

**8-bit Loads**

* ``LD r,r'`` - Load register to register (49 variants)
* ``LD r,n`` - Load immediate to register (7 variants)
* ``LD r,(HL)`` - Load from memory via HL (7 variants)
* ``LD (HL),r`` - Store to memory via HL (7 variants)
* ``LD A,(BC/DE)`` - Load from memory via BC/DE
* ``LD (BC/DE),A`` - Store to memory via BC/DE
* ``LDH A,(n)`` / ``LDH (n),A`` - High memory load/store

**16-bit Loads**

* ``LD rr,nn`` - Load 16-bit immediate
* ``PUSH rr`` - Push to stack
* ``POP rr`` - Pop from stack
* ``LD SP,HL`` - Load HL into SP

ALU Instructions
^^^^^^^^^^^^^^^^

**Arithmetic**

* ``ADD A,r`` - Add register to A
* ``ADC A,r`` - Add with carry
* ``SUB r`` - Subtract from A
* ``SBC A,r`` - Subtract with carry
* ``INC r`` - Increment 8-bit
* ``DEC r`` - Decrement 8-bit

**Logic**

* ``AND r`` - Bitwise AND
* ``OR r`` - Bitwise OR
* ``XOR r`` - Bitwise XOR
* ``CP r`` - Compare (subtract without storing)

**16-bit Arithmetic**

* ``ADD HL,rr`` - Add 16-bit register pair to HL
* ``INC rr`` - Increment 16-bit
* ``DEC rr`` - Decrement 16-bit
* ``ADD SP,e`` - Add signed immediate to SP

Control Flow
^^^^^^^^^^^^

**Jumps**

* ``JP nn`` - Jump to address
* ``JP cc,nn`` - Conditional jump (NZ, Z, NC, C)
* ``JP (HL)`` - Jump to address in HL
* ``JR e`` - Relative jump
* ``JR cc,e`` - Conditional relative jump

**Calls & Returns**

* ``CALL nn`` - Call subroutine
* ``CALL cc,nn`` - Conditional call
* ``RET`` - Return from subroutine
* ``RET cc`` - Conditional return
* ``RETI`` - Return and enable interrupts
* ``RST n`` - Restart (call to fixed address)

Bit Operations (CB Prefix)
^^^^^^^^^^^^^^^^^^^^^^^^^^^

* ``BIT n,r`` - Test bit n in register
* ``SET n,r`` - Set bit n in register
* ``RES n,r`` - Reset (clear) bit n in register
* ``RL r`` - Rotate left through carry
* ``RLC r`` - Rotate left
* ``RR r`` - Rotate right through carry
* ``RRC r`` - Rotate right
* ``SLA r`` - Shift left arithmetic
* ``SRA r`` - Shift right arithmetic
* ``SRL r`` - Shift right logical
* ``SWAP r`` - Swap nibbles

Instruction Timing
------------------

Instructions take 1-6 M-cycles to execute.

.. list-table::
   :header-rows: 1
   :widths: 40 20 40

   * - Instruction
     - M-cycles
     - Notes
   * - ``NOP``
     - 1
     - No operation
   * - ``LD r,r'``
     - 1
     - Register to register
   * - ``LD r,n``
     - 2
     - Fetch immediate byte
   * - ``LD r,(HL)``
     - 2
     - Memory read
   * - ``CALL nn``
     - 6
     - Push PC and jump
   * - ``RET``
     - 4
     - Pop PC and return

Implementation
--------------

CPU Class
~~~~~~~~~

.. code-block:: cpp

   class CPU {
   public:
       explicit CPU(Memory& memory);
       
       Cycles step();              // Execute one instruction
       void handle_interrupts();   // Check and service interrupts
       void reset();              // Reset to initial state
       
       const Registers& registers() const;
       bool ime() const;          // Interrupt Master Enable
       bool is_halted() const;
       
   private:
       Registers regs_;
       Memory& memory_;
       bool ime_;
       bool halted_;
       
       Cycles execute_instruction(u8 opcode);
       Cycles execute_cb_instruction(u8 opcode);
       
       // Helper methods
       u8 fetch_byte();
       u16 fetch_word();
       void push_stack(u16 value);
       u16 pop_stack();
       
       // ALU operations
       void alu_add(u8 value, bool use_carry = false);
       void alu_sub(u8 value, bool use_carry = false);
       void alu_and(u8 value);
       void alu_or(u8 value);
       void alu_xor(u8 value);
       void alu_cp(u8 value);
       void alu_inc(u8& reg);
       void alu_dec(u8& reg);
   };

Instruction Dispatch
~~~~~~~~~~~~~~~~~~~~

Instructions are dispatched via switch statement:

.. code-block:: cpp

   Cycles CPU::execute_instruction(u8 opcode) {
       switch (opcode) {
           case 0x00: return 1;  // NOP
           case 0x3E: regs_.a = fetch_byte(); return 2;  // LD A,n
           case 0xC3: regs_.pc = fetch_word(); return 4;  // JP nn
           // ... 180+ more cases
       }
   }

ALU Operations
~~~~~~~~~~~~~~

ALU operations properly set flags:

.. code-block:: cpp

   void CPU::alu_add(u8 value, bool use_carry) {
       u16 result = regs_.a + value;
       if (use_carry && regs_.get_flag(FLAG_C)) {
           result++;
       }
       
       regs_.set_flag(FLAG_Z, (result & 0xFF) == 0);
       regs_.set_flag(FLAG_N, false);
       regs_.set_flag(FLAG_H, ((regs_.a & 0x0F) + (value & 0x0F)) > 0x0F);
       regs_.set_flag(FLAG_C, result > 0xFF);
       
       regs_.a = result & 0xFF;
   }

Testing
-------

CPU tests verify:

* Register operations and flag behavior
* Memory access patterns
* Instruction timing accuracy
* Stack operations
* Jump/call logic

Example test:

.. code-block:: cpp

   void test_cpu_add() {
       CPU cpu(memory);
       cpu.registers().a = 0x42;
       cpu.registers().b = 0x10;
       
       // Execute ADD A,B
       cpu.step();
       
       assert(cpu.registers().a == 0x52);
       assert(!cpu.registers().get_flag(FLAG_Z));
       assert(!cpu.registers().get_flag(FLAG_C));
   }

