#pragma once

#include "../core/types.h"
#include <vector>
#include <set>
#include <string>
#include <deque>
#include <functional>

namespace gbglow {

// Forward declarations
class CPU;
class Memory;
class PPU;
class Registers;

/**
 * Instruction disassembly result
 */
struct DisassembledInstruction {
    u16 address;
    std::vector<u8> bytes;
    std::string mnemonic;
    std::string operands;
    bool is_breakpoint;
};

/**
 * Memory watch entry
 */
struct MemoryWatch {
    u16 address;
    std::string label;
    u8 last_value;
    bool break_on_change;
};

/**
 * Built-in Debugger for GBCrush
 * 
 * Provides comprehensive debugging capabilities:
 * - Breakpoints (address-based)
 * - Step execution (step into, step over)
 * - Memory inspection and editing
 * - Register viewing
 * - Disassembly view
 * - Memory watches
 * - Execution history
 */
class Debugger {
public:
    Debugger();
    ~Debugger() = default;
    
    /**
     * Connect debugger to emulator components
     */
    void attach(CPU* cpu, Memory* memory, PPU* ppu);
    
    /**
     * Check if debugger is attached
     */
    bool is_attached() const;
    
    // === Breakpoint Management ===
    
    /**
     * Add a breakpoint at address
     */
    void add_breakpoint(u16 address);
    
    /**
     * Remove breakpoint at address
     */
    void remove_breakpoint(u16 address);
    
    /**
     * Toggle breakpoint at address
     */
    void toggle_breakpoint(u16 address);
    
    /**
     * Check if address has a breakpoint
     */
    bool has_breakpoint(u16 address) const;
    
    /**
     * Clear all breakpoints
     */
    void clear_all_breakpoints();
    
    /**
     * Get all breakpoint addresses
     */
    const std::set<u16>& get_breakpoints() const;
    
    // === Execution Control ===
    
    /**
     * Check if execution should pause (hit breakpoint)
     * Call before each CPU step
     */
    bool should_break(u16 pc) const;
    
    /**
     * Request single step
     */
    void request_step();
    
    /**
     * Request step over (skip calls)
     */
    void request_step_over();
    
    /**
     * Check if step was requested
     */
    bool step_requested() const;
    
    /**
     * Clear step request after execution
     */
    void clear_step_request();
    
    /**
     * Check if we're in step-over mode
     */
    bool is_step_over_active() const;
    
    /**
     * Set step-over return address
     */
    void set_step_over_return(u16 address);
    
    /**
     * Check if we should stop step-over
     */
    bool should_stop_step_over(u16 pc) const;
    
    /**
     * Clear step-over state
     */
    void clear_step_over();
    
    // === Disassembly ===
    
    /**
     * Disassemble a single instruction at address
     * Returns the instruction and updates next_address
     */
    DisassembledInstruction disassemble_at(u16 address, u16& next_address) const;
    
    /**
     * Disassemble a range of instructions
     */
    std::vector<DisassembledInstruction> disassemble_range(u16 start, int count) const;
    
    /**
     * Get disassembly around PC
     */
    std::vector<DisassembledInstruction> disassemble_around_pc(int lines_before, int lines_after) const;
    
    // === Memory Inspection ===
    
    /**
     * Read memory byte (safe, returns 0 if not attached)
     */
    u8 read_memory(u16 address) const;
    
    /**
     * Write memory byte
     */
    void write_memory(u16 address, u8 value);
    
    /**
     * Read memory region
     */
    std::vector<u8> read_memory_region(u16 start, u16 length) const;
    
    // === Memory Watches ===
    
    /**
     * Add memory watch
     */
    void add_watch(u16 address, const std::string& label = "", bool break_on_change = false);
    
    /**
     * Remove memory watch
     */
    void remove_watch(u16 address);
    
    /**
     * Get all watches
     */
    const std::vector<MemoryWatch>& get_watches() const;
    
    /**
     * Update watch values and check for breaks
     * Returns true if a watched value changed with break_on_change set
     */
    bool update_watches();
    
    // === Execution History ===
    
    /**
     * Record executed instruction (call after each step)
     */
    void record_execution(u16 pc);
    
    /**
     * Get execution history
     */
    const std::deque<u16>& get_execution_history() const;
    
    /**
     * Clear execution history
     */
    void clear_history();
    
    /**
     * Set maximum history size
     */
    void set_max_history(size_t max_size);
    
    // === Register Access ===
    
    /**
     * Get CPU registers (const)
     */
    const Registers* get_registers() const;
    
    /**
     * Get current PC
     */
    u16 get_pc() const;
    
    // === Utility ===
    
    /**
     * Get human-readable memory region name
     */
    static std::string get_memory_region_name(u16 address);
    
    /**
     * Format address with region name
     */
    static std::string format_address(u16 address);
    
private:
    CPU* cpu_;
    Memory* memory_;
    PPU* ppu_;
    
    // Breakpoints
    std::set<u16> breakpoints_;
    
    // Step control
    bool step_requested_;
    bool step_over_active_;
    u16 step_over_return_address_;
    
    // Memory watches
    std::vector<MemoryWatch> watches_;
    
    // Execution history
    std::deque<u16> execution_history_;
    size_t max_history_size_;
    
    // Disassembly helpers
    std::string disassemble_opcode(u8 opcode, u16 address, u16& next_address, std::vector<u8>& bytes) const;
    std::string disassemble_cb_opcode(u8 opcode) const;
    std::string get_register_name_8(u8 reg_index) const;
    std::string get_register_name_16(u8 reg_index) const;
    std::string get_condition_name(u8 cond_index) const;
};

} // namespace gbglow
