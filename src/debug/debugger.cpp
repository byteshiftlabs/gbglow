// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 gbglow Contributors
// This file is part of gbglow. See LICENSE for details.

#include "debugger.h"
#include "../core/cpu.h"
#include "../core/memory.h"
#include "../core/registers.h"
#include "../video/ppu.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace gbglow {

struct Debugger::ExecutionServices {
    std::set<u16> breakpoints;
    bool step_requested = false;
    bool step_over_active = false;
    u16 step_over_return_address = 0;
    bool skip_breakpoint_once = false;
    u16 skipped_breakpoint_pc = 0;

    bool has_breakpoint(u16 address) const {
        return breakpoints.find(address) != breakpoints.end();
    }

    bool should_break(u16 pc) {
        if (skip_breakpoint_once && pc == skipped_breakpoint_pc) {
            skip_breakpoint_once = false;
            return false;
        }

        return has_breakpoint(pc);
    }

    void skip_breakpoint_once_at(u16 pc) {
        skip_breakpoint_once = true;
        skipped_breakpoint_pc = pc;
    }

    void request_step() {
        step_requested = true;
    }

    void request_step_over() {
        step_requested = true;
        step_over_active = true;
    }

    void clear_step_request() {
        step_requested = false;
    }

    void set_step_over_return(u16 address) {
        step_over_return_address = address;
    }

    bool should_stop_step_over(u16 pc) const {
        return step_over_active && pc == step_over_return_address;
    }

    void clear_step_over() {
        step_over_active = false;
        step_over_return_address = 0;
        step_requested = false;
    }
};

struct Debugger::InspectionServices {
    std::vector<MemoryWatch> watches;
    std::deque<u16> execution_history;
    size_t max_history_size = 1000;

    void add_or_update_watch(u16 address, const std::string& label, bool break_on_change, u8 initial_value) {
        auto it = std::find_if(watches.begin(), watches.end(),
            [address](const MemoryWatch& watch) { return watch.address == address; });
        if (it != watches.end()) {
            it->label = label;
            it->break_on_change = break_on_change;
            return;
        }

        MemoryWatch watch;
        watch.address = address;
        watch.label = label;
        watch.last_value = initial_value;
        watch.break_on_change = break_on_change;
        watches.push_back(watch);
    }

    void remove_watch(u16 address) {
        watches.erase(
            std::remove_if(watches.begin(), watches.end(),
                [address](const MemoryWatch& watch) { return watch.address == address; }),
            watches.end());
    }

    bool update_watches(const std::function<u8(u16)>& read_memory) {
        bool watch_triggered = false;
        for (auto& watch : watches) {
            const u8 current = read_memory(watch.address);
            if (current != watch.last_value) {
                if (watch.break_on_change) {
                    watch_triggered = true;
                }
                watch.last_value = current;
            }
        }

        return watch_triggered;
    }

    void record_execution(u16 pc) {
        execution_history.push_back(pc);
        while (execution_history.size() > max_history_size) {
            execution_history.pop_front();
        }
    }

    void clear_history() {
        execution_history.clear();
    }

    void set_max_history(size_t max_size) {
        max_history_size = max_size;
        while (execution_history.size() > max_history_size) {
            execution_history.pop_front();
        }
    }
};

namespace {

bool is_step_over_opcode(u8 opcode) {
    switch (opcode) {
        case 0xC4:
        case 0xCC:
        case 0xCD:
        case 0xD4:
        case 0xDC:
        case 0xC7:
        case 0xCF:
        case 0xD7:
        case 0xDF:
        case 0xE7:
        case 0xEF:
        case 0xF7:
        case 0xFF:
            return true;
        default:
            return false;
    }
}

class DebuggerDisassembler final {
public:
    static std::string get_register_name_8(u8 reg_index) {
        static const char* names[] = {"B", "C", "D", "E", "H", "L", "(HL)", "A"};
        return names[reg_index & 0x07];
    }

    static std::string disassemble_cb_opcode(u8 opcode) {
        std::stringstream ss;
        const u8 reg = opcode & 0x07;
        const u8 bit = (opcode >> 3) & 0x07;
        const u8 op_type = opcode >> 6;
        const std::string reg_name = get_register_name_8(reg);

        switch (op_type) {
            case 0: {
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

    static std::string disassemble_opcode(const Debugger& debugger, u8 opcode, u16 address, u16& next_address, std::vector<u8>& bytes) {
        std::stringstream ss;
        bytes.push_back(opcode);
        next_address = address + 1;

        auto read_imm8 = [&]() -> u8 {
            const u8 val = debugger.read_memory(next_address++);
            bytes.push_back(val);
            return val;
        };

        auto read_imm16 = [&]() -> u16 {
            const u8 lo = debugger.read_memory(next_address++);
            const u8 hi = debugger.read_memory(next_address++);
            bytes.push_back(lo);
            bytes.push_back(hi);
            return static_cast<u16>((hi << 8) | lo);
        };

        auto format_hex8 = [](u8 val) -> std::string {
            std::stringstream stream;
            stream << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << static_cast<int>(val);
            return stream.str();
        };

        auto format_hex16 = [](u16 val) -> std::string {
            std::stringstream stream;
            stream << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << val;
            return stream.str();
        };

        switch (opcode) {
            case 0x00: ss << "NOP"; break;

            case 0x01: ss << "LD BC, " << format_hex16(read_imm16()); break;
            case 0x11: ss << "LD DE, " << format_hex16(read_imm16()); break;
            case 0x21: ss << "LD HL, " << format_hex16(read_imm16()); break;
            case 0x31: ss << "LD SP, " << format_hex16(read_imm16()); break;

            case 0x02: ss << "LD (BC), A"; break;
            case 0x12: ss << "LD (DE), A"; break;
            case 0x22: ss << "LD (HL+), A"; break;
            case 0x32: ss << "LD (HL-), A"; break;

            case 0x03: ss << "INC BC"; break;
            case 0x13: ss << "INC DE"; break;
            case 0x23: ss << "INC HL"; break;
            case 0x33: ss << "INC SP"; break;

            case 0x04: ss << "INC B"; break;
            case 0x0C: ss << "INC C"; break;
            case 0x14: ss << "INC D"; break;
            case 0x1C: ss << "INC E"; break;
            case 0x24: ss << "INC H"; break;
            case 0x2C: ss << "INC L"; break;
            case 0x34: ss << "INC (HL)"; break;
            case 0x3C: ss << "INC A"; break;

            case 0x05: ss << "DEC B"; break;
            case 0x0D: ss << "DEC C"; break;
            case 0x15: ss << "DEC D"; break;
            case 0x1D: ss << "DEC E"; break;
            case 0x25: ss << "DEC H"; break;
            case 0x2D: ss << "DEC L"; break;
            case 0x35: ss << "DEC (HL)"; break;
            case 0x3D: ss << "DEC A"; break;

            case 0x06: ss << "LD B, " << format_hex8(read_imm8()); break;
            case 0x0E: ss << "LD C, " << format_hex8(read_imm8()); break;
            case 0x16: ss << "LD D, " << format_hex8(read_imm8()); break;
            case 0x1E: ss << "LD E, " << format_hex8(read_imm8()); break;
            case 0x26: ss << "LD H, " << format_hex8(read_imm8()); break;
            case 0x2E: ss << "LD L, " << format_hex8(read_imm8()); break;
            case 0x36: ss << "LD (HL), " << format_hex8(read_imm8()); break;
            case 0x3E: ss << "LD A, " << format_hex8(read_imm8()); break;

            case 0x07: ss << "RLCA"; break;
            case 0x0F: ss << "RRCA"; break;
            case 0x17: ss << "RLA"; break;
            case 0x1F: ss << "RRA"; break;

            case 0x08: ss << "LD (" << format_hex16(read_imm16()) << "), SP"; break;
            case 0x09: ss << "ADD HL, BC"; break;
            case 0x19: ss << "ADD HL, DE"; break;
            case 0x29: ss << "ADD HL, HL"; break;
            case 0x39: ss << "ADD HL, SP"; break;

            case 0x0A: ss << "LD A, (BC)"; break;
            case 0x1A: ss << "LD A, (DE)"; break;
            case 0x2A: ss << "LD A, (HL+)"; break;
            case 0x3A: ss << "LD A, (HL-)"; break;

            case 0x0B: ss << "DEC BC"; break;
            case 0x1B: ss << "DEC DE"; break;
            case 0x2B: ss << "DEC HL"; break;
            case 0x3B: ss << "DEC SP"; break;

            case 0x10: ss << "STOP"; read_imm8(); break;
            case 0x18: {
                const i8 offset = static_cast<i8>(read_imm8());
                const u16 target = next_address + offset;
                ss << "JR " << format_hex16(target);
                break;
            }
            case 0x20: {
                const i8 offset = static_cast<i8>(read_imm8());
                const u16 target = next_address + offset;
                ss << "JR NZ, " << format_hex16(target);
                break;
            }
            case 0x28: {
                const i8 offset = static_cast<i8>(read_imm8());
                const u16 target = next_address + offset;
                ss << "JR Z, " << format_hex16(target);
                break;
            }
            case 0x30: {
                const i8 offset = static_cast<i8>(read_imm8());
                const u16 target = next_address + offset;
                ss << "JR NC, " << format_hex16(target);
                break;
            }
            case 0x38: {
                const i8 offset = static_cast<i8>(read_imm8());
                const u16 target = next_address + offset;
                ss << "JR C, " << format_hex16(target);
                break;
            }

            case 0x27: ss << "DAA"; break;
            case 0x2F: ss << "CPL"; break;
            case 0x37: ss << "SCF"; break;
            case 0x3F: ss << "CCF"; break;

            case 0x76: ss << "HALT"; break;

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

            case 0xC0: ss << "RET NZ"; break;
            case 0xC8: ss << "RET Z"; break;
            case 0xD0: ss << "RET NC"; break;
            case 0xD8: ss << "RET C"; break;

            case 0xC1: ss << "POP BC"; break;
            case 0xD1: ss << "POP DE"; break;
            case 0xE1: ss << "POP HL"; break;
            case 0xF1: ss << "POP AF"; break;

            case 0xC5: ss << "PUSH BC"; break;
            case 0xD5: ss << "PUSH DE"; break;
            case 0xE5: ss << "PUSH HL"; break;
            case 0xF5: ss << "PUSH AF"; break;

            case 0xC2: ss << "JP NZ, " << format_hex16(read_imm16()); break;
            case 0xCA: ss << "JP Z, " << format_hex16(read_imm16()); break;
            case 0xD2: ss << "JP NC, " << format_hex16(read_imm16()); break;
            case 0xDA: ss << "JP C, " << format_hex16(read_imm16()); break;
            case 0xC3: ss << "JP " << format_hex16(read_imm16()); break;
            case 0xE9: ss << "JP HL"; break;

            case 0xC4: ss << "CALL NZ, " << format_hex16(read_imm16()); break;
            case 0xCC: ss << "CALL Z, " << format_hex16(read_imm16()); break;
            case 0xD4: ss << "CALL NC, " << format_hex16(read_imm16()); break;
            case 0xDC: ss << "CALL C, " << format_hex16(read_imm16()); break;
            case 0xCD: ss << "CALL " << format_hex16(read_imm16()); break;

            case 0xC6: ss << "ADD A, " << format_hex8(read_imm8()); break;
            case 0xCE: ss << "ADC A, " << format_hex8(read_imm8()); break;
            case 0xD6: ss << "SUB " << format_hex8(read_imm8()); break;
            case 0xDE: ss << "SBC A, " << format_hex8(read_imm8()); break;
            case 0xE6: ss << "AND " << format_hex8(read_imm8()); break;
            case 0xEE: ss << "XOR " << format_hex8(read_imm8()); break;
            case 0xF6: ss << "OR " << format_hex8(read_imm8()); break;
            case 0xFE: ss << "CP " << format_hex8(read_imm8()); break;

            case 0xC7: ss << "RST $00"; break;
            case 0xCF: ss << "RST $08"; break;
            case 0xD7: ss << "RST $10"; break;
            case 0xDF: ss << "RST $18"; break;
            case 0xE7: ss << "RST $20"; break;
            case 0xEF: ss << "RST $28"; break;
            case 0xF7: ss << "RST $30"; break;
            case 0xFF: ss << "RST $38"; break;

            case 0xC9: ss << "RET"; break;
            case 0xD9: ss << "RETI"; break;

            case 0xE0: ss << "LDH (" << format_hex8(read_imm8()) << "), A"; break;
            case 0xF0: ss << "LDH A, (" << format_hex8(read_imm8()) << ")"; break;
            case 0xE2: ss << "LDH (C), A"; break;
            case 0xF2: ss << "LDH A, (C)"; break;

            case 0xEA: ss << "LD (" << format_hex16(read_imm16()) << "), A"; break;
            case 0xFA: ss << "LD A, (" << format_hex16(read_imm16()) << ")"; break;

            case 0xE8: ss << "ADD SP, " << format_hex8(read_imm8()); break;
            case 0xF8: ss << "LD HL, SP+" << format_hex8(read_imm8()); break;
            case 0xF9: ss << "LD SP, HL"; break;

            case 0xF3: ss << "DI"; break;
            case 0xFB: ss << "EI"; break;

            case 0xCB: {
                const u8 cb_op = read_imm8();
                ss << disassemble_cb_opcode(cb_op);
                break;
            }

            case 0xD3: case 0xDB: case 0xDD: case 0xE3: case 0xE4:
            case 0xEB: case 0xEC: case 0xED: case 0xF4: case 0xFC: case 0xFD:
                ss << "DB " << format_hex8(opcode);
                break;

            default:
                if (opcode >= 0x40 && opcode < 0x80 && opcode != 0x76) {
                    const u8 dst = (opcode >> 3) & 0x07;
                    const u8 src = opcode & 0x07;
                    ss << "LD " << get_register_name_8(dst) << ", " << get_register_name_8(src);
                } else {
                    ss << "DB " << format_hex8(opcode);
                }
                break;
        }

        return ss.str();
    }
};

}  // namespace

Debugger::Debugger()
    : cpu_(nullptr)
    , memory_(nullptr)
    , ppu_(nullptr)
    , execution_(std::make_unique<ExecutionServices>())
    , inspection_(std::make_unique<InspectionServices>())
{
}

Debugger::~Debugger() = default;

void Debugger::attach(CPU& cpu, Memory& memory, PPU& ppu) {
    cpu_ = &cpu;
    memory_ = &memory;
    ppu_ = &ppu;
}

bool Debugger::is_attached() const {
    return cpu_ != nullptr && memory_ != nullptr;
}

void Debugger::add_breakpoint(u16 address) {
    execution_->breakpoints.insert(address);
}

void Debugger::remove_breakpoint(u16 address) {
    execution_->breakpoints.erase(address);
}

void Debugger::toggle_breakpoint(u16 address) {
    if (has_breakpoint(address)) {
        remove_breakpoint(address);
    } else {
        add_breakpoint(address);
    }
}

bool Debugger::has_breakpoint(u16 address) const {
    return execution_->has_breakpoint(address);
}

void Debugger::clear_all_breakpoints() {
    execution_->breakpoints.clear();
}

const std::set<u16>& Debugger::get_breakpoints() const {
    return execution_->breakpoints;
}

bool Debugger::should_break(u16 pc) {
    return execution_->should_break(pc);
}

void Debugger::skip_breakpoint_once(u16 pc) {
    execution_->skip_breakpoint_once_at(pc);
}

void Debugger::request_step() {
    execution_->request_step();
}

void Debugger::request_step_over() {
    execution_->request_step_over();
}

bool Debugger::prepare_step_over_for_current_instruction() {
    if (!cpu_ || !memory_) {
        return false;
    }

    const u16 current_pc = cpu_->registers().pc;
    const u8 opcode = read_memory(current_pc);
    if (!is_step_over_opcode(opcode)) {
        return false;
    }

    u16 next_address = current_pc;
    disassemble_at(current_pc, next_address);
    request_step_over();
    set_step_over_return(next_address);
    return true;
}

bool Debugger::step_requested() const {
    return execution_->step_requested;
}

void Debugger::clear_step_request() {
    execution_->clear_step_request();
}

bool Debugger::is_step_over_active() const {
    return execution_->step_over_active;
}

void Debugger::set_step_over_return(u16 address) {
    execution_->set_step_over_return(address);
}

bool Debugger::should_stop_step_over(u16 pc) const {
    return execution_->should_stop_step_over(pc);
}

void Debugger::clear_step_over() {
    execution_->clear_step_over();
}

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

void Debugger::add_watch(u16 address, const std::string& label, bool break_on_change) {
    const std::string watch_label = label.empty() ? format_address(address) : label;
    inspection_->add_or_update_watch(address, watch_label, break_on_change, read_memory(address));
}

void Debugger::remove_watch(u16 address) {
    inspection_->remove_watch(address);
}

const std::vector<MemoryWatch>& Debugger::get_watches() const {
    return inspection_->watches;
}

bool Debugger::update_watches() {
    return inspection_->update_watches([this](u16 address) { return read_memory(address); });
}

void Debugger::record_execution(u16 pc) {
    inspection_->record_execution(pc);
}

const std::deque<u16>& Debugger::get_execution_history() const {
    return inspection_->execution_history;
}

void Debugger::clear_history() {
    inspection_->clear_history();
}

void Debugger::set_max_history(size_t max_size) {
    inspection_->set_max_history(max_size);
}

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
    }

    return "IE";
}

std::string Debugger::format_address(u16 address) {
    std::stringstream ss;
    ss << get_memory_region_name(address) << ":$"
       << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << address;
    return ss.str();
}

DisassembledInstruction Debugger::disassemble_at(u16 address, u16& next_address) const {
    DisassembledInstruction result;
    result.address = address;
    result.is_breakpoint = has_breakpoint(address);

    const u8 opcode = read_memory(address);
    const std::string full_disasm = DebuggerDisassembler::disassemble_opcode(*this, opcode, address, next_address, result.bytes);

    const size_t space_pos = full_disasm.find(' ');
    if (space_pos != std::string::npos) {
        result.mnemonic = full_disasm.substr(0, space_pos);
        result.operands = full_disasm.substr(space_pos + 1);
    } else {
        result.mnemonic = full_disasm;
        result.operands.clear();
    }

    return result;
}

std::vector<DisassembledInstruction> Debugger::disassemble_range(u16 start, int count) const {
    std::vector<DisassembledInstruction> results;
    results.reserve(count);

    u16 address = start;
    for (int i = 0; i < count && address < 0xFFFF; ++i) {
        u16 next = address;
        results.push_back(disassemble_at(address, next));
        address = next;
    }

    return results;
}

std::vector<DisassembledInstruction> Debugger::disassemble_around_pc(int lines_before, int lines_after) const {
    if (!cpu_) {
        return {};
    }

    const u16 pc = cpu_->registers().pc;
    std::vector<DisassembledInstruction> all_instructions;

    const u16 scan_start = (pc > 64) ? static_cast<u16>(pc - 64) : 0;
    u16 address = scan_start;

    while (address < pc + 32 && address < 0xFFFF) {
        u16 next = address;
        all_instructions.push_back(disassemble_at(address, next));
        address = next;
    }

    int pc_index = -1;
    const auto pc_it = std::find_if(
        all_instructions.begin(),
        all_instructions.end(),
        [pc](const DisassembledInstruction& instruction) {
            return instruction.address == pc;
        }
    );
    if (pc_it != all_instructions.end()) {
        pc_index = static_cast<int>(std::distance(all_instructions.begin(), pc_it));
    }

    std::vector<DisassembledInstruction> result;
    if (pc_index >= 0) {
        const int start_idx = std::max(0, pc_index - lines_before);
        const int end_idx = std::min(static_cast<int>(all_instructions.size()), pc_index + lines_after + 1);
        for (int index = start_idx; index < end_idx; ++index) {
            result.push_back(all_instructions[index]);
        }
    }

    return result;
}

}  // namespace gbglow
