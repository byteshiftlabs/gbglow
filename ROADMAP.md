# EmuGBC Development Roadmap

## Project Goal
Create a crystal-clear, educational Game Boy Color emulator capable of running commercial games, particularly Pokémon titles.

---

## ✅ Phase 1-4: Foundation & Code Quality (COMPLETE)

### Phase 1: Core Architecture
- ✅ CPU structure (registers, flags)
- ✅ Memory Management Unit (MMU) with full address space
- ✅ Basic PPU structure (background rendering)
- ✅ Cartridge abstraction (ROM-only, MBC1)
- ✅ Type system and core utilities

### Phase 2: Code Quality & Standards
- ✅ Move all inline implementations to .cpp files
- ✅ Fix single-line statement violations
- ✅ Proper include ordering (system → project)
- ✅ Replace WHAT comments with WHY comments

### Phase 3: Documentation
- ✅ Comprehensive inline documentation
- ✅ Hardware behavior explanations
- ✅ Design decision rationale

### Phase 4: Professional Polish
- ✅ Named constants for all magic numbers
- ✅ Memory map constants (30+ constants)
- ✅ Hardware dimension constants
- ✅ Enhanced TODO documentation
- ✅ Zero compilation warnings

**Status:** 100% complete, production-ready code quality

---

## ✅ Phase 5: CPU Instruction Execution (COMPLETE)

**Complexity:** High (1000+ lines)  
**Priority:** CRITICAL BLOCKER - Nothing runs without this  
**Dependencies:** Phase 1-4

### Objectives
Implement all 256 base opcodes and 256 CB-prefixed opcodes for the Sharp LR35902 CPU.

### Implementation Details
- ✅ Base opcode decoder with switch statement (256 cases)
- ✅ All 8-bit register-to-register loads (LD r,r)
- ✅ Memory loads/stores (LD r,(HL), LD (HL),r)
- ✅ Immediate loads (LD r,n, LD rr,nn)
- ✅ Stack operations (PUSH/POP all register pairs)
- ✅ 8-bit ALU operations (ADD, ADC, SUB, SBC, AND, OR, XOR, CP)
- ✅ 8/16-bit INC/DEC operations
- ✅ Jumps: JP, JR (absolute and relative, conditional)
- ✅ Calls and Returns: CALL, RET, RETI, RST (with conditions)
- ✅ Special loads: LDH, LD (HL+), LD (HL-), LD (nn),SP
- ✅ 16-bit ADD operations
- ✅ Rotate/shift operations on A (RLCA, RLA, RRCA, RRA)
- ✅ Misc operations: DAA, CPL, CCF, SCF
- ✅ CB-prefixed instructions:
  - ✅ RLC/RRC (rotate left/right circular)
  - ✅ RL/RR (rotate through carry)
  - ✅ SLA/SRA/SRL (shift arithmetic/logical)
  - ✅ SWAP (swap nibbles)
  - ✅ BIT (test bit)
  - ✅ RES (reset bit)
  - ✅ SET (set bit)

### Testing
- ✅ All tests pass
- ✅ Zero compilation warnings
- ✅ Proper M-cycle counting (not T-cycles)

**Status:** 100% complete - All 512 opcodes implemented and tested

---

## ✅ Phase 6: Interrupt System (COMPLETE)

**Goal:** Implement Game Boy interrupt handling mechanism

**Complexity:** 🟡 Medium  
**Estimated Lines:** 150-200  
**Priority:** CRITICAL - Required for VBlank, Input, Timer

### Implementation Details
- ✅ VBlank interrupt (INT 0x40)
- ✅ LCD STAT interrupt (INT 0x48)
- ✅ Timer interrupt (INT 0x50)
- ✅ Serial interrupt (INT 0x58)
- ✅ Joypad interrupt (INT 0x60)
- ✅ IME (Interrupt Master Enable) flag management
- ✅ IF register (0xFF0F) - Interrupt Flags
- ✅ IE register (0xFFFF) - Interrupt Enable
- ✅ Interrupt priority handling (VBlank > LCD > Timer > Serial > Joypad)
- ✅ EI/DI instruction integration
- ✅ HALT mode wake-up on interrupt
- ✅ Proper interrupt dispatch (push PC, jump to vector, disable IME)

### Acceptance Criteria
- ✅ All 5 interrupt types functional
- ✅ Correct priority order
- ✅ IME flag properly managed
- ✅ Interrupt timing accurate
- ✅ request_interrupt() method for external interrupt sources

**Status:** 100% complete - Full interrupt system operational

---

## ✅ Phase 7: MBC3 Cartridge Support (COMPLETE)

**Goal:** Support MBC3 cartridges used by Pokémon games

**Complexity:** 🟡 Medium  
**Estimated Lines:** 200-300  
**Priority:** HIGH - Required for Pokémon Red/Blue/Yellow/Gold/Silver/Crystal

### Requirements
- ROM banking (up to 128 banks × 16KB)
- RAM banking (4 banks × 8KB)
- Bank switching logic
- Real-Time Clock (RTC) registers (for Gold/Silver/Crystal)
  - Seconds register
  - Minutes register
  - Hours register
  - Days register (lower)
  - Days register (upper) + flags
- RTC latching mechanism
- Battery-backed RAM indication

### Implementation Details
- **Files Created:**
  - `src/cartridge/mbc3.h` - MBC3 class interface with RTC support
  - `src/cartridge/mbc3.cpp` - ROM/RAM banking and RTC implementation
- **Files Modified:**
  - `src/cartridge/cartridge.h` - Added base constructor for derived classes
  - `src/cartridge/cartridge.cpp` - Added MBC3 cartridge type recognition and loading
  - `src/cartridge/mbc1.cpp` - Updated to use base constructor
  - `CMakeLists.txt` - Added mbc3.cpp to build
  - `tests/CMakeLists.txt` - Added mbc3.cpp to tests
- **Features:**
  - 7-bit ROM banking (0x01-0x7F, 128 banks)
  - 2-bit RAM banking (0x00-0x03, 4 banks)
  - RTC support with 5 registers (seconds, minutes, hours, days_low, days_high)
  - RTC latching mechanism (write 0x00 then 0x01)
  - Battery-backed RAM recognition
  - Zero magic numbers (all constants properly named)

### Acceptance Criteria
- ✅ ROM banking functional (128 banks)
- ✅ RAM banking functional (4 banks)
- ✅ RTC registers implemented
- ✅ RTC latching works correctly
- ✅ Pokémon games can load and read cartridge header
- ✅ All tests passing
- ✅ Zero compilation warnings
- ✅ Clean code compliance (zero magic numbers)

---

## 🎨 Phase 8: Sprite Rendering (OAM) (NEXT UP)

**Goal:** Render sprites for characters, items, and UI elements

**Complexity:** 🟡 Medium-High  
**Estimated Lines:** 300-400  
**Priority:** HIGH - Required for visible game elements

### Requirements
- OAM (Object Attribute Memory) parsing (0xFE00-0xFE9F)
- Sprite attributes:
  - Y position
  - X position
  - Tile index
  - Flags (priority, Y-flip, X-flip, palette)
- 8x8 sprite mode
- 8x16 sprite mode (OBJ Size bit in LCDC)
- Sprite-to-background priority
- Sprite-to-sprite priority
- Per-scanline sprite limit (10 sprites)
- OBJ palette support (OBP0, OBP1)
- Sprite rendering during Mode 3
- Transparent color (color 0)

### Acceptance Criteria
- [ ] Sprites render correctly
- [ ] Priority system works
- [ ] 10-sprite-per-line limit enforced
- [ ] Flip operations work
- [ ] Pokémon character visible on screen

---

## 🕹️ Phase 9: Input System (Joypad)

**Goal:** Enable player input for game control

**Complexity:** 🟢 Low  
**Estimated Lines:** 100-150  
**Priority:** HIGH - Required to play games

### Requirements
- Joypad register (0xFF00) implementation
- Button inputs:
  - A button
  - B button
  - Start button
  - Select button
- Direction inputs:
  - Up
  - Down
  - Left
  - Right
- Button/Direction mode selection
- Joypad interrupt trigger on press
- Keyboard mapping (for testing)

### Acceptance Criteria
- [ ] All 8 buttons functional
- [ ] Mode selection works
- [ ] Joypad interrupt triggers
- [ ] Can navigate Pokémon menus
- [ ] Can move player character

---

## ⏱️ Phase 10: Timer System

**Goal:** Implement hardware timer for game timing and events

**Complexity:** 🟢 Low-Medium  
**Estimated Lines:** 100-150  
**Priority:** MEDIUM - Required for timing-dependent games

### Requirements
- DIV register (0xFF04) - Divider register
  - Increments at 16384 Hz
  - Resets to 0 on write
- TIMA register (0xFF05) - Timer counter
  - Configurable frequency
  - Triggers interrupt on overflow
- TMA register (0xFF06) - Timer modulo
  - Reload value for TIMA on overflow
- TAC register (0xFF07) - Timer control
  - Enable/disable timer
  - Clock select (4 frequencies)
- Timer interrupt on TIMA overflow
- Accurate cycle counting

### Acceptance Criteria
- [ ] DIV increments correctly
- [ ] TIMA configurable and functional
- [ ] Timer interrupt triggers on overflow
- [ ] All 4 clock frequencies work

---

## 💾 Phase 11: Save File Support

**Goal:** Persistent storage for battery-backed RAM

**Complexity:** 🟢 Low  
**Estimated Lines:** 50-100  
**Priority:** HIGH - Required to save Pokémon progress

### Requirements
- .sav file format
- Load RAM from .sav on startup
- Save RAM to .sav periodically
- Save on emulator shutdown
- Handle missing .sav files (initialize)
- RTC state persistence (for MBC3)

### Acceptance Criteria
- [ ] Can save Pokémon game
- [ ] Saves persist across runs
- [ ] RTC state persists (for Gold/Silver/Crystal)
- [ ] No data corruption

---

## 🪟 Phase 12: Window Layer

**Goal:** Implement window overlay for menus and text

**Complexity:** 🟢 Low-Medium  
**Estimated Lines:** 100-150  
**Priority:** MEDIUM - Enhances graphics accuracy

### Requirements
- Window enable bit (LCDC bit 5)
- WX register (0xFF4B) - Window X position
- WY register (0xFF4A) - Window Y position
- Window tile map selection
- Window-background overlap handling
- Window internal line counter

### Acceptance Criteria
- [ ] Window renders correctly
- [ ] Window position adjustable
- [ ] Pokémon text boxes display properly

---

## 🎵 Phase 13: Audio Processing Unit (APU)

**Goal:** Sound and music generation

**Complexity:** 🔴 High  
**Estimated Lines:** 500-800  
**Priority:** LOW - Enhances experience but optional

### Requirements
- Channel 1: Square wave with sweep
- Channel 2: Square wave
- Channel 3: Programmable wave
- Channel 4: Noise
- Sound control registers (0xFF10-0xFF26)
- Volume control
- Panning control
- Audio mixing
- Sample output (WAV or direct audio)

### Acceptance Criteria
- [ ] All 4 channels functional
- [ ] Pokémon music plays
- [ ] Sound effects work
- [ ] Volume control responsive

---

## 🎨 Phase 14: Color Support (GBC Mode)

**Goal:** Full Game Boy Color palette system

**Complexity:** 🟡 Medium  
**Estimated Lines:** 200-300  
**Priority:** MEDIUM - Required for GBC-only games

### Requirements
- CGB mode detection
- Background palette memory (BCPS/BCPD)
- Sprite palette memory (OCPS/OCPD)
- 32K×15-bit color palettes
- Palette auto-increment
- Double-speed mode (CPU at 8MHz)
- VRAM banking (2 banks)

### Acceptance Criteria
- [ ] GBC palettes functional
- [ ] Colors display correctly
- [ ] Double-speed mode works

---

## 🚀 Phase 15: Performance & Polish

**Goal:** Optimize and refine emulator

**Complexity:** 🟡 Medium  
**Priority:** LOW - Quality of life improvements

### Requirements
- Cycle-accurate timing
- Frame rate limiting (60 FPS)
- Turbo mode (fast forward)
- Save states
- Debugger integration
- Memory viewer
- Performance profiling
- Extended test suite

---

## 🎯 Minimum Viable Pokémon (MVP)

To run Pokémon Red/Blue/Yellow at a basic playable level:

1. ✅ Phase 1-4: Foundation (COMPLETE)
2. 🚧 Phase 5: CPU Instructions (IN PROGRESS)
3. Phase 6: Interrupts
4. Phase 7: MBC3 Support
5. Phase 8: Sprite Rendering
6. Phase 9: Input System
7. Phase 10: Timer
8. Phase 11: Save Files

**After MVP:** Phases 12-15 are enhancements for better accuracy and features.

---

## 📊 Progress Tracking

| Phase | Status | Completion |
|-------|--------|------------|
| 1-4: Foundation | ✅ Complete | 100% |
| 5: CPU Instructions | 🚧 In Progress | 5% |
| 6: Interrupts | ⏳ Planned | 0% |
| 7: MBC3 | ⏳ Planned | 0% |
| 8: Sprites | ⏳ Planned | 0% |
| 9: Input | ⏳ Planned | 0% |
| 10: Timer | ⏳ Planned | 0% |
| 11: Save Files | ⏳ Planned | 0% |
| 12: Window | ⏳ Future | 0% |
| 13: Audio | ⏳ Future | 0% |
| 14: Color | ⏳ Future | 0% |
| 15: Polish | ⏳ Future | 0% |

**Overall Progress: ~20%** (Foundation complete, CPU in progress)

---

## 🎮 Target Games

### Primary Targets (MVP)
- ✅ Tetris (ROM-only) - Should work after Phase 5
- 🎯 Pokémon Red/Blue/Yellow (MBC3) - Target for MVP
- 🎯 Pokémon Gold/Silver/Crystal (MBC3+RTC) - Target after MVP

### Secondary Targets
- Super Mario Land (ROM-only)
- The Legend of Zelda: Link's Awakening (MBC5)
- Kirby's Dream Land (ROM-only)
- Metroid II (MBC1)

### Stretch Goals
- Pokémon Trading Card Game (MBC5)
- Super Mario Bros. Deluxe (GBC, MBC5)
- The Legend of Zelda: Oracle series (GBC, MBC5)

---

## 📚 Testing Strategy

### Unit Tests
- CPU instruction tests (Blargg's test ROMs)
- Memory system tests
- Cartridge type tests
- PPU timing tests

### Integration Tests
- Full ROM execution tests
- Save/load tests
- Interrupt handling tests

### Acceptance Tests
- Can boot Pokémon Red
- Can see title screen
- Can start new game
- Can walk around
- Can enter battle
- Can save game
- Can load saved game

---

## 🛠️ Development Principles

1. **Clarity over Performance** - Code must teach, not just execute
2. **Standards Compliance** - 100% adherence to copilot-instructions.md
3. **Incremental Progress** - Small, testable commits
4. **Documentation First** - Explain WHY before implementing
5. **Crystal-Clear Code** - Every line should be obvious
6. **Test-Driven** - Validate with known-good test ROMs
7. **Educational Value** - Reference implementation for learning

---

## 📖 Resources

- Pan Docs: https://gbdev.io/pandocs/
- CPU Instruction Reference: https://gbdev.io/gb-opcodes/
- Blargg's Test ROMs: https://github.com/retrio/gb-test-roms
- Game Boy CPU Manual: http://marc.rawer.de/Gameboy/Docs/GBCPUman.pdf
- Ultimate Game Boy Talk: https://www.youtube.com/watch?v=HyzD8pNlpwI

---

**Last Updated:** December 17, 2025
**Current Focus:** Phase 5 - CPU Instruction Execution
