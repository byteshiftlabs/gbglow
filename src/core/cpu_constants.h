#pragma once

#include "types.h"

namespace gbglow {

// ============================================================================
// CPU Instruction Cycle Counts (in M-cycles, where 1 M-cycle = 4 T-states)
// ============================================================================

constexpr Cycles CYCLES_REGISTER_OP = 1;        // Register-to-register operations
constexpr Cycles CYCLES_IMMEDIATE_BYTE = 2;     // Immediate byte load/ALU
constexpr Cycles CYCLES_IMMEDIATE_WORD = 3;     // Immediate word load
constexpr Cycles CYCLES_MEMORY_READ = 2;        // Memory read operations
constexpr Cycles CYCLES_MEMORY_WRITE = 2;       // Memory write operations
constexpr Cycles CYCLES_PUSH = 4;               // PUSH operations
constexpr Cycles CYCLES_POP = 3;                // POP operations
constexpr Cycles CYCLES_CALL = 6;               // CALL instruction
constexpr Cycles CYCLES_RET = 4;                // RET instruction
constexpr Cycles CYCLES_RET_CONDITIONAL = 5;    // RET with condition (when taken)
constexpr Cycles CYCLES_JP_ABSOLUTE = 4;        // JP nn instruction
constexpr Cycles CYCLES_JR_RELATIVE = 3;        // JR n instruction (when taken)
constexpr Cycles CYCLES_RST = 4;                // RST instruction
constexpr Cycles CYCLES_MEMORY_HL_INC_DEC = 3;  // INC/DEC (HL)
constexpr Cycles CYCLES_LD_NN_SP = 5;           // LD (nn),SP
constexpr Cycles CYCLES_ADD_SP_N = 4;           // ADD SP,n

// ============================================================================
// Bit Manipulation Constants
// ============================================================================

constexpr u8 BIT_0 = 0x01;
constexpr u8 BIT_7 = 0x80;
constexpr u8 NIBBLE_MASK = 0x0F;
constexpr u8 BYTE_MASK = 0xFF;
constexpr u16 ADDR_MASK_12BIT = 0x0FFF;
constexpr u16 ADDR_MASK_16BIT = 0xFFFF;

constexpr u8 BIT_POSITION_SHIFT = 3;           // Shift to extract bit position from CB opcode
constexpr u8 REGISTER_INDEX_MASK = 0x07;       // Mask to extract register index

constexpr u8 NIBBLE_SHIFT = 4;                  // Bits to shift for nibble swap
constexpr u8 BIT_SHIFT = 1;                     // Single bit shift amount

constexpr u8 MEMORY_HL_REGISTER_INDEX = 6;      // Register index value indicating (HL) memory access

// ============================================================================
// Memory Map Constants
// ============================================================================

constexpr u16 IO_REGISTERS_BASE = 0xFF00;       // Base address for I/O registers (high RAM page)

// ============================================================================
// Interrupt System Constants
// ============================================================================

// Interrupt vector addresses - where PC jumps when interrupt is serviced
constexpr u16 INT_VBLANK_VECTOR = 0x0040;       // VBlank interrupt vector
constexpr u16 INT_LCD_STAT_VECTOR = 0x0048;     // LCD STAT interrupt vector
constexpr u16 INT_TIMER_VECTOR = 0x0050;        // Timer interrupt vector
constexpr u16 INT_SERIAL_VECTOR = 0x0058;       // Serial interrupt vector
constexpr u16 INT_JOYPAD_VECTOR = 0x0060;       // Joypad interrupt vector

// Interrupt register addresses
constexpr u16 REG_INTERRUPT_FLAG = 0xFF0F;      // IF register - Interrupt Flag (pending interrupts)
constexpr u16 REG_INTERRUPT_ENABLE = 0xFFFF;    // IE register - Interrupt Enable (which interrupts are enabled)

// Interrupt bit masks in IF/IE registers
constexpr u8 INT_VBLANK_BIT = 0x01;             // Bit 0: VBlank interrupt
constexpr u8 INT_LCD_STAT_BIT = 0x02;           // Bit 1: LCD STAT interrupt
constexpr u8 INT_TIMER_BIT = 0x04;              // Bit 2: Timer interrupt
constexpr u8 INT_SERIAL_BIT = 0x08;             // Bit 3: Serial interrupt
constexpr u8 INT_JOYPAD_BIT = 0x10;             // Bit 4: Joypad interrupt

// Interrupt handling timing
constexpr Cycles CYCLES_INTERRUPT_DISPATCH = 5;  // M-cycles to dispatch an interrupt (push PC, jump to vector)

// ============================================================================
// RST Vector Addresses
// ============================================================================
// RST instructions jump to fixed addresses in the lower memory page
// These are commonly used for interrupt handlers and frequently-called routines

constexpr u16 RST_00 = 0x00;
constexpr u16 RST_08 = 0x08;
constexpr u16 RST_10 = 0x10;
constexpr u16 RST_18 = 0x18;
constexpr u16 RST_20 = 0x20;
constexpr u16 RST_28 = 0x28;
constexpr u16 RST_30 = 0x30;
constexpr u16 RST_38 = 0x38;

// ============================================================================
// DAA (Decimal Adjust Accumulator) Constants
// ============================================================================
// Used for BCD (Binary Coded Decimal) arithmetic

constexpr u8 BCD_CORRECTION_LOWER = 0x06;       // Correction for lower nibble in DAA
constexpr u8 BCD_CORRECTION_UPPER = 0x60;       // Correction for upper nibble in DAA
constexpr u8 BCD_DIGIT_MAX = 9;                 // Maximum BCD digit value
constexpr u8 BCD_BYTE_MAX = 0x99;               // Maximum BCD byte value

// ============================================================================
// HL Increment/Decrement Constants
// ============================================================================

constexpr u16 HL_INCREMENT = 1;
constexpr u16 HL_DECREMENT = 1;

} // namespace gbglow
