// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 gbglow Contributors
// This file is part of gbglow. See LICENSE for details.

#pragma once

#include "types.h"

namespace gbglow {

/**
 * I/O Register Addresses
 *
 * Hardware register addresses for the Game Boy and Game Boy Color.
 * Shared across PPU, APU, debugger, and other subsystems.
 *
 * Reference: Pan Docs — https://gbdev.io/pandocs/
 */
namespace io_reg {

// --- Memory map boundaries ---
constexpr u16 VRAM_START = 0x8000;
constexpr u16 EXTERNAL_RAM_START = 0xA000;
constexpr u16 WRAM_START = 0xC000;
constexpr u16 WRAM_END = 0xE000;
constexpr u16 OAM_START = 0xFE00;
constexpr u16 IO_START = 0xFF00;
constexpr u16 HRAM_START = 0xFF80;

// --- LCD registers ---
constexpr u16 REG_LCDC = 0xFF40;  // LCD Control
constexpr u16 REG_STAT = 0xFF41;  // LCD Status
constexpr u16 REG_SCY  = 0xFF42;  // Scroll Y
constexpr u16 REG_SCX  = 0xFF43;  // Scroll X
constexpr u16 REG_LY   = 0xFF44;  // LCD Y coordinate
constexpr u16 REG_LYC  = 0xFF45;  // LY Compare
constexpr u16 REG_DMA  = 0xFF46;  // OAM DMA transfer
constexpr u16 REG_BGP  = 0xFF47;  // Background palette (DMG)
constexpr u16 REG_OBP0 = 0xFF48;  // Object palette 0 (DMG)
constexpr u16 REG_OBP1 = 0xFF49;  // Object palette 1 (DMG)
constexpr u16 REG_WY   = 0xFF4A;  // Window Y position
constexpr u16 REG_WX   = 0xFF4B;  // Window X position

// --- Sound registers ---
constexpr u16 REG_NR10 = 0xFF10;  // Channel 1 sweep
constexpr u16 REG_NR11 = 0xFF11;  // Channel 1 length/duty
constexpr u16 REG_NR12 = 0xFF12;  // Channel 1 envelope
constexpr u16 REG_NR13 = 0xFF13;  // Channel 1 frequency lo
constexpr u16 REG_NR14 = 0xFF14;  // Channel 1 frequency hi
constexpr u16 REG_NR21 = 0xFF16;  // Channel 2 length/duty
constexpr u16 REG_NR22 = 0xFF17;  // Channel 2 envelope
constexpr u16 REG_NR23 = 0xFF18;  // Channel 2 frequency lo
constexpr u16 REG_NR24 = 0xFF19;  // Channel 2 frequency hi
constexpr u16 REG_NR30 = 0xFF1A;  // Channel 3 on/off
constexpr u16 REG_NR31 = 0xFF1B;  // Channel 3 length
constexpr u16 REG_NR32 = 0xFF1C;  // Channel 3 output level
constexpr u16 REG_NR33 = 0xFF1D;  // Channel 3 frequency lo
constexpr u16 REG_NR34 = 0xFF1E;  // Channel 3 frequency hi
constexpr u16 REG_NR41 = 0xFF20;  // Channel 4 length
constexpr u16 REG_NR42 = 0xFF21;  // Channel 4 envelope
constexpr u16 REG_NR43 = 0xFF22;  // Channel 4 polynomial
constexpr u16 REG_NR44 = 0xFF23;  // Channel 4 control
constexpr u16 REG_NR50 = 0xFF24;  // Master volume
constexpr u16 REG_NR51 = 0xFF25;  // Sound panning
constexpr u16 REG_NR52 = 0xFF26;  // Sound on/off
constexpr u16 WAVE_RAM_START = 0xFF30;
constexpr u16 WAVE_RAM_END   = 0xFF3F;

// --- Timer registers ---
constexpr u16 REG_DIV  = 0xFF04;  // Divider
constexpr u16 REG_TIMA = 0xFF05;  // Timer counter
constexpr u16 REG_TMA  = 0xFF06;  // Timer modulo
constexpr u16 REG_TAC  = 0xFF07;  // Timer control

// --- Interrupt registers ---
constexpr u16 REG_IF = 0xFF0F;    // Interrupt flags
constexpr u16 REG_IE = 0xFFFF;    // Interrupt enable

// --- Misc registers ---
constexpr u16 REG_JOYP = 0xFF00;  // Joypad
constexpr u16 REG_SB   = 0xFF01;  // Serial data
constexpr u16 REG_SC   = 0xFF02;  // Serial control

// --- CGB registers ---
constexpr u16 REG_KEY1 = 0xFF4D;  // Speed switch
constexpr u16 REG_VBK  = 0xFF4F;  // VRAM bank select
constexpr u16 REG_BCPS = 0xFF68;  // BG palette specification
constexpr u16 REG_BCPD = 0xFF69;  // BG palette data
constexpr u16 REG_OCPS = 0xFF6A;  // OBJ palette specification
constexpr u16 REG_OCPD = 0xFF6B;  // OBJ palette data

} // namespace io_reg
} // namespace gbglow
