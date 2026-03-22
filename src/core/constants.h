// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 gbglow Contributors
// This file is part of gbglow. See LICENSE for details.

#pragma once

#include "types.h"

#include <cstddef>

namespace gbglow::constants {

namespace application {
extern const char kName[];
extern const char kConfigDirectoryName[];
extern const char kKeybindingsFileName[];
extern const char kGamepadConfigFileName[];
extern const char kRecentRomsFileName[];
extern const char kOpenRomDialogTitle[];
extern const char kScreenshotDirectoryName[];
}

namespace emulator {
inline constexpr double kFrameRateHz = 59.73;
inline constexpr double kFrameTimeMs = 1000.0 / kFrameRateHz;
inline constexpr Cycles kCyclesPerFrame = 70224;
inline constexpr unsigned int kAudioSampleRate = 44100;
inline constexpr unsigned int kAudioChannels = 2;
inline constexpr unsigned int kAudioBytesPerSample = 2;
inline constexpr unsigned int kTargetBufferedFrames = 2;
inline constexpr unsigned int kTargetAudioQueueBytes =
    kAudioSampleRate * kAudioChannels * kAudioBytesPerSample * kTargetBufferedFrames / 60;
inline constexpr unsigned int kMaxAudioQueueBytes = kTargetAudioQueueBytes * 2;
}

namespace savestate {
extern const char kHeader[];
extern const std::size_t kHeaderLength;
inline constexpr std::size_t kMaxBytes = 512 * 1024;
inline constexpr int kMinSlot = 0;
inline constexpr int kMaxSlot = 9;
inline constexpr int kSlotCount = 10;
inline constexpr int kHotkeySlotCount = 9;
}

namespace cpu {
inline constexpr Cycles kCyclesRegisterOp = 1;
inline constexpr Cycles kCyclesImmediateByte = 2;
inline constexpr Cycles kCyclesImmediateWord = 3;
inline constexpr Cycles kCyclesMemoryRead = 2;
inline constexpr Cycles kCyclesMemoryWrite = 2;
inline constexpr Cycles kCyclesPush = 4;
inline constexpr Cycles kCyclesPop = 3;
inline constexpr Cycles kCyclesCall = 6;
inline constexpr Cycles kCyclesRet = 4;
inline constexpr Cycles kCyclesRetConditional = 5;
inline constexpr Cycles kCyclesJpAbsolute = 4;
inline constexpr Cycles kCyclesJrRelative = 3;
inline constexpr Cycles kCyclesRst = 4;
inline constexpr Cycles kCyclesMemoryHlIncDec = 3;
inline constexpr Cycles kCyclesLdNnSp = 5;
inline constexpr Cycles kCyclesAddSpN = 4;

inline constexpr u8 kBit0 = 0x01;
inline constexpr u8 kBit7 = 0x80;
inline constexpr u8 kNibbleMask = 0x0F;
inline constexpr u8 kByteMask = 0xFF;
inline constexpr u16 kAddrMask12Bit = 0x0FFF;
inline constexpr u16 kAddrMask16Bit = 0xFFFF;
inline constexpr u8 kBitPositionShift = 3;
inline constexpr u8 kRegisterIndexMask = 0x07;
inline constexpr u8 kNibbleShift = 4;
inline constexpr u8 kBitShift = 1;
inline constexpr u8 kMemoryHlRegisterIndex = 6;

inline constexpr u16 kIoRegistersBase = 0xFF00;

inline constexpr u16 kInterruptVBlankVector = 0x0040;
inline constexpr u16 kInterruptLcdStatVector = 0x0048;
inline constexpr u16 kInterruptTimerVector = 0x0050;
inline constexpr u16 kInterruptSerialVector = 0x0058;
inline constexpr u16 kInterruptJoypadVector = 0x0060;
inline constexpr u16 kRegInterruptFlag = 0xFF0F;
inline constexpr u16 kRegInterruptEnable = 0xFFFF;
inline constexpr u8 kInterruptVBlankBit = 0x01;
inline constexpr u8 kInterruptLcdStatBit = 0x02;
inline constexpr u8 kInterruptTimerBit = 0x04;
inline constexpr u8 kInterruptSerialBit = 0x08;
inline constexpr u8 kInterruptJoypadBit = 0x10;
inline constexpr Cycles kCyclesInterruptDispatch = 5;

inline constexpr u16 kRst00 = 0x00;
inline constexpr u16 kRst08 = 0x08;
inline constexpr u16 kRst10 = 0x10;
inline constexpr u16 kRst18 = 0x18;
inline constexpr u16 kRst20 = 0x20;
inline constexpr u16 kRst28 = 0x28;
inline constexpr u16 kRst30 = 0x30;
inline constexpr u16 kRst38 = 0x38;

inline constexpr u8 kBcdCorrectionLower = 0x06;
inline constexpr u8 kBcdCorrectionUpper = 0x60;
inline constexpr u8 kBcdDigitMax = 9;
inline constexpr u8 kBcdByteMax = 0x99;

inline constexpr u16 kHlIncrement = 1;
inline constexpr u16 kHlDecrement = 1;

inline constexpr u16 kCgbKey1Register = 0xFF4D;
inline constexpr u8 kCgbSpeedPrepareBit = 0x01;
inline constexpr u8 kCgbSpeedToggleBit = 0x80;

inline constexpr Cycles CYCLES_REGISTER_OP = kCyclesRegisterOp;
inline constexpr Cycles CYCLES_IMMEDIATE_BYTE = kCyclesImmediateByte;
inline constexpr Cycles CYCLES_IMMEDIATE_WORD = kCyclesImmediateWord;
inline constexpr Cycles CYCLES_MEMORY_READ = kCyclesMemoryRead;
inline constexpr Cycles CYCLES_MEMORY_WRITE = kCyclesMemoryWrite;
inline constexpr Cycles CYCLES_PUSH = kCyclesPush;
inline constexpr Cycles CYCLES_POP = kCyclesPop;
inline constexpr Cycles CYCLES_CALL = kCyclesCall;
inline constexpr Cycles CYCLES_RET = kCyclesRet;
inline constexpr Cycles CYCLES_RET_CONDITIONAL = kCyclesRetConditional;
inline constexpr Cycles CYCLES_JP_ABSOLUTE = kCyclesJpAbsolute;
inline constexpr Cycles CYCLES_JR_RELATIVE = kCyclesJrRelative;
inline constexpr Cycles CYCLES_RST = kCyclesRst;
inline constexpr Cycles CYCLES_MEMORY_HL_INC_DEC = kCyclesMemoryHlIncDec;
inline constexpr Cycles CYCLES_LD_NN_SP = kCyclesLdNnSp;
inline constexpr Cycles CYCLES_ADD_SP_N = kCyclesAddSpN;

inline constexpr u8 BIT_0 = kBit0;
inline constexpr u8 BIT_7 = kBit7;
inline constexpr u8 NIBBLE_MASK = kNibbleMask;
inline constexpr u8 BYTE_MASK = kByteMask;
inline constexpr u16 ADDR_MASK_12BIT = kAddrMask12Bit;
inline constexpr u16 ADDR_MASK_16BIT = kAddrMask16Bit;
inline constexpr u8 BIT_POSITION_SHIFT = kBitPositionShift;
inline constexpr u8 REGISTER_INDEX_MASK = kRegisterIndexMask;
inline constexpr u8 NIBBLE_SHIFT = kNibbleShift;
inline constexpr u8 BIT_SHIFT = kBitShift;
inline constexpr u8 MEMORY_HL_REGISTER_INDEX = kMemoryHlRegisterIndex;

inline constexpr u16 IO_REGISTERS_BASE = kIoRegistersBase;

inline constexpr u16 INT_VBLANK_VECTOR = kInterruptVBlankVector;
inline constexpr u16 INT_LCD_STAT_VECTOR = kInterruptLcdStatVector;
inline constexpr u16 INT_TIMER_VECTOR = kInterruptTimerVector;
inline constexpr u16 INT_SERIAL_VECTOR = kInterruptSerialVector;
inline constexpr u16 INT_JOYPAD_VECTOR = kInterruptJoypadVector;
inline constexpr u16 REG_INTERRUPT_FLAG = kRegInterruptFlag;
inline constexpr u16 REG_INTERRUPT_ENABLE = kRegInterruptEnable;
inline constexpr u8 INT_VBLANK_BIT = kInterruptVBlankBit;
inline constexpr u8 INT_LCD_STAT_BIT = kInterruptLcdStatBit;
inline constexpr u8 INT_TIMER_BIT = kInterruptTimerBit;
inline constexpr u8 INT_SERIAL_BIT = kInterruptSerialBit;
inline constexpr u8 INT_JOYPAD_BIT = kInterruptJoypadBit;
inline constexpr Cycles CYCLES_INTERRUPT_DISPATCH = kCyclesInterruptDispatch;

inline constexpr u16 RST_00 = kRst00;
inline constexpr u16 RST_08 = kRst08;
inline constexpr u16 RST_10 = kRst10;
inline constexpr u16 RST_18 = kRst18;
inline constexpr u16 RST_20 = kRst20;
inline constexpr u16 RST_28 = kRst28;
inline constexpr u16 RST_30 = kRst30;
inline constexpr u16 RST_38 = kRst38;

inline constexpr u8 BCD_CORRECTION_LOWER = kBcdCorrectionLower;
inline constexpr u8 BCD_CORRECTION_UPPER = kBcdCorrectionUpper;
inline constexpr u8 BCD_DIGIT_MAX = kBcdDigitMax;
inline constexpr u8 BCD_BYTE_MAX = kBcdByteMax;

inline constexpr u16 HL_INCREMENT = kHlIncrement;
inline constexpr u16 HL_DECREMENT = kHlDecrement;

inline constexpr u16 CGB_KEY1 = kCgbKey1Register;
inline constexpr u8 CGB_SPEED_PREPARE_BIT = kCgbSpeedPrepareBit;
inline constexpr u8 CGB_SPEED_TOGGLE_BIT = kCgbSpeedToggleBit;
}

namespace display {
inline constexpr int kLcdWidth = 160;
inline constexpr int kLcdHeight = 144;
inline constexpr int kBytesPerPixel = 4;
inline constexpr int kDefaultScale = 4;
inline constexpr int kDebuggerWindowWidth = 1280;
inline constexpr int kDebuggerWindowHeight = 800;
inline constexpr int kAudioU8Midpoint = 128;
inline constexpr int kAudioU8Scale = 128;
inline constexpr u8 kClearColorR = 0;
inline constexpr u8 kClearColorG = 0;
inline constexpr u8 kClearColorB = 0;
inline constexpr u8 kClearColorA = 255;
inline constexpr std::size_t kSlotLabelSize = 128;
inline constexpr std::size_t kOpenRomPathBufferSize = 1024;
}

namespace recent_roms {
inline constexpr std::size_t kMaxEntries = 10;
}

namespace timer {
inline constexpr u16 kRegDiv = 0xFF04;
inline constexpr u16 kRegTima = 0xFF05;
inline constexpr u16 kRegTma = 0xFF06;
inline constexpr u16 kRegTac = 0xFF07;
inline constexpr u16 kRegIf = 0xFF0F;
inline constexpr u8 kTacEnableBit = 0x04;
inline constexpr u8 kTacClockSelectMask = 0x03;
inline constexpr u16 kClockFreq00 = 1024;
inline constexpr u16 kClockFreq01 = 16;
inline constexpr u16 kClockFreq10 = 64;
inline constexpr u16 kClockFreq11 = 256;
inline constexpr u16 kDivFrequency = 256;
inline constexpr u8 kTimerInterruptBit = 0x04;
}

namespace input {
inline constexpr u16 kRegJoyp = 0xFF00;
inline constexpr u16 kRegIf = 0xFF0F;
inline constexpr u8 kBitSelectButtons = 5;
inline constexpr u8 kBitSelectDirections = 4;
inline constexpr u8 kBitDownStart = 3;
inline constexpr u8 kBitUpSelect = 2;
inline constexpr u8 kBitLeftB = 1;
inline constexpr u8 kBitRightA = 0;
inline constexpr u8 kInputMask = 0x0F;
inline constexpr u8 kSelectMask = 0x30;
inline constexpr u8 kUnusedMask = 0xC0;
inline constexpr u8 kJoypadInterruptBit = 0x10;
inline constexpr u8 kBitNotPressed = 1;
inline constexpr u8 kBitPressed = 0;

inline constexpr int kSdlAxisMax = 32767;
inline constexpr int kDefaultGamepadDeadzone = 8000;
}

namespace memory {
inline constexpr u16 kVramTotalSize = 0x4000;
inline constexpr u16 kVramBankSize = 0x2000;
inline constexpr u16 kWramSize = 0x2000;
}

namespace ppu {
inline constexpr int kScreenWidth = display::kLcdWidth;
inline constexpr int kScreenHeight = display::kLcdHeight;
inline constexpr u16 kOamStart = 0xFE00;
inline constexpr u16 kOamEnd = 0xFEA0;
inline constexpr u8 kOamEntrySize = 4;
inline constexpr u8 kMaxSpritesPerScanline = 10;
inline constexpr u8 kSpriteHeight8x8 = 8;
inline constexpr u8 kSpriteHeight8x16 = 16;
inline constexpr u8 kSpriteFlagPriorityBit = 7;
inline constexpr u8 kSpriteFlagYFlipBit = 6;
inline constexpr u8 kSpriteFlagXFlipBit = 5;
inline constexpr u8 kSpriteFlagPaletteBit = 4;
inline constexpr u8 kSpriteYOffset = 16;
inline constexpr u8 kSpriteXOffset = 8;
inline constexpr u8 kTransparentColor = 0;
}

}  // namespace gbglow::constants