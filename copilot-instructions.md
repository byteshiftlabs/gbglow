# EmuGBC Clean Code Standards

This document defines the coding standards for EmuGBC. The primary goal is **crystal-clear, maintainable code** that serves as an educational reference.

## Core Principles

1. **Clarity over cleverness** - Code should be immediately understandable
2. **Self-documenting** - Names and structure tell the story
3. **Consistent style** - Predictable patterns throughout
4. **Minimal complexity** - Simplest solution that works
5. **Well-documented** - Explain WHY, not WHAT

## Naming Conventions

### Classes and Structs

Use **PascalCase** for types:

```cpp
class CPU { };
class MemoryBankController { };
struct Registers { };
class PPU { };
```

### Functions and Methods

Use **snake_case** for all functions:

```cpp
void execute_instruction();
u8 read_byte(u16 address);
void set_flag_z(bool value);
Cycles run_frame();
```

### Variables

Use **snake_case** for all variables:

```cpp
u16 program_counter;
u8 opcode;
bool vblank_occurred;
int tile_index;
```

### Constants

Use **UPPER_SNAKE_CASE** for compile-time constants:

```cpp
constexpr int SCREEN_WIDTH = 160;
constexpr int SCREEN_HEIGHT = 144;
constexpr Cycles CYCLES_PER_FRAME = 70224;
constexpr u16 VRAM_START = 0x8000;
```

### Private Members

Add **trailing underscore** to private member variables:

```cpp
class Example {
private:
    int value_;
    bool enabled_;
    std::vector<u8> buffer_;
};
```

### Type Aliases

Use consistent abbreviations:

```cpp
using u8 = std::uint8_t;    // unsigned 8-bit
using u16 = std::uint16_t;  // unsigned 16-bit
using i8 = std::int8_t;     // signed 8-bit
```

## File Organization

### Header Files (.h)

```cpp
#pragma once

// System includes first
#include <vector>
#include <cstdint>

// Project includes second
#include "types.h"
#include "memory.h"

// Class declaration
class Example {
public:
    // Constructors and destructor first
    Example();
    explicit Example(int value);
    ~Example();
    
    // Public interface
    void do_something();
    int get_value() const;
    void set_value(int value);
    
private:
    // Private helpers
    void helper_function();
    
    // Private data members last
    int value_;
    bool enabled_;
};
```

### Implementation Files (.cpp)

```cpp
#include "example.h"

// Implementation constructors first
Example::Example()
    : value_(0)      // Initialize in declaration order
    , enabled_(false)
{
}

// Public methods
void Example::do_something() {
    // Implementation
}

// Private methods last
void Example::helper_function() {
    // Implementation
}
```

## Function Design

### Keep Functions Small

Each function should do **one thing well**:

```cpp
// Good: Single responsibility
void CPU::increment_program_counter() {
    regs_.pc++;
}

// Good: Clear purpose
u8 CPU::fetch_byte() {
    return memory_.read(regs_.pc++);
}

// Bad: Too many responsibilities
void CPU::execute_and_update_everything() {
    // 200 lines of mixed logic
}
```

### Clear Parameter Lists

Use descriptive names and reasonable parameter counts:

```cpp
// Good: Clear what each parameter means
void write_tile_pixel(u8 tile_index, u8 x, u8 y, u8 color);

// Good: Few related parameters
void set_position(u16 x, u16 y);

// Bad: Unclear boolean
void set_mode(bool flag);  // What does true mean?

// Good: Named parameter
void set_mode(Mode mode);
enum class Mode { HBlank, VBlank };
```

### Return Values

Prefer explicit return values over output parameters:

```cpp
// Good: Clear return value
u16 calculate_address(u8 bank, u16 offset) {
    return bank * 0x4000 + offset;
}

// Bad: Output parameter obscures intent
void calculate_address(u8 bank, u16 offset, u16& result) {
    result = bank * 0x4000 + offset;
}
```

### Const Correctness

Mark methods and parameters const when they don't modify state:

```cpp
class Memory {
public:
    u8 read(u16 address) const;     // Doesn't modify state
    void write(u16 address, u8 value);  // Modifies state
    
    bool is_empty() const;          // Query method
};
```

## Class Design

### Single Responsibility

Each class should have one clear purpose:

```cpp
// Good: Focused responsibility
class CPU {
    // Only CPU execution logic
};

// Good: Focused responsibility
class Memory {
    // Only memory management
};

// Bad: Mixed responsibilities
class CPUAndMemoryManager {
    // CPU logic + memory management + graphics?
};
```

### Interface Design

Keep public interfaces minimal and clear:

```cpp
class PPU {
public:
    // Clear public interface
    void step(Cycles cycles);
    void render_scanline();
    const std::vector<u8>& get_framebuffer() const;
    
private:
    // Hide implementation details
    void fetch_tile_data();
    void render_pixel(int x, int y);
    u8 get_tile_color(u8 tile_index, u8 x, u8 y);
};
```

### Initialization

Initialize members in declaration order:

```cpp
class Example {
public:
    Example(int a, int b)
        : first_(a)      // Declaration order
        , second_(b)
        , third_(0)
    {
    }
    
private:
    int first_;   // Declared first
    int second_;  // Then second
    int third_;   // Then third
};
```

## Comments and Documentation

### Explain WHY, Not WHAT

```cpp
// Bad: States the obvious
// Increment program counter
regs_.pc++;

// Good: Explains reasoning
// Skip two-byte instruction operand
regs_.pc += 2;

// Bad: Redundant
// Read from memory at address
u8 value = memory_.read(address);

// Good: Explains non-obvious behavior
// Hardware quirk: writes to DIV reset it to 0
if (address == 0xFF04) {
    div_register_ = 0;
    return;
}
```

### Document Complex Logic

```cpp
// Explain algorithm or hardware behavior
// Game Boy tile format is 2-bit planar:
// Each row takes 2 bytes, one bit per pixel per byte
// Combine corresponding bits to get 2-bit color (0-3)
u8 get_tile_pixel(u8 tile_index, u8 x, u8 y) const {
    u16 tile_addr = 0x8000 + (tile_index * 16) + (y * 2);
    u8 low = memory_.read(tile_addr);
    u8 high = memory_.read(tile_addr + 1);
    
    u8 bit = 7 - x;
    return ((high >> bit) & 1) << 1 | ((low >> bit) & 1);
}
```

### Class Documentation

Document class purpose at declaration:

```cpp
// Manages the 64KB Game Boy address space with memory-mapped regions:
// 0x0000-0x7FFF: Cartridge ROM (16KB banks)
// 0x8000-0x9FFF: Video RAM (8KB)
// 0xA000-0xBFFF: External RAM (cartridge)
// 0xC000-0xDFFF: Work RAM (8KB)
// 0xE000-0xFDFF: Echo RAM (mirrors 0xC000-0xDDFF)
// 0xFE00-0xFE9F: Object Attribute Memory (sprite data)
// 0xFF00-0xFF7F: I/O Registers
// 0xFF80-0xFFFE: High RAM (fast internal RAM)
// 0xFFFF: Interrupt Enable Register
class Memory {
    // ...
};
```

### Function Headers

Document public API functions:

```cpp
// Executes a single CPU instruction and returns cycle count.
// Updates program counter, flags, and all affected registers.
// Handles all 256 standard opcodes plus 256 CB prefix opcodes.
Cycles execute_instruction(u8 opcode);
```

## Code Structure

### Avoid Deep Nesting

Keep nesting to 2-3 levels maximum:

```cpp
// Bad: Deep nesting
void process() {
    if (condition1) {
        if (condition2) {
            if (condition3) {
                if (condition4) {
                    // Deep logic
                }
            }
        }
    }
}

// Good: Early returns
void process() {
    if (!condition1) return;
    if (!condition2) return;
    if (!condition3) return;
    if (!condition4) return;
    
    // Main logic at top level
}
```

### Use Meaningful Intermediate Variables

```cpp
// Bad: Complex expression
if ((memory_.read(0xFF40) & 0x80) && scanline_ >= 144) {
    // ...
}

// Good: Named intermediate values
bool lcd_enabled = (memory_.read(0xFF40) & 0x80) != 0;
bool in_vblank = scanline_ >= 144;

if (lcd_enabled && in_vblank) {
    // ...
}
```

### Switch Statements

Use switches for discrete cases, format consistently:

```cpp
Cycles CPU::execute_instruction(u8 opcode) {
    switch (opcode) {
        case 0x00:  // NOP
            return 1;
            
        case 0x01:  // LD BC, nn
            regs_.bc(read_word_pc());
            return 3;
            
        case 0x02:  // LD (BC), A
            memory_.write(regs_.bc(), regs_.a);
            return 2;
            
        default:
            throw std::runtime_error("Unknown opcode");
    }
}
```

## Error Handling

### Use Exceptions for Unrecoverable Errors

```cpp
// Good: Exception for programming error
if (address >= MEMORY_SIZE) {
    throw std::out_of_range("Memory access out of bounds");
}

// Good: Exception for invalid state
if (!cartridge_loaded_) {
    throw std::runtime_error("No cartridge loaded");
}
```

### Return Values for Expected Failures

```cpp
// Good: Boolean for expected failure cases
bool Cartridge::load_save_file() {
    std::ifstream file(save_path_);
    if (!file) {
        return false;  // No save file exists (normal)
    }
    // Load data...
    return true;
}
```

### Descriptive Error Messages

```cpp
// Bad: Generic message
throw std::runtime_error("Error");

// Good: Specific, actionable message
throw std::runtime_error(
    "Unsupported cartridge type 0x" + to_hex(type) +
    ". Only ROM-only and MBC1 are currently supported."
);
```

## Type Safety

### Use Enums for Discrete States

```cpp
// Good: Type-safe states
enum class PPUMode {
    HBlank,
    VBlank,
    OAMSearch,
    Transfer
};

PPUMode mode_ = PPUMode::HBlank;

// Bad: Magic numbers
int mode_ = 0;  // What do the values mean?
```

### Use Strong Types

```cpp
// Good: Distinct types prevent mixing
using Cycles = u32;
using Address = u16;

Cycles cycles = 4;
Address addr = 0xC000;

// Compiler prevents: cycles = addr;  // Type error!
```

### Explicit Constructors

Prevent implicit conversions:

```cpp
class CPU {
public:
    explicit CPU(Memory& memory);  // Must be explicit
    
    // Bad: Allows CPU cpu = memory;
    // CPU(Memory& memory);
};
```

## Modern C++ Features

### Use auto for Obvious Types

```cpp
// Good: Type is obvious from right side
auto cartridge = std::make_unique<MBC1>(rom_data);
auto it = map.find(key);

// Bad: Obscures type unnecessarily
auto value = calculate();  // What type is returned?

// Good: Explicit when not obvious
u16 value = calculate();
```

### Range-Based For Loops

```cpp
// Good: Clear iteration
for (u8 byte : rom_data) {
    checksum += byte;
}

// Good: Reference to avoid copies
for (const Sprite& sprite : sprites) {
    render_sprite(sprite);
}
```

### Smart Pointers

```cpp
// Good: Ownership is clear
class Memory {
private:
    std::unique_ptr<Cartridge> cartridge_;  // Memory owns cartridge
};

// Good: Non-owning reference
class CPU {
public:
    explicit CPU(Memory& memory)  // CPU doesn't own memory
        : memory_(memory) { }
        
private:
    Memory& memory_;  // Reference, not owning
};
```

## Formatting

### Indentation

- Use **4 spaces** (no tabs)
- Align related code vertically when helpful

```cpp
void example() {
    int first  = 1;
    int second = 2;
    int third  = 3;
}
```

### Braces

- Opening brace on same line for classes/functions
- Control flow braces on next line (optional, be consistent)

```cpp
class Example {
    void function() {
        if (condition) {
            action();
        }
    }
};
```

### Line Length

- Aim for **80-100 characters**
- Break long lines at logical points

```cpp
// Good: Break at logical boundary
bool result = complex_condition_one() &&
              complex_condition_two() &&
              complex_condition_three();
```

### Whitespace

```cpp
// Good spacing around operators
int result = a + b * c;

// Good spacing in function calls
function(arg1, arg2, arg3);

// No space before parentheses
if (condition) { }
while (condition) { }

// Space after keywords
if (test) { }
for (int i = 0; i < 10; ++i) { }
```

## Code Examples

### Before (Bad Style)

```cpp
class c{public:void f(){int x=g(1,2);if(x>0){for(int i=0;i<x;i++){p(i);}}}};
```

### After (Good Style)

```cpp
class Counter {
public:
    void process() {
        int value = calculate(1, 2);
        
        if (value > 0) {
            for (int i = 0; i < value; ++i) {
                print_value(i);
            }
        }
    }
    
private:
    int calculate(int a, int b);
    void print_value(int value);
};
```

## Summary

**Remember the mission:** Write code that teaches. Every line should be clear enough that someone learning Game Boy emulation can understand it immediately.

When in doubt:
1. Choose clarity over cleverness
2. Add a comment explaining WHY
3. Use descriptive names
4. Keep functions small and focused
5. Follow consistent patterns
