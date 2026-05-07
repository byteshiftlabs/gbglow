// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 gbglow Contributors
// This file is part of gbglow. See LICENSE for details.

#include "debugger.h"
#include "../core/cpu.h"
#include "../core/memory.h"
#include "../core/registers.h"
#include "../video/ppu.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace gbglow {

Debugger::Debugger()
    : cpu_(nullptr)
    , memory_(nullptr)
    , ppu_(nullptr)
    , step_requested_(false)
    , step_over_active_(false)
    , step_over_return_address_(0)
    , max_history_size_(1000)
{
}

void Debugger::attach(CPU* cpu, Memory* memory, PPU* ppu) {
    cpu_ = cpu;
    memory_ = memory;
    ppu_ = ppu;
}

bool Debugger::is_attached() const {
    return cpu_ != nullptr && memory_ != nullptr;
}

// === Breakpoint Management ===

void Debugger::add_breakpoint(u16 address) {
    breakpoints_.insert(address);
}

void Debugger::remove_breakpoint(u16 address) {
    breakpoints_.erase(address);
}

void Debugger::toggle_breakpoint(u16 address) {
    if (has_breakpoint(address)) {
        remove_breakpoint(address);
    } else {
        add_breakpoint(address);
    }
}

bool Debugger::has_breakpoint(u16 address) const {
    return breakpoints_.find(address) != breakpoints_.end();
}

void Debugger::clear_all_breakpoints() {
    breakpoints_.clear();
}

const std::set<u16>& Debugger::get_breakpoints() const {
    return breakpoints_;
}

// === Execution Control ===

bool Debugger::should_break(u16 pc) const {
    return has_breakpoint(pc);
}

void Debugger::request_step() {
    step_requested_ = true;
}

void Debugger::request_step_over() {
    step_requested_ = true;
    step_over_active_ = true;
}

bool Debugger::step_requested() const {
    return step_requested_;
}

void Debugger::clear_step_request() {
    step_requested_ = false;
}

bool Debugger::is_step_over_active() const {
    return step_over_active_;
}

void Debugger::set_step_over_return(u16 address) {
    step_over_return_address_ = address;
}

bool Debugger::should_stop_step_over(u16 pc) const {
    return step_over_active_ && pc == step_over_return_address_;
}

void Debugger::clear_step_over() {
    step_over_active_ = false;
    step_over_return_address_ = 0;
}

// === Memory Inspection ===

u8 Debugger::read_memory(u16 address) const {
    if (!memory_) {
        return 0;
    }
    return memory_->read(address);
}

void Debugger::write_memory(u16 address, u8 value) {
    if (memory_) {
        memory_->write(address, value);
    }
}

std::vector<u8> Debugger::read_memory_region(u16 start, u16 length) const {
    std::vector<u8> data;
    data.reserve(length);
    for (u16 i = 0; i < length; ++i) {
        data.push_back(read_memory(start + i));
    }
    return data;
}

// === Memory Watches ===

void Debugger::add_watch(u16 address, const std::string& label, bool break_on_change) {
    // Check if already watching this address
    auto it = std::find_if(watches_.begin(), watches_.end(),
        [address](const MemoryWatch& w) { return w.address == address; });
    if (it != watches_.end()) {
        it->label = label;
        it->break_on_change = break_on_change;
        return;
    }
    
    MemoryWatch watch;
    watch.address = address;
    watch.label = label.empty() ? format_address(address) : label;
    watch.last_value = read_memory(address);
    watch.break_on_change = break_on_change;
    watches_.push_back(watch);
}

void Debugger::remove_watch(u16 address) {
    watches_.erase(
        std::remove_if(watches_.begin(), watches_.end(),
            [address](const MemoryWatch& w) { return w.address == address; }),
        watches_.end()
    );
}

const std::vector<MemoryWatch>& Debugger::get_watches() const {
    return watches_;
}

bool Debugger::update_watches() {
    bool watch_triggered = false;
    for (auto& watch : watches_) {
        u8 current = read_memory(watch.address);
        if (current != watch.last_value) {
            if (watch.break_on_change) {
                watch_triggered = true;
            }
            watch.last_value = current;
        }
    }
    return watch_triggered;
}

// === Execution History ===

void Debugger::record_execution(u16 pc) {
    execution_history_.push_back(pc);
    while (execution_history_.size() > max_history_size_) {
        execution_history_.pop_front();
    }
}

const std::deque<u16>& Debugger::get_execution_history() const {
    return execution_history_;
}

void Debugger::clear_history() {
    execution_history_.clear();
}

void Debugger::set_max_history(size_t max_size) {
    max_history_size_ = max_size;
    while (execution_history_.size() > max_history_size_) {
        execution_history_.pop_front();
    }
}

// === Register Access ===

const Registers* Debugger::get_registers() const {
    if (!cpu_) {
        return nullptr;
    }
    return &cpu_->registers();
}

u16 Debugger::get_pc() const {
    if (!cpu_) {
        return 0;
    }
    return cpu_->registers().pc;
}

// === Utility ===

std::string Debugger::get_memory_region_name(u16 address) {
    if (address < 0x4000) {
        return "ROM0";
    } else if (address < 0x8000) {
        return "ROM1";
    } else if (address < 0xA000) {
        return "VRAM";
    } else if (address < 0xC000) {
        return "ERAM";
    } else if (address < 0xD000) {
        return "WRAM0";
    } else if (address < 0xE000) {
        return "WRAM1";
    } else if (address < 0xFE00) {
        return "ECHO";
    } else if (address < 0xFEA0) {
        return "OAM";
    } else if (address < 0xFF00) {
        return "----";
    } else if (address < 0xFF80) {
        return "IO";
    } else if (address < 0xFFFF) {
        return "HRAM";
    } else {
        return "IE";
    }
}

std::string Debugger::format_address(u16 address) {
    std::stringstream ss;
    ss << get_memory_region_name(address) << ":$" 
       << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << address;
    return ss.str();
}

// === Disassembly ===

std::string Debugger::get_register_name_8(u8 reg_index) const {
    static const char* names[] = {"B", "C", "D", "E", "H", "L", "(HL)", "A"};
    return names[reg_index & 0x07];
}

std::string Debugger::get_register_name_16(u8 reg_index) const {
    static const char* names[] = {"BC", "DE", "HL", "SP"};
    return names[reg_index & 0x03];
}

std::string Debugger::get_condition_name(u8 cond_index) const {
    static const char* names[] = {"NZ", "Z", "NC", "C"};
    return names[cond_index & 0x03];
}

std::string Debugger::disassemble_cb_opcode(u8 opcode) const {
    std::stringstream ss;
    u8 reg = opcode & 0x07;
    u8 bit = (opcode >> 3) & 0x07;
    u8 op_type = opcode >> 6;
    
    std::string reg_name = get_register_name_8(reg);
    
    switch (op_type) {
        case 0: {
            // Rotates and shifts
            static const char* ops[] = {"RLC", "RRC", "RL", "RR", "SLA", "SRA", "SWAP", "SRL"};
            ss << ops[bit] << " " << reg_name;
            break;
        }
        case 1:
            ss << "BIT " << static_cast<int>(bit) << ", " << reg_name;
            break;
        case 2:
            ss << "RES " << static_cast<int>(bit) << ", " << reg_name;
            break;
        case 3:
            ss << "SET " << static_cast<int>(bit) << ", " << reg_name;
            break;
    }
    
    return ss.str();
}

std::string Debugger::disassemble_opcode(u8 opcode, u16 address, u16& next_address, std::vector<u8>& bytes) const {
    std::stringstream ss;
    bytes.push_back(opcode);
    next_address = address + 1;
    
    auto read_imm8 = [&]() -> u8 {
        u8 val = read_memory(next_address++);
        bytes.push_back(val);
        return val;
    };
    
    auto read_imm16 = [&]() -> u16 {
        u8 lo = read_memory(next_address++);
        u8 hi = read_memory(next_address++);
        bytes.push_back(lo);
        bytes.push_back(hi);
        return (hi << 8) | lo;
    };
    
    auto format_hex8 = [](u8 val) -> std::string {
        std::stringstream s;
        s << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << static_cast<int>(val);
        return s.str();
    };
    
    auto format_hex16 = [](u16 val) -> std::string {
        std::stringstream s;
        s << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << val;
        return s.str();
    };
    
    // Decode instruction
    switch (opcode) {
        // NOP
        case 0x00: ss << "NOP"; break;
        
        // LD rr, nn
        case 0x01: ss << "LD BC, " << format_hex16(read_imm16()); break;
        case 0x11: ss << "LD DE, " << format_hex16(read_imm16()); break;
        case 0x21: ss << "LD HL, " << format_hex16(read_imm16()); break;
        case 0x31: ss << "LD SP, " << format_hex16(read_imm16()); break;
        
        // LD (rr), A
        case 0x02: ss << "LD (BC), A"; break;
        case 0x12: ss << "LD (DE), A"; break;
        case 0x22: ss << "LD (HL+), A"; break;
        case 0x32: ss << "LD (HL-), A"; break;
        
        // INC rr
        case 0x03: ss << "INC BC"; break;
        case 0x13: ss << "INC DE"; break;
        case 0x23: ss << "INC HL"; break;
        case 0x33: ss << "INC SP"; break;
        
        // INC r
        case 0x04: ss << "INC B"; break;
        case 0x0C: ss << "INC C"; break;
        case 0x14: ss << "INC D"; break;
        case 0x1C: ss << "INC E"; break;
        case 0x24: ss << "INC H"; break;
        case 0x2C: ss << "INC L"; break;
        case 0x34: ss << "INC (HL)"; break;
        case 0x3C: ss << "INC A"; break;
        
        // DEC r
        case 0x05: ss << "DEC B"; break;
        case 0x0D: ss << "DEC C"; break;
        case 0x15: ss << "DEC D"; break;
        case 0x1D: ss << "DEC E"; break;
        case 0x25: ss << "DEC H"; break;
        case 0x2D: ss << "DEC L"; break;
        case 0x35: ss << "DEC (HL)"; break;
        case 0x3D: ss << "DEC A"; break;
        
        // LD r, n
        case 0x06: ss << "LD B, " << format_hex8(read_imm8()); break;
        case 0x0E: ss << "LD C, " << format_hex8(read_imm8()); break;
        case 0x16: ss << "LD D, " << format_hex8(read_imm8()); break;
        case 0x1E: ss << "LD E, " << format_hex8(read_imm8()); break;
        case 0x26: ss << "LD H, " << format_hex8(read_imm8()); break;
        case 0x2E: ss << "LD L, " << format_hex8(read_imm8()); break;
        case 0x36: ss << "LD (HL), " << format_hex8(read_imm8()); break;
        case 0x3E: ss << "LD A, " << format_hex8(read_imm8()); break;
        
        // Rotates on A
        case 0x07: ss << "RLCA"; break;
        case 0x0F: ss << "RRCA"; break;
        case 0x17: ss << "RLA"; break;
        case 0x1F: ss << "RRA"; break;
        
        // Misc
        case 0x08: ss << "LD (" << format_hex16(read_imm16()) << "), SP"; break;
        case 0x09: ss << "ADD HL, BC"; break;
        case 0x19: ss << "ADD HL, DE"; break;
        case 0x29: ss << "ADD HL, HL"; break;
        case 0x39: ss << "ADD HL, SP"; break;
        
        // LD A, (rr)
        case 0x0A: ss << "LD A, (BC)"; break;
        case 0x1A: ss << "LD A, (DE)"; break;
        case 0x2A: ss << "LD A, (HL+)"; break;
        case 0x3A: ss << "LD A, (HL-)"; break;
        
        // DEC rr
        case 0x0B: ss << "DEC BC"; break;
        case 0x1B: ss << "DEC DE"; break;
        case 0x2B: ss << "DEC HL"; break;
        case 0x3B: ss << "DEC SP"; break;
        
        // Special
        case 0x10: ss << "STOP"; read_imm8(); break;
        case 0x18: {
            i8 offset = static_cast<i8>(read_imm8());
            u16 target = next_address + offset;
            ss << "JR " << format_hex16(target);
            break;
        }
        case 0x20: {
            i8 offset = static_cast<i8>(read_imm8());
            u16 target = next_address + offset;
            ss << "JR NZ, " << format_hex16(target);
            break;
        }
        case 0x28: {
            i8 offset = static_cast<i8>(read_imm8());
            u16 target = next_address + offset;
            ss << "JR Z, " << format_hex16(target);
            break;
        }
        case 0x30: {
            i8 offset = static_cast<i8>(read_imm8());
            u16 target = next_address + offset;
            ss << "JR NC, " << format_hex16(target);
            break;
        }
        case 0x38: {
            i8 offset = static_cast<i8>(read_imm8());
            u16 target = next_address + offset;
            ss << "JR C, " << format_hex16(target);
            break;
        }
        
        case 0x27: ss << "DAA"; break;
        case 0x2F: ss << "CPL"; break;
        case 0x37: ss << "SCF"; break;
        case 0x3F: ss << "CCF"; break;
        
        // LD r, r (0x40-0x7F except HALT)
        case 0x76: ss << "HALT"; break;
        
        // ALU A, r
        case 0x80: ss << "ADD A, B"; break;
        case 0x81: ss << "ADD A, C"; break;
        case 0x82: ss << "ADD A, D"; break;
        case 0x83: ss << "ADD A, E"; break;
        case 0x84: ss << "ADD A, H"; break;
        case 0x85: ss << "ADD A, L"; break;
        case 0x86: ss << "ADD A, (HL)"; break;
        case 0x87: ss << "ADD A, A"; break;
        
        case 0x88: ss << "ADC A, B"; break;
        case 0x89: ss << "ADC A, C"; break;
        case 0x8A: ss << "ADC A, D"; break;
        case 0x8B: ss << "ADC A, E"; break;
        case 0x8C: ss << "ADC A, H"; break;
        case 0x8D: ss << "ADC A, L"; break;
        case 0x8E: ss << "ADC A, (HL)"; break;
        case 0x8F: ss << "ADC A, A"; break;
        
        case 0x90: ss << "SUB B"; break;
        case 0x91: ss << "SUB C"; break;
        case 0x92: ss << "SUB D"; break;
        case 0x93: ss << "SUB E"; break;
        case 0x94: ss << "SUB H"; break;
        case 0x95: ss << "SUB L"; break;
        case 0x96: ss << "SUB (HL)"; break;
        case 0x97: ss << "SUB A"; break;
        
        case 0x98: ss << "SBC A, B"; break;
        case 0x99: ss << "SBC A, C"; break;
        case 0x9A: ss << "SBC A, D"; break;
        case 0x9B: ss << "SBC A, E"; break;
        case 0x9C: ss << "SBC A, H"; break;
        case 0x9D: ss << "SBC A, L"; break;
        case 0x9E: ss << "SBC A, (HL)"; break;
        case 0x9F: ss << "SBC A, A"; break;
        
        case 0xA0: ss << "AND B"; break;
        case 0xA1: ss << "AND C"; break;
        case 0xA2: ss << "AND D"; break;
        case 0xA3: ss << "AND E"; break;
        case 0xA4: ss << "AND H"; break;
        case 0xA5: ss << "AND L"; break;
        case 0xA6: ss << "AND (HL)"; break;
        case 0xA7: ss << "AND A"; break;
        
        case 0xA8: ss << "XOR B"; break;
        case 0xA9: ss << "XOR C"; break;
        case 0xAA: ss << "XOR D"; break;
        case 0xAB: ss << "XOR E"; break;
        case 0xAC: ss << "XOR H"; break;
        case 0xAD: ss << "XOR L"; break;
        case 0xAE: ss << "XOR (HL)"; break;
        case 0xAF: ss << "XOR A"; break;
        
        case 0xB0: ss << "OR B"; break;
        case 0xB1: ss << "OR C"; break;
        case 0xB2: ss << "OR D"; break;
        case 0xB3: ss << "OR E"; break;
        case 0xB4: ss << "OR H"; break;
        case 0xB5: ss << "OR L"; break;
        case 0xB6: ss << "OR (HL)"; break;
        case 0xB7: ss << "OR A"; break;
        
        case 0xB8: ss << "CP B"; break;
        case 0xB9: ss << "CP C"; break;
        case 0xBA: ss << "CP D"; break;
        case 0xBB: ss << "CP E"; break;
        case 0xBC: ss << "CP H"; break;
        case 0xBD: ss << "CP L"; break;
        case 0xBE: ss << "CP (HL)"; break;
        case 0xBF: ss << "CP A"; break;
        
        // RET cc
        case 0xC0: ss << "RET NZ"; break;
        case 0xC8: ss << "RET Z"; break;
        case 0xD0: ss << "RET NC"; break;
        case 0xD8: ss << "RET C"; break;
        
        // POP/PUSH
        case 0xC1: ss << "POP BC"; break;
        case 0xD1: ss << "POP DE"; break;
        case 0xE1: ss << "POP HL"; break;
        case 0xF1: ss << "POP AF"; break;
        
        case 0xC5: ss << "PUSH BC"; break;
        case 0xD5: ss << "PUSH DE"; break;
        case 0xE5: ss << "PUSH HL"; break;
        case 0xF5: ss << "PUSH AF"; break;
        
        // JP cc, nn
        case 0xC2: ss << "JP NZ, " << format_hex16(read_imm16()); break;
        case 0xCA: ss << "JP Z, " << format_hex16(read_imm16()); break;
        case 0xD2: ss << "JP NC, " << format_hex16(read_imm16()); break;
        case 0xDA: ss << "JP C, " << format_hex16(read_imm16()); break;
        case 0xC3: ss << "JP " << format_hex16(read_imm16()); break;
        case 0xE9: ss << "JP HL"; break;
        
        // CALL cc, nn
        case 0xC4: ss << "CALL NZ, " << format_hex16(read_imm16()); break;
        case 0xCC: ss << "CALL Z, " << format_hex16(read_imm16()); break;
        case 0xD4: ss << "CALL NC, " << format_hex16(read_imm16()); break;
        case 0xDC: ss << "CALL C, " << format_hex16(read_imm16()); break;
        case 0xCD: ss << "CALL " << format_hex16(read_imm16()); break;
        
        // ALU A, n
        case 0xC6: ss << "ADD A, " << format_hex8(read_imm8()); break;
        case 0xCE: ss << "ADC A, " << format_hex8(read_imm8()); break;
        case 0xD6: ss << "SUB " << format_hex8(read_imm8()); break;
        case 0xDE: ss << "SBC A, " << format_hex8(read_imm8()); break;
        case 0xE6: ss << "AND " << format_hex8(read_imm8()); break;
        case 0xEE: ss << "XOR " << format_hex8(read_imm8()); break;
        case 0xF6: ss << "OR " << format_hex8(read_imm8()); break;
        case 0xFE: ss << "CP " << format_hex8(read_imm8()); break;
        
        // RST
        case 0xC7: ss << "RST $00"; break;
        case 0xCF: ss << "RST $08"; break;
        case 0xD7: ss << "RST $10"; break;
        case 0xDF: ss << "RST $18"; break;
        case 0xE7: ss << "RST $20"; break;
        case 0xEF: ss << "RST $28"; break;
        case 0xF7: ss << "RST $30"; break;
        case 0xFF: ss << "RST $38"; break;
        
        // RET/RETI
        case 0xC9: ss << "RET"; break;
        case 0xD9: ss << "RETI"; break;
        
        // LDH
        case 0xE0: ss << "LDH (" << format_hex8(read_imm8()) << "), A"; break;
        case 0xF0: ss << "LDH A, (" << format_hex8(read_imm8()) << ")"; break;
        case 0xE2: ss << "LDH (C), A"; break;
        case 0xF2: ss << "LDH A, (C)"; break;
        
        // LD (nn), A / LD A, (nn)
        case 0xEA: ss << "LD (" << format_hex16(read_imm16()) << "), A"; break;
        case 0xFA: ss << "LD A, (" << format_hex16(read_imm16()) << ")"; break;
        
        // Misc
        case 0xE8: ss << "ADD SP, " << format_hex8(read_imm8()); break;
        case 0xF8: ss << "LD HL, SP+" << format_hex8(read_imm8()); break;
        case 0xF9: ss << "LD SP, HL"; break;
        
        // DI/EI
        case 0xF3: ss << "DI"; break;
        case 0xFB: ss << "EI"; break;
        
        // CB prefix
        case 0xCB: {
            u8 cb_op = read_imm8();
            ss << disassemble_cb_opcode(cb_op);
            break;
        }
        
        // Invalid/undefined opcodes
        case 0xD3: case 0xDB: case 0xDD: case 0xE3: case 0xE4:
        case 0xEB: case 0xEC: case 0xED: case 0xF4: case 0xFC: case 0xFD:
            ss << "DB " << format_hex8(opcode);
            break;
        
        default:
            // LD r, r instructions (0x40-0x7F except 0x76)
            if (opcode >= 0x40 && opcode < 0x80 && opcode != 0x76) {
                u8 dst = (opcode >> 3) & 0x07;
                u8 src = opcode & 0x07;
                ss << "LD " << get_register_name_8(dst) << ", " << get_register_name_8(src);
            } else {
                ss << "DB " << format_hex8(opcode);
            }
            break;
    }
    
    return ss.str();
}

DisassembledInstruction Debugger::disassemble_at(u16 address, u16& next_address) const {
    DisassembledInstruction result;
    result.address = address;
    result.is_breakpoint = has_breakpoint(address);
    
    u8 opcode = read_memory(address);
    std::string full_disasm = disassemble_opcode(opcode, address, next_address, result.bytes);
    
    // Split mnemonic and operands
    size_t space_pos = full_disasm.find(' ');
    if (space_pos != std::string::npos) {
        result.mnemonic = full_disasm.substr(0, space_pos);
        result.operands = full_disasm.substr(space_pos + 1);
    } else {
        result.mnemonic = full_disasm;
        result.operands = "";
    }
    
    return result;
}

std::vector<DisassembledInstruction> Debugger::disassemble_range(u16 start, int count) const {
    std::vector<DisassembledInstruction> results;
    results.reserve(count);
    
    u16 address = start;
    for (int i = 0; i < count && address < 0xFFFF; ++i) {
        u16 next;
        results.push_back(disassemble_at(address, next));
        address = next;
    }
    
    return results;
}

std::vector<DisassembledInstruction> Debugger::disassemble_around_pc(int lines_before, int lines_after) const {
    if (!cpu_) {
        return {};
    }
    
    u16 pc = cpu_->registers().pc;
    
    // For lines before PC, we need to scan backwards which is tricky
    // since Game Boy instructions are variable length (1-3 bytes)
    // We'll scan from a safe distance and keep track
    std::vector<DisassembledInstruction> all_instructions;
    
    // Start scanning from some bytes before PC
    u16 scan_start = (pc > 64) ? pc - 64 : 0;
    u16 address = scan_start;
    
    while (address < pc + 32 && address < 0xFFFF) {
        u16 next;
        all_instructions.push_back(disassemble_at(address, next));
        address = next;
    }
    
    // Find PC in our disassembly
    int pc_index = -1;
    for (size_t i = 0; i < all_instructions.size(); ++i) {
        if (all_instructions[i].address == pc) {
            pc_index = static_cast<int>(i);
            break;
        }
    }
    
    // Extract the window around PC
    std::vector<DisassembledInstruction> result;
    if (pc_index >= 0) {
        int start_idx = std::max(0, pc_index - lines_before);
        int end_idx = std::min(static_cast<int>(all_instructions.size()), pc_index + lines_after + 1);
        
        for (int i = start_idx; i < end_idx; ++i) {
            result.push_back(all_instructions[i]);
        }
    }
    
    return result;
}

} // namespace gbglow
