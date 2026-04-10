# GBCrush Clean Code Standards

This document defines the coding standards for GBCrush. The primary goal is **maintainable code** that serves as an educational reference.

## Core Principles

1. **Clarity over cleverness** - Code should be immediately understandable
2. **Self-documenting** - Names and structure tell the story
3. **Consistent style** - Predictable patterns throughout
4. **Minimal complexity** - Simplest solution that works
5. **Well-documented** - Explain WHY, not WHAT

---

## 1. Naming Conventions

### Classes and Types
- Use **PascalCase** for all class names, struct names, and type definitions
- Names should clearly indicate the entity's purpose

### Functions and Methods
- Use **snake_case** for all function and method names
- Use verb phrases that describe what the function does
- Boolean-returning functions should start with `is_`, `has_`, `can_`, or similar

### Variables
- Use **snake_case** for all local variables, parameters, and non-const members
- Choose descriptive names that reveal intent, not implementation
- Avoid abbreviations unless universally understood (like "addr" for address)

### Constants
- Use **UPPER_SNAKE_CASE** for all compile-time constants
- Define magic numbers as named constants
- Use constexpr for compile-time values

### Private Members
- Add **trailing underscore** to all private and protected member variables
- This clearly distinguishes members from local variables

### Type Aliases
- Use consistent abbreviations: `u8`, `u16`, `u32`, `u64` for unsigned types
- Use `i8`, `i16`, `i32`, `i64` for signed types
- Create type aliases for domain concepts (like `Cycles`, `Address`)

---

## 2. Code Organization

### Header Files
- Use `#pragma once` instead of include guards
- Order includes: system headers first, then project headers
- Group class members: constructors/destructor, public interface, private helpers, private data
- Declare data members last in classes

### Implementation Files
- Include the corresponding header first
- Implement constructors first, then public methods, then private methods
- Keep related functions together

### File Structure
- One class per file pair (header + implementation)
- Name files after the primary class they contain
- Use subdirectories to organize related components

### Dependencies
- Minimize header dependencies, use forward declarations when possible
- Avoid circular dependencies
- Keep coupling loose between major components

### Separation of Concerns
- Put large constant tables or lookup arrays in separate files
- Keep hardware-specific constants grouped together
- Separate interface definitions from implementations

---

## 3. Functions and Methods

### Single Responsibility
- Each function should do one thing and do it well
- If a function needs "and" in its name, consider splitting it

### Function Size
- Keep functions small - aim for less than 30 lines
- Complex logic should be broken into helper functions
- Extract nested loops and conditionals into named functions

### Parameter Lists
- Keep parameter counts low (ideally 3 or fewer)
- Use descriptive parameter names
- Avoid boolean parameters - use enums for clarity

### Return Values
- Prefer return values over output parameters
- Use early returns to reduce nesting
- Return const references for large objects that don't need copying

### Const Correctness
- Mark methods const if they don't modify object state
- Mark parameters const when they shouldn't be modified
- Use const references for parameters you don't need to copy

### Default Arguments
- Avoid default arguments - they hide behavior
- Make function calls explicit and clear

---

## 4. Variables and State

### Initialization
- Initialize all variables at declaration
- Use constructor initialization lists in member initialization order
- Initialize in the same order as member declaration

### Scope
- Minimize variable scope - declare as close to use as possible
- Avoid global variables
- Prefer local variables over members when state doesn't need to persist

### Mutability
- Prefer const variables when values don't change
- Minimize mutable state
- Make data members private and provide controlled access

### Large Data Structures
- Move large arrays and lookup tables to separate files
- Use static const for compile-time data
- Consider lazy initialization for expensive-to-compute data

### Magic Numbers
- Replace all magic numbers with named constants
- Group related constants together
- Document the meaning and source of hardware-specific values

---

## 5. Comments and Documentation

### Comment Purpose
- Explain **WHY**, not **WHAT** - code should show what it does
- Document hardware behavior, quirks, and timing requirements
- Explain non-obvious algorithms or bit manipulations

### Class Documentation
- Document the purpose and responsibility of each class
- Explain memory maps, register layouts, or hardware behavior
- Include relevant technical reference information

### Function Documentation
- Document public API functions with their purpose and behavior
- Explain preconditions, postconditions, and side effects
- Document return values and parameters for complex functions

### Implementation Comments
- Comment complex algorithms or hardware-specific logic
- Explain timing-critical operations
- Note any deviations from standard behavior or workarounds

---

## 6. Error Handling

### Exceptions
- Use exceptions for unrecoverable errors (programming bugs, invalid state)
- Throw appropriate standard exceptions (out_of_range, runtime_error, logic_error)
- Let exceptions propagate to appropriate handlers

### Return Values
- Use return values (bool, optional, expected) for expected failures
- Return false/null/error codes when failure is part of normal operation
- Document error conditions in function comments

### Error Messages
- Provide specific, actionable error messages
- Include context: what failed, why, and what values were involved
- Avoid generic messages like "Error" or "Failed"

---

## 7. Type Safety

### Enums
- Use `enum class` for type-safe enumerations
- Give enums meaningful names that describe the concept
- Use enums instead of boolean flags when meaning would be unclear

### Strong Types
- Create type aliases for domain concepts (Cycles, Address)
- Use distinct types to prevent mixing incompatible values
- Leverage the type system to catch errors at compile time

### Explicit Conversions
- Use explicit constructors to prevent implicit conversions
- Be explicit about type conversions and casts
- Avoid C-style casts, use static_cast when needed

### Type Deduction
- Use auto when the type is obvious from the right-hand side
- Be explicit when the type is important for understanding
- Don't use auto to hide important type information

---

## 8. Modern C++ Practices

### Smart Pointers
- Use `unique_ptr` for exclusive ownership
- Use `shared_ptr` only when shared ownership is needed
- Use raw references or raw pointers for non-owning relationships
- Never manually delete - let RAII handle cleanup

### STL Containers
- Use standard containers (vector, array, map) over raw arrays
- Prefer vector for dynamic arrays
- Use array for fixed-size arrays
- Reserve capacity for vectors when size is known

### Range-Based Loops
- Use range-based for loops for simple iteration
- Use const references in loops to avoid copies
- Use traditional loops when you need the index

### Lambdas
- Use lambdas for short, local function objects
- Capture by reference [&] when you need to modify external state
- Capture by value [=] when you need a copy
- Be explicit about captures when ownership matters

### Move Semantics
- Return large objects by value and let the compiler optimize
- Use std::move for transferring ownership
- Don't use std::move on return values (prevents copy elision)

### constexpr
- Use constexpr for compile-time constants
- Mark functions constexpr when they can be evaluated at compile time
- Prefer constexpr over #define for constants

---

## 9. Performance Considerations

### Premature Optimization
- Write clear code first, optimize only if needed
- Profile before optimizing
- Document why optimizations are necessary

### Copying
- Pass large objects by const reference
- Use move semantics for transferring ownership
- Return by value and trust the compiler to optimize

### Allocation
- Avoid frequent allocations in hot paths
- Reuse buffers when possible
- Pre-allocate collections when size is known

### Inlining
- Let the compiler decide about inlining for most functions
- Only force inline for proven hot spots
- Keep inline functions small and simple

### Cache Locality
- Keep hot data structures compact
- Use structure-of-arrays for performance-critical data
- Consider data access patterns in memory layout

---

## 10. Hardware Emulation Specific

### Timing Accuracy
- Document cycle counts for all operations
- Make timing behavior explicit and verifiable
- Use named constants for hardware timing values

### State Management
- Keep emulator state clearly organized
- Separate logical components (CPU, PPU, APU, Memory)
- Make state serializable for save states

### Hardware Registers
- Document register addresses and bit meanings
- Use named constants for register addresses
- Use bit manipulation helpers for clarity

### Testing
- Write tests for CPU instructions
- Test edge cases and hardware quirks
- Verify timing-dependent behavior

---

## Summary

**Remember the mission:** Write code that teaches. Every line should be clear enough that someone learning Game Boy emulation can understand it immediately.

When in doubt:
1. Choose clarity over cleverness
2. Add a comment explaining WHY
3. Use descriptive names
4. Keep functions small and focused
5. Follow consistent patterns
6. Let the type system help you
7. Document hardware behavior
8. Write testable code
