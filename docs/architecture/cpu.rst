CPU Architecture
================

Overview
--------

The Sharp LR35902 is a custom 8-bit processor designed specifically for the Game Boy. It combines elements from the Intel 8080 and Zilog Z80 architectures [PanDocs_CPU]_.

Historical Context
~~~~~~~~~~~~~~~~~~

The LR35902 was designed by Sharp Corporation for Nintendo's Game Boy handheld console. While its exact design rationale has not been officially documented by Sharp or Nintendo, the architecture clearly shows influences from both the Intel 8080 and Zilog Z80:

* **Register set**: Similar to the Z80 (A, B, C, D, E, H, L, F registers)
* **Instruction encoding**: More closely resembles the 8080
* **Removed features**: Lacks Z80's shadow registers, index registers (IX/IY), and some extended instructions [PanDocs_CPU]_

The resulting processor was well-suited for a battery-powered gaming device, balancing capability with power efficiency.

Technical Specifications
~~~~~~~~~~~~~~~~~~~~~~~~

* **Architecture**: 8-bit data bus, 16-bit address bus
* **Clock Speed**: 4.194304 MHz (exactly 2²² Hz)
* **Address Space**: 64KB (0x0000-0xFFFF)
* **Instruction Set**: ~240 unique instructions
* **Execution Model**: Variable-length instructions (1-3 bytes), variable-cycle execution (1-6 machine cycles)

The clock speed wasn't arbitrary—it's derived from the crystal oscillator frequency needed for the PPU's timing requirements, ensuring perfect synchronization between CPU and graphics operations.

Register Architecture
---------------------

Understanding the CPU's registers is fundamental to understanding how Game Boy programs work. Registers are small, extremely fast storage locations built directly into the CPU—think of them as the CPU's "working memory."

8-bit General Purpose Registers
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The LR35902 has six 8-bit general-purpose registers: **B**, **C**, **D**, **E**, **H**, and **L**. While called "general purpose," each has conventional uses that Game Boy programmers follow:

+------------------------+-----------------------------------------------------------------------------------------------+
| Register               | Purpose                                                                                       |
+========================+===============================================================================================+
| **B & C Registers**    | Often used as a counter pair. **B** commonly holds loop counters, while **C** is frequently   |
|                        | used for I/O port addresses (especially in the high memory region 0xFF00-0xFFFF).             |
+------------------------+-----------------------------------------------------------------------------------------------+
| **D & E Registers**    | General-purpose data storage and temporary calculations. Often used for 16-bit source         |
|                        | addresses in memory copy operations.                                                          |
+------------------------+-----------------------------------------------------------------------------------------------+
| **H & L Registers**    | **HL** (the **H**\ igh/**L**\ ow pair) is the primary memory addressing register. Most        |
|                        | instructions that access memory use HL to point to data. This is the most heavily used        |
|                        | register pair.                                                                                |
+------------------------+-----------------------------------------------------------------------------------------------+

The Accumulator (A Register)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The **A** register (accumulator) is special—it's the primary register for arithmetic and logical operations. Almost all ALU (Arithmetic Logic Unit) operations involve the A register:

* Addition: always adds TO the A register
* Subtraction: always subtracts FROM the A register  
* Logical operations (AND, OR, XOR): always operate ON the A register
* Comparisons: compare other values against A

This design follows the 8080 architecture philosophy: centralizing operations around one register makes the CPU simpler and faster, at the cost of requiring more data movement between registers.

Register Pairing
~~~~~~~~~~~~~~~~

The 8-bit registers can be combined into three 16-bit register pairs: **BC**, **DE**, and **HL**. This pairing is not just conceptual—the CPU has actual hardware instructions that treat these pairs as single 16-bit registers.

**Why pairing matters:**

1. **Memory addressing**: The Game Boy has 64KB of address space, requiring 16-bit addresses. Register pairs let you store and manipulate these addresses.

2. **16-bit arithmetic**: Games frequently need to work with values larger than 255. Register pairs enable 16-bit addition, increment, and decrement operations.

3. **Efficient data movement**: You can load or store 16 bits of data with a single instruction using register pairs.

The **HL** pair is particularly important because it's the only register pair that can be used for certain memory addressing modes, making it the "pointer register" of the Game Boy.

Special Purpose Registers
~~~~~~~~~~~~~~~~~~~~~~~~~~

Beyond the general-purpose registers, the CPU has specialized registers for program flow and stack management:

+------------------------+-----------------------------------------------------------------------------------------------+
| Register               | Purpose                                                                                       |
+========================+===============================================================================================+
| **PC** (Program        | Holds the memory address of the next instruction to execute. After the Game Boy boot ROM      |
| Counter)               | finishes, PC starts at 0x0100 (the cartridge entry point). The PC automatically increments    |
|                        | as instructions are fetched and decoded.                                                      |
+------------------------+-----------------------------------------------------------------------------------------------+
| **SP** (Stack Pointer) | Points to the current top of the stack in memory. Initialized to 0xFFFE (the top of usable    |
|                        | RAM). The stack grows downward (toward lower addresses) as data is pushed onto it.            |
+------------------------+-----------------------------------------------------------------------------------------------+
| **F** (Flags)          | Contains condition flags set by operations. These flags control conditional jumps and affect  |
|                        | certain instructions. See the Flags Register section below.                                   |
+------------------------+-----------------------------------------------------------------------------------------------+

Flags Register (F)
~~~~~~~~~~~~~~~~~~

The **F** register (flags register) contains four condition flags in its upper nibble (bits 7-4). The lower nibble (bits 3-0) is always zero and cannot be written. These flags are the CPU's way of "remembering" information about the last operation performed.

**Flag Descriptions**

.. list-table::
   :widths: 10 15 75
   :header-rows: 1

   * - Bit
     - Flag
     - Purpose & Behavior
   * - 7
     - **Z** (Zero)
     - Set (1) when an arithmetic or logical operation produces a zero result. Used extensively for checking equality: ``CP`` (compare) subtracts without storing the result—if values are equal, the result is zero and Z is set.
   * - 6
     - **N** (Subtract)
     - Set (1) if the last operation was a subtraction, cleared (0) if it was addition. Used internally by the DAA instruction for BCD (Binary-Coded Decimal) adjustment. Modern games rarely use this directly, but it's essential for the CPU's arithmetic correctness.
   * - 5
     - **H** (Half Carry)
     - Set (1) when an operation causes a carry from bit 3 to bit 4 (carry out of the lower nibble). Also used by DAA for BCD arithmetic. In 8-bit addition, this detects when the lower 4 bits overflow.
   * - 4
     - **C** (Carry)
     - Set (1) when an arithmetic operation overflows (addition exceeds 255, or subtraction borrows). Also set/cleared by shift and rotate instructions, and can be explicitly set/cleared by SCF/CCF instructions. Used for multi-byte arithmetic and conditional logic.

**Understanding Flags Through Examples**

*Example 1: Zero Flag in Comparisons*

When you want to check if two values are equal:

::

   A register contains: 0x42
   B register contains: 0x42
   Execute: CP B  (compare A with B, which does A - B without storing)
   Result: 0x42 - 0x42 = 0x00
   Flags: Z=1 (zero), N=1 (subtract), H and C depend on the values

Programs then use conditional jumps: ``JP Z, address`` (jump if zero flag is set) to branch when values are equal.

*Example 2: Carry Flag in Addition*

When adding two 8-bit numbers that exceed 255:

::

   A register contains: 0xFF (255 decimal)
   Add immediate value: 0x02
   Execute: ADD A, 0x02
   Result: 0xFF + 0x02 = 0x0101 (257 decimal)
   A register receives: 0x01 (only bottom 8 bits)
   Flags: Z=0 (not zero), N=0 (addition), H=1 (carry from bit 3), C=1 (overflow)

The carry flag tells you the addition overflowed. For 16-bit addition, you'd add the low bytes, then add the high bytes WITH carry using ADC.

*Example 3: Half Carry Flag*

The half carry flag might seem obscure, but it's crucial for BCD arithmetic (used in some older games for decimal score display):

::

   A register contains: 0x0F (15 decimal, 0b00001111)
   Add immediate value: 0x01
   Execute: ADD A, 0x01
   Result: 0x0F + 0x01 = 0x10
   Flags: Z=0, N=0, H=1 (carry from bit 3 to bit 4), C=0 (no overflow from bit 7)

The half carry occurred because the lower 4 bits (0xF + 0x1 = 0x10) caused a carry into the upper nibble.

The Stack
~~~~~~~~~

The stack is a "last-in-first-out" (LIFO) data structure in memory, managed by the stack pointer (SP). Think of it like a stack of plates—you can only add or remove from the top.

**Why does the stack grow downward?**

This is a convention from early computer design: 

* Program code and data start at low memory addresses (growing upward)
* The stack starts at high memory addresses (growing downward)
* This maximizes the space available to both before they collide

The Game Boy follows this pattern: SP starts at 0xFFFE, and each PUSH operation decrements SP and writes data to memory. POP operations read data and increment SP.

**The stack is used for:**

1. **Function calls**: When you CALL a subroutine, the CPU pushes the return address onto the stack
2. **Preserving registers**: Functions push registers to save their values, then pop them before returning
3. **Local variables**: Some compilers allocate temporary variables on the stack
4. **Interrupt handling**: When an interrupt occurs, the CPU automatically pushes PC onto the stack

Execution Cycle
---------------

The CPU operates in a continuous loop called the fetch-decode-execute cycle. Understanding this cycle is essential to understanding how programs run.

The Fetch-Decode-Execute Cycle
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**1. Fetch**

The CPU reads the byte at the address stored in the Program Counter (PC):

* CPU puts PC value on the address bus
* Memory responds with the byte at that address
* CPU stores this byte as the current instruction opcode
* PC increments to point to the next byte

This takes 1 machine cycle (4 clock cycles).

**2. Decode**

The CPU interprets the opcode to determine:

* What operation to perform
* What registers or memory locations are involved
* If additional bytes are needed (operands)
* How many machine cycles the instruction will take

Some instructions need additional bytes (immediate values, addresses). The CPU fetches these in additional cycles before execution.

**3. Execute**

The CPU performs the operation specified by the instruction:

* Arithmetic/logic operations are performed by the ALU
* Register-to-register transfers happen via internal buses
* Memory reads/writes go through the memory interface
* Program counter might be modified (jumps, calls, returns)

**4. Repeat**

The cycle immediately continues with the next fetch. This happens continuously at 4.194304 MHz—roughly 4.2 million times per second—as long as the CPU is not halted.

Machine Cycles and Timing
~~~~~~~~~~~~~~~~~~~~~~~~~~

The Game Boy CPU's timing is based on **machine cycles** (M-cycles), not individual clock cycles:

* 1 M-cycle = 4 clock cycles
* Instructions take 1-6 M-cycles to complete
* Simple operations (register-to-register) take 1 M-cycle
* Memory accesses add M-cycles (each read or write takes 1 M-cycle)
* Complex operations (CALL, RET) take more M-cycles due to multiple memory accesses

**Why this matters:**

Every component in the Game Boy is synchronized to this timing. The PPU (graphics processor) operates in lock-step with the CPU. When your program takes too long to complete during VBlank, it causes graphical glitches. Understanding instruction timing helps optimize game performance.

The Arithmetic Logic Unit (ALU)
--------------------------------

The ALU is the CPU's calculation engine—it performs all arithmetic and logical operations. Understanding how the ALU works and interacts with flags helps you understand why Game Boy code looks the way it does.

ALU Operations
~~~~~~~~~~~~~~

The ALU supports:

* **Arithmetic**: ADD, ADC (add with carry), SUB, SBC (subtract with carry), INC, DEC, CP (compare)
* **Logical**: AND, OR, XOR
* **Shifts/Rotates**: RLC, RRC, RL, RR, SLA, SRA, SRL, SWAP
* **Bit Operations**: BIT (test bit), SET (set bit), RES (reset bit)

All arithmetic and logical operations involving 8-bit values operate on the **A register** and update the flags register accordingly.

Flag Behavior in Operations
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Every ALU operation has specific rules for how it sets flags. Understanding these rules is essential for writing correct conditional logic.

**Addition (ADD, ADC)**

::

   Z: Set if result is zero
   N: Cleared (this was addition, not subtraction)
   H: Set if carry from bit 3 to bit 4
   C: Set if result exceeds 255 (carry from bit 7)

**Subtraction (SUB, SBC, CP)**

::

   Z: Set if result is zero
   N: Set (this was subtraction)
   H: Set if borrow from bit 4
   C: Set if result is negative (borrow occurred)

**Logical Operations (AND, OR, XOR)**

::

   Z: Set if result is zero
   N: Cleared
   H: Set for AND, cleared for OR and XOR (following Z80 behavior)
   C: Cleared

**Increment/Decrement (INC, DEC)**

::

   Z: Set if result is zero
   N: Set for DEC, cleared for INC
   H: Set according to half-carry
   C: Not affected (preserved from previous operation)

Note that INC and DEC don't affect the carry flag—this allows multi-byte increments without disrupting carry.

16-bit Operations
~~~~~~~~~~~~~~~~~

The CPU can perform some 16-bit operations on register pairs:

* **ADD HL, rr**: Add a 16-bit register pair to HL (only affects N, H, C flags—not Z)
* **INC/DEC rr**: Increment/decrement 16-bit register pairs (no flags affected!)
* **ADD SP, e**: Add signed 8-bit value to stack pointer (special flag behavior)

The lack of flag updates for 16-bit INC/DEC is significant—you can't easily check if a 16-bit counter reached zero using just INC and conditional jumps.

DAA - Decimal Adjust Accumulator
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The DAA instruction is a relic from the Z80's heritage, designed for **Binary-Coded Decimal (BCD)** arithmetic, where each byte represents two decimal digits (0-99).

**What is BCD?**

In BCD format, each nibble (4 bits) represents a single decimal digit (0-9):

::

   Binary: 0011 0111 = 0x37 = 55 decimal (in normal binary)
   BCD:    0011 0111 = "37" (digit 3 and digit 7)

BCD was popular in early computing because it made decimal I/O easier—no binary-to-decimal conversion needed for displays.

**The Problem DAA Solves**

When you add two BCD numbers using normal binary addition, the result can be invalid BCD:

::

   Example: Add 0x29 (BCD "29") + 0x18 (BCD "18")
   Binary addition: 0x29 + 0x18 = 0x41 (invalid BCD!)
   Expected BCD: Should be 0x47 (BCD "47")

The issue: When a nibble exceeds 9, we need to adjust by adding 6 to "skip" the invalid values (A-F).

**How DAA Works**

DAA examines the accumulator and adjusts it based on the N (subtract) and H (half-carry) flags:

.. list-table::
   :widths: 20 20 60
   :header-rows: 1

   * - Condition
     - Flag State
     - Adjustment
   * - After Addition
     - N = 0
     - • If lower nibble > 9 OR H = 1: Add 0x06
       
       • If upper nibble > 9 OR C = 1: Add 0x60
   * - After Subtraction
     - N = 1
     - • If H = 1: Subtract 0x06
       
       • If C = 1: Subtract 0x60

The H flag indicates a carry from bit 3, signaling the lower nibble needs adjustment. The C flag indicates the upper nibble needs adjustment.

**Complete BCD Addition Example**

::

   ; Add BCD "58" + BCD "37" = BCD "95"
   
   LD A, 0x58        ; Load BCD "58"
   ADD A, 0x37       ; Binary add: 0x58 + 0x37 = 0x8F
   ; After ADD: A = 0x8F, H = 0, C = 0 (invalid BCD!)
   
   DAA               ; Decimal adjust
   ; DAA sees lower nibble = 0xF (> 9), adds 0x06
   ; Result: 0x8F + 0x06 = 0x95 (valid BCD "95")
   ; Final: A = 0x95, Z = 0, H = 0, C = 0

**BCD Subtraction Example**

::

   ; Subtract BCD "45" - BCD "28" = BCD "17"
   
   LD A, 0x45        ; Load BCD "45"
   SUB 0x28          ; Binary sub: 0x45 - 0x28 = 0x1D
   ; After SUB: A = 0x1D, N = 1, H = 1 (borrow from bit 4)
   
   DAA               ; Decimal adjust for subtraction
   ; DAA sees N = 1 and H = 1, subtracts 0x06
   ; Result: 0x1D - 0x06 = 0x17 (valid BCD "17")

**Usage in Games**

Most modern Game Boy games don't use BCD arithmetic, preferring to store scores as binary and convert for display. However, some older titles (particularly early Game Boy games from developers familiar with Z80) used BCD for score counters and timers, making DAA necessary for emulation accuracy.

**Implementation Note**

DAA is one of the trickiest instructions to implement correctly in an emulator. The adjustment logic depends on previous operation flags, and edge cases (like 0x99 + 0x01 = 0x00 with C = 1) must be handled precisely to match hardware behavior.

References
----------

.. [PanDocs_CPU] Pan Docs - CPU Registers and Flags. https://gbdev.io/pandocs/CPU_Registers_and_Flags.html
.. [GBCPUman] Game Boy CPU Manual. http://marc.rawer.de/Gameboy/Docs/GBCPUman.pdf
.. [TCAGBD] The Cycle-Accurate Game Boy Docs. https://github.com/AntonioND/giibiiadvance/blob/master/docs/TCAGBD.pdf
.. [Opcodes] GB Opcodes Reference. https://gbdev.io/gb-opcodes/optables/

.. important::
   **Documentation Disclaimer**
   
   The exact design rationale and business decisions behind the LR35902 have not been officially documented by Sharp or Nintendo. The technical characteristics described in this document are based on hardware analysis and behavior documented by the Game Boy development community over decades of reverse engineering and testing.