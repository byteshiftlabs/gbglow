# gbglow Project-Specific Standards

For general coding guidelines (code review, architecture, error handling, documentation, etc.), follow the shared prompts in `ai-dev-prompts/`.

This file covers **gbglow-specific** conventions only.

---

## Naming Conventions

- **Classes/Types**: PascalCase (`Memory`, `Cartridge`, `PPU`)
- **Functions/Methods**: snake_case (`step()`, `handle_interrupts()`, `is_halted()`)
- **Variables**: snake_case; descriptive names (`vram_bank_`, not `vb`)
- **Constants**: UPPER_SNAKE_CASE, always `constexpr` (`VRAM_BANK_SIZE`, `OAM_START`)
- **Private members**: trailing underscore (`regs_`, `halted_`, `cycle_accumulator_`)
- **Booleans**: prefix with `is_`, `has_`, `can_` (`is_cgb_supported()`, `has_battery()`)

## Type Aliases

Use the project-defined fixed-width types from `types.h`:
- `u8`, `u16`, `u32`, `u64` for unsigned
- `i8`, `i16`, `i32`, `i64` for signed
- `Cycles` for cycle counts, `Address` for memory addresses

## Hardware Emulation Patterns

- **One class per hardware component**: CPU, PPU, APU, Memory, Timer, Joypad, Cartridge
- **Cycle accuracy**: document cycle counts for all operations with named constants
- **Register documentation**: document addresses and bit meanings with `constexpr` constants
- **State serialization**: every stateful component implements `serialize()`/`deserialize()`
- **Memory map constants**: use named constants for all Game Boy address ranges (e.g. `WRAM_START`, `VRAM_END`, `OAM_START`)
- **Magic numbers**: replace all numeric literals with named `constexpr` constants — no raw hex offsets, sizes, or hardware values in logic

## File Organization

- One class per `.h`/`.cpp` pair, named after the class
- Headers: `#pragma once`, system includes first, then project includes
- Class layout: constructors → public interface → private helpers → private data
- Large lookup tables go in separate files

## C++ Style

- C++17 standard
- `static_cast` over C-style casts
- `unique_ptr` for ownership, raw pointers/references for non-owning access
- `std::array` for fixed-size, `std::vector` for dynamic
- `constexpr` over `#define`
- `enum class` over plain enums
- Const correctness everywhere

## Mission

Write code that teaches. Every line should be clear enough that someone learning Game Boy emulation can understand it immediately.
