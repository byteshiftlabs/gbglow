# CPU Documentation

## Sharp LR35902 CPU

The Game Boy uses a custom Sharp LR35902 CPU, which is similar to the Intel 8080 and Zilog Z80, but with some differences.

### Clock Speed
- **DMG (Original Game Boy)**: 4.194304 MHz
- **CGB (Game Boy Color)**: 4.194304 MHz or 8.388608 MHz (double-speed mode)

### Registers

#### 8-bit Registers
- **A**: Accumulator - primary register for arithmetic operations
- **F**: Flags register - contains condition codes
  - Bit 7: Zero (Z) - Set if result is zero
  - Bit 6: Subtract (N) - Set if last operation was subtraction
  - Bit 5: Half Carry (H) - Set if carry from bit 3 to bit 4
  - Bit 4: Carry (C) - Set if carry from bit 7
  - Bits 0-3: Always 0
- **B, C, D, E, H, L**: General purpose registers

#### 16-bit Registers
- **SP**: Stack Pointer (0xFFFE after boot)
- **PC**: Program Counter (0x0100 after boot)

#### Register Pairs
8-bit registers can be combined for 16-bit operations:
- **AF**: Accumulator + Flags
- **BC**: B + C
- **DE**: D + E
- **HL**: H + L (often used for memory addressing)

### Instruction Set

The CPU has 256 base instructions (0x00-0xFF) plus 256 CB-prefixed instructions for bit operations.

#### Instruction Categories
1. **8-bit loads**: LD r, r' / LD r, n / LD r, (HL)
2. **16-bit loads**: LD rr, nn / PUSH rr / POP rr
3. **8-bit ALU**: ADD / SUB / AND / OR / XOR / CP / INC / DEC
4. **16-bit ALU**: ADD HL, rr / INC rr / DEC rr
5. **Rotates & Shifts**: RLCA / RLA / RRCA / RRA / RLC / RL / RRC / RR / SLA / SRA / SRL
6. **Bit Operations**: BIT / SET / RES (CB-prefixed)
7. **Jumps**: JP / JR / CALL / RET / RST
8. **Control**: NOP / HALT / STOP / DI / EI

### Timing

Instructions take between 1-6 M-cycles:
- 1 M-cycle = 4 clock cycles (T-states)
- Example timings:
  - NOP: 1 M-cycle (4 T-states)
  - LD A, B: 1 M-cycle (4 T-states)
  - LD A, (BC): 2 M-cycles (8 T-states)
  - CALL nn: 6 M-cycles (24 T-states)

### Interrupts

Five interrupt sources:
1. **VBlank** (0x40): Triggered at start of VBlank period (~59.7 Hz)
2. **LCD STAT** (0x48): Various LCD controller conditions
3. **Timer** (0x50): Timer overflow
4. **Serial** (0x58): Serial transfer complete
5. **Joypad** (0x60): Button press

Interrupt handling:
- IME (Interrupt Master Enable) flag must be set
- Interrupt Enable (IE) register at 0xFFFF
- Interrupt Flag (IF) register at 0xFF0F
- When serviced: IME cleared, PC pushed to stack, jump to vector

### References
- [Pan Docs - CPU Registers](https://gbdev.io/pandocs/CPU_Registers_and_Flags.html)
- [Game Boy CPU Manual](http://marc.rawer.de/Gameboy/Docs/GBCPUman.pdf)
- [Opcode Table](https://www.pastraiser.com/cpu/gameboy/gameboy_opcodes.html)
