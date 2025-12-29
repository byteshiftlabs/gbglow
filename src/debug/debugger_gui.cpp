#include "debugger_gui.h"
#include "debugger.h"
#include "../core/registers.h"
#include <imgui.h>
#include <sstream>
#include <iomanip>
#include <cstring>

namespace gbcrush {

DebuggerGUI::DebuggerGUI()
    : debugger_(nullptr)
    , visible_(false)
    , paused_(false)
    , step_requested_(false)
    , continue_requested_(false)
    , show_registers_(true)
    , show_disassembly_(true)
    , show_memory_(true)
    , show_breakpoints_(true)
    , show_watches_(false)
    , show_stack_(false)
    , show_io_registers_(false)
    , memory_view_address_(0x0000)
    , memory_view_columns_(16)
    , follow_pc_(true)
    , disasm_address_(0x0000)
    , watch_break_on_change_(false)
{
    std::memset(memory_goto_address_, 0, sizeof(memory_goto_address_));
    std::memset(disasm_goto_address_, 0, sizeof(disasm_goto_address_));
    std::memset(breakpoint_address_, 0, sizeof(breakpoint_address_));
    std::memset(watch_address_, 0, sizeof(watch_address_));
    std::memset(watch_label_, 0, sizeof(watch_label_));
}

void DebuggerGUI::attach(Debugger* debugger) {
    debugger_ = debugger;
}

void DebuggerGUI::toggle_visible() {
    visible_ = !visible_;
    if (visible_) {
        paused_ = true;  // Auto-pause when opening debugger
    }
}

bool DebuggerGUI::is_visible() const {
    return visible_;
}

void DebuggerGUI::set_visible(bool visible) {
    visible_ = visible;
    if (visible) {
        paused_ = true;
    }
}

bool DebuggerGUI::should_pause() const {
    return visible_ && paused_;
}

bool DebuggerGUI::step_requested() const {
    return step_requested_;
}

void DebuggerGUI::clear_step_request() {
    step_requested_ = false;
}

void DebuggerGUI::continue_execution() {
    continue_requested_ = true;
    paused_ = false;
}

bool DebuggerGUI::should_continue() const {
    return continue_requested_;
}

void DebuggerGUI::clear_continue() {
    continue_requested_ = false;
}

void DebuggerGUI::render() {
    if (!visible_ || !debugger_ || !debugger_->is_attached()) {
        return;
    }
    
    render_menu_bar();
    
    if (show_registers_) {
        render_registers_window();
    }
    if (show_disassembly_) {
        render_disassembly_window();
    }
    if (show_memory_) {
        render_memory_window();
    }
    if (show_breakpoints_) {
        render_breakpoints_window();
    }
    if (show_watches_) {
        render_watches_window();
    }
    if (show_stack_) {
        render_stack_window();
    }
    if (show_io_registers_) {
        render_io_registers_window();
    }
}

void DebuggerGUI::render_menu_bar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Debug")) {
            if (ImGui::MenuItem("Registers", nullptr, show_registers_)) {
                show_registers_ = !show_registers_;
            }
            if (ImGui::MenuItem("Disassembly", nullptr, show_disassembly_)) {
                show_disassembly_ = !show_disassembly_;
            }
            if (ImGui::MenuItem("Memory", nullptr, show_memory_)) {
                show_memory_ = !show_memory_;
            }
            if (ImGui::MenuItem("Breakpoints", nullptr, show_breakpoints_)) {
                show_breakpoints_ = !show_breakpoints_;
            }
            if (ImGui::MenuItem("Watches", nullptr, show_watches_)) {
                show_watches_ = !show_watches_;
            }
            if (ImGui::MenuItem("Stack", nullptr, show_stack_)) {
                show_stack_ = !show_stack_;
            }
            if (ImGui::MenuItem("I/O Registers", nullptr, show_io_registers_)) {
                show_io_registers_ = !show_io_registers_;
            }
            
            ImGui::Separator();
            
            if (ImGui::MenuItem("Close Debugger", "F12")) {
                visible_ = false;
                paused_ = false;
            }
            
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

std::string DebuggerGUI::format_flags(u8 f) const {
    std::string result;
    result += (f & 0x80) ? 'Z' : '-';
    result += (f & 0x40) ? 'N' : '-';
    result += (f & 0x20) ? 'H' : '-';
    result += (f & 0x10) ? 'C' : '-';
    return result;
}

void DebuggerGUI::render_register_row(const char* name, u16 value, bool highlight) {
    if (highlight) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
    }
    ImGui::Text("%s: %04X", name, value);
    if (highlight) {
        ImGui::PopStyleColor();
    }
}

void DebuggerGUI::render_register_row_8(const char* name, u8 value, bool highlight) {
    if (highlight) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
    }
    ImGui::Text("%s: %02X", name, value);
    if (highlight) {
        ImGui::PopStyleColor();
    }
}

void DebuggerGUI::render_registers_window() {
    ImGui::SetNextWindowSize(ImVec2(200, 300), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("CPU Registers", &show_registers_)) {
        const Registers* regs = debugger_->get_registers();
        
        if (regs) {
            // Execution controls
            if (paused_) {
                if (ImGui::Button("Continue (F5)")) {
                    continue_execution();
                }
                ImGui::SameLine();
                if (ImGui::Button("Step (F10)")) {
                    step_requested_ = true;
                }
                ImGui::SameLine();
                if (ImGui::Button("Step Into (F11)")) {
                    step_requested_ = true;
                }
            } else {
                if (ImGui::Button("Pause (F5)")) {
                    paused_ = true;
                }
            }
            
            ImGui::Separator();
            
            // 16-bit register pairs
            ImGui::Text("16-bit Registers:");
            render_register_row("AF", regs->af());
            render_register_row("BC", regs->bc());
            render_register_row("DE", regs->de());
            render_register_row("HL", regs->hl());
            render_register_row("SP", regs->sp);
            render_register_row("PC", regs->pc);
            
            ImGui::Separator();
            
            // 8-bit registers
            ImGui::Text("8-bit Registers:");
            ImGui::Columns(2, nullptr, false);
            render_register_row_8("A", regs->a);
            render_register_row_8("B", regs->b);
            render_register_row_8("D", regs->d);
            render_register_row_8("H", regs->h);
            ImGui::NextColumn();
            render_register_row_8("F", regs->f);
            render_register_row_8("C", regs->c);
            render_register_row_8("E", regs->e);
            render_register_row_8("L", regs->l);
            ImGui::Columns(1);
            
            ImGui::Separator();
            
            // Flags
            ImGui::Text("Flags: %s", format_flags(regs->f).c_str());
            ImGui::Text("  Z=%d N=%d H=%d C=%d",
                (regs->f >> 7) & 1,
                (regs->f >> 6) & 1,
                (regs->f >> 5) & 1,
                (regs->f >> 4) & 1);
        } else {
            ImGui::Text("CPU not attached");
        }
    }
    ImGui::End();
}

void DebuggerGUI::render_disassembly_window() {
    ImGui::SetNextWindowSize(ImVec2(400, 400), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Disassembly", &show_disassembly_)) {
        // Controls
        ImGui::Checkbox("Follow PC", &follow_pc_);
        ImGui::SameLine();
        
        ImGui::SetNextItemWidth(60);
        if (ImGui::InputText("Go to", disasm_goto_address_, sizeof(disasm_goto_address_),
                             ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue)) {
            u16 addr = static_cast<u16>(std::strtol(disasm_goto_address_, nullptr, 16));
            disasm_address_ = addr;
            follow_pc_ = false;
        }
        
        ImGui::Separator();
        
        // Get PC
        u16 pc = debugger_->get_pc();
        
        // Determine view address
        u16 view_addr = follow_pc_ ? pc : disasm_address_;
        
        // Get disassembly
        auto instructions = debugger_->disassemble_range(view_addr > 16 ? view_addr - 16 : 0, 30);
        
        // Render instructions
        ImGui::BeginChild("DisasmScroll", ImVec2(0, 0), true);
        
        for (size_t i = 0; i < instructions.size(); ++i) {
            const auto& inst = instructions[i];
            
            // Build bytes string
            std::stringstream bytes_ss;
            for (u8 b : inst.bytes) {
                bytes_ss << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << static_cast<int>(b) << " ";
            }
            std::string bytes_str = bytes_ss.str();
            
            // Determine line color
            bool is_pc = (inst.address == pc);
            bool is_bp = inst.is_breakpoint;
            
            ImVec4 color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            if (is_pc && is_bp) {
                color = ImVec4(1.0f, 0.5f, 0.0f, 1.0f);  // Orange for PC + breakpoint
            } else if (is_pc) {
                color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);  // Yellow for PC
            } else if (is_bp) {
                color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);  // Red for breakpoint
            }
            
            ImGui::PushStyleColor(ImGuiCol_Text, color);
            
            // Breakpoint marker
            const char* marker = is_bp ? "●" : " ";
            const char* pc_marker = is_pc ? "►" : " ";
            
            // Render line
            std::stringstream line;
            line << marker << pc_marker << " "
                 << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << inst.address
                 << "  " << std::setw(9) << std::left << bytes_str
                 << std::setw(6) << inst.mnemonic << " " << inst.operands;
            
            if (ImGui::Selectable(line.str().c_str(), is_pc, ImGuiSelectableFlags_AllowDoubleClick)) {
                if (ImGui::IsMouseDoubleClicked(0)) {
                    debugger_->toggle_breakpoint(inst.address);
                }
            }
            
            // Right-click context menu
            if (ImGui::BeginPopupContextItem()) {
                if (is_bp) {
                    if (ImGui::MenuItem("Remove Breakpoint")) {
                        debugger_->remove_breakpoint(inst.address);
                    }
                } else {
                    if (ImGui::MenuItem("Add Breakpoint")) {
                        debugger_->add_breakpoint(inst.address);
                    }
                }
                if (ImGui::MenuItem("Run to here")) {
                    debugger_->add_breakpoint(inst.address);
                    continue_execution();
                }
                ImGui::EndPopup();
            }
            
            ImGui::PopStyleColor();
            
            // Auto-scroll to PC
            if (is_pc && follow_pc_) {
                ImGui::SetScrollHereY(0.5f);
            }
        }
        
        ImGui::EndChild();
    }
    ImGui::End();
}

void DebuggerGUI::render_memory_window() {
    ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Memory Viewer", &show_memory_)) {
        // Controls
        ImGui::SetNextItemWidth(60);
        if (ImGui::InputText("Address", memory_goto_address_, sizeof(memory_goto_address_),
                             ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue)) {
            memory_view_address_ = static_cast<u16>(std::strtol(memory_goto_address_, nullptr, 16));
        }
        
        ImGui::SameLine();
        if (ImGui::Button("ROM0")) memory_view_address_ = 0x0000;
        ImGui::SameLine();
        if (ImGui::Button("VRAM")) memory_view_address_ = 0x8000;
        ImGui::SameLine();
        if (ImGui::Button("WRAM")) memory_view_address_ = 0xC000;
        ImGui::SameLine();
        if (ImGui::Button("OAM")) memory_view_address_ = 0xFE00;
        ImGui::SameLine();
        if (ImGui::Button("IO")) memory_view_address_ = 0xFF00;
        ImGui::SameLine();
        if (ImGui::Button("HRAM")) memory_view_address_ = 0xFF80;
        
        ImGui::Separator();
        
        // Memory region info
        ImGui::Text("Region: %s", Debugger::get_memory_region_name(memory_view_address_).c_str());
        
        ImGui::Separator();
        
        // Memory display
        ImGui::BeginChild("MemoryScroll", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
        
        // Use a monospace font appearance
        int cols = memory_view_columns_;
        int rows = 16;
        
        for (int row = 0; row < rows; ++row) {
            u16 row_addr = memory_view_address_ + row * cols;
            
            // Address
            ImGui::Text("%04X: ", row_addr);
            ImGui::SameLine();
            
            // Hex values
            std::string hex_line;
            std::string ascii_line;
            
            for (int col = 0; col < cols; ++col) {
                u16 addr = row_addr + col;
                u8 val = debugger_->read_memory(addr);
                
                std::stringstream ss;
                ss << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << static_cast<int>(val) << " ";
                hex_line += ss.str();
                
                // ASCII representation
                if (val >= 32 && val < 127) {
                    ascii_line += static_cast<char>(val);
                } else {
                    ascii_line += '.';
                }
            }
            
            ImGui::Text("%s", hex_line.c_str());
            ImGui::SameLine();
            ImGui::TextDisabled("|%s|", ascii_line.c_str());
        }
        
        ImGui::EndChild();
        
        // Navigation
        if (ImGui::Button("◄◄")) {
            memory_view_address_ = (memory_view_address_ >= 0x100) ? memory_view_address_ - 0x100 : 0;
        }
        ImGui::SameLine();
        if (ImGui::Button("◄")) {
            memory_view_address_ = (memory_view_address_ >= cols) ? memory_view_address_ - cols : 0;
        }
        ImGui::SameLine();
        if (ImGui::Button("►")) {
            memory_view_address_ = (memory_view_address_ + cols <= 0xFFFF - rows * cols) ? memory_view_address_ + cols : memory_view_address_;
        }
        ImGui::SameLine();
        if (ImGui::Button("►►")) {
            memory_view_address_ = (memory_view_address_ + 0x100 <= 0xFFFF - rows * cols) ? memory_view_address_ + 0x100 : memory_view_address_;
        }
    }
    ImGui::End();
}

void DebuggerGUI::render_breakpoints_window() {
    ImGui::SetNextWindowSize(ImVec2(250, 300), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Breakpoints", &show_breakpoints_)) {
        // Add breakpoint
        ImGui::SetNextItemWidth(60);
        if (ImGui::InputText("##bp_addr", breakpoint_address_, sizeof(breakpoint_address_),
                             ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue)) {
            u16 addr = static_cast<u16>(std::strtol(breakpoint_address_, nullptr, 16));
            debugger_->add_breakpoint(addr);
            std::memset(breakpoint_address_, 0, sizeof(breakpoint_address_));
        }
        ImGui::SameLine();
        if (ImGui::Button("Add")) {
            u16 addr = static_cast<u16>(std::strtol(breakpoint_address_, nullptr, 16));
            debugger_->add_breakpoint(addr);
            std::memset(breakpoint_address_, 0, sizeof(breakpoint_address_));
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear All")) {
            debugger_->clear_all_breakpoints();
        }
        
        ImGui::Separator();
        
        // List breakpoints
        const auto& breakpoints = debugger_->get_breakpoints();
        
        if (breakpoints.empty()) {
            ImGui::TextDisabled("No breakpoints set");
        } else {
            ImGui::BeginChild("BPList", ImVec2(0, 0), true);
            
            std::vector<u16> to_remove;
            
            for (u16 addr : breakpoints) {
                std::stringstream ss;
                ss << "● $" << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << addr
                   << " (" << Debugger::get_memory_region_name(addr) << ")";
                
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
                ImGui::Selectable(ss.str().c_str());
                ImGui::PopStyleColor();
                
                if (ImGui::BeginPopupContextItem()) {
                    if (ImGui::MenuItem("Remove")) {
                        to_remove.push_back(addr);
                    }
                    if (ImGui::MenuItem("Go to in Disassembly")) {
                        disasm_address_ = addr;
                        follow_pc_ = false;
                        show_disassembly_ = true;
                    }
                    ImGui::EndPopup();
                }
            }
            
            // Remove breakpoints after iteration
            for (u16 addr : to_remove) {
                debugger_->remove_breakpoint(addr);
            }
            
            ImGui::EndChild();
        }
    }
    ImGui::End();
}

void DebuggerGUI::render_watches_window() {
    ImGui::SetNextWindowSize(ImVec2(300, 250), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Watches", &show_watches_)) {
        // Add watch
        ImGui::SetNextItemWidth(60);
        ImGui::InputText("##watch_addr", watch_address_, sizeof(watch_address_),
                         ImGuiInputTextFlags_CharsHexadecimal);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(100);
        ImGui::InputText("Label", watch_label_, sizeof(watch_label_));
        ImGui::SameLine();
        ImGui::Checkbox("Break", &watch_break_on_change_);
        ImGui::SameLine();
        if (ImGui::Button("Add Watch")) {
            u16 addr = static_cast<u16>(std::strtol(watch_address_, nullptr, 16));
            debugger_->add_watch(addr, watch_label_, watch_break_on_change_);
            std::memset(watch_address_, 0, sizeof(watch_address_));
            std::memset(watch_label_, 0, sizeof(watch_label_));
        }
        
        ImGui::Separator();
        
        // List watches
        const auto& watches = debugger_->get_watches();
        
        if (watches.empty()) {
            ImGui::TextDisabled("No watches set");
        } else {
            ImGui::BeginChild("WatchList", ImVec2(0, 0), true);
            
            // Header
            ImGui::Columns(4, "WatchColumns");
            ImGui::Text("Address"); ImGui::NextColumn();
            ImGui::Text("Label"); ImGui::NextColumn();
            ImGui::Text("Value"); ImGui::NextColumn();
            ImGui::Text("Break"); ImGui::NextColumn();
            ImGui::Separator();
            
            std::vector<u16> to_remove;
            
            for (const auto& watch : watches) {
                std::stringstream addr_ss;
                addr_ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << watch.address;
                ImGui::Text("%s", addr_ss.str().c_str());
                ImGui::NextColumn();
                
                ImGui::Text("%s", watch.label.c_str());
                ImGui::NextColumn();
                
                u8 val = debugger_->read_memory(watch.address);
                ImGui::Text("$%02X (%d)", val, val);
                ImGui::NextColumn();
                
                ImGui::Text("%s", watch.break_on_change ? "Yes" : "No");
                ImGui::NextColumn();
                
                // Context menu
                std::string popup_id = "watch_ctx_" + addr_ss.str();
                if (ImGui::BeginPopupContextItem(popup_id.c_str())) {
                    if (ImGui::MenuItem("Remove")) {
                        to_remove.push_back(watch.address);
                    }
                    if (ImGui::MenuItem("View in Memory")) {
                        memory_view_address_ = watch.address;
                        show_memory_ = true;
                    }
                    ImGui::EndPopup();
                }
            }
            
            ImGui::Columns(1);
            
            for (u16 addr : to_remove) {
                debugger_->remove_watch(addr);
            }
            
            ImGui::EndChild();
        }
    }
    ImGui::End();
}

void DebuggerGUI::render_stack_window() {
    ImGui::SetNextWindowSize(ImVec2(200, 300), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Stack", &show_stack_)) {
        const Registers* regs = debugger_->get_registers();
        
        if (regs) {
            ImGui::Text("SP: $%04X", regs->sp);
            ImGui::Separator();
            
            // Show stack contents (16 words below SP)
            ImGui::BeginChild("StackContents", ImVec2(0, 0), true);
            
            for (int i = 0; i < 16; ++i) {
                u16 addr = regs->sp + i * 2;
                if (addr > 0xFFFE) break;
                
                u8 lo = debugger_->read_memory(addr);
                u8 hi = debugger_->read_memory(addr + 1);
                u16 val = (hi << 8) | lo;
                
                if (i == 0) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
                }
                
                ImGui::Text("$%04X: %04X", addr, val);
                
                if (i == 0) {
                    ImGui::PopStyleColor();
                }
            }
            
            ImGui::EndChild();
        }
    }
    ImGui::End();
}

void DebuggerGUI::render_io_registers_window() {
    ImGui::SetNextWindowSize(ImVec2(350, 400), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("I/O Registers", &show_io_registers_)) {
        ImGui::BeginChild("IORegs", ImVec2(0, 0), true);
        
        // Helper lambda for register display
        auto show_reg = [this](const char* name, u16 addr) {
            u8 val = debugger_->read_memory(addr);
            ImGui::Text("$%04X %s: $%02X (%3d) %%", addr, name, val, val);
            ImGui::SameLine();
            // Binary representation
            for (int i = 7; i >= 0; --i) {
                ImGui::Text("%c", (val & (1 << i)) ? '1' : '0');
                if (i > 0) ImGui::SameLine(0, 0);
            }
        };
        
        // LCD registers
        if (ImGui::CollapsingHeader("LCD", ImGuiTreeNodeFlags_DefaultOpen)) {
            show_reg("LCDC", 0xFF40);
            show_reg("STAT", 0xFF41);
            show_reg("SCY ", 0xFF42);
            show_reg("SCX ", 0xFF43);
            show_reg("LY  ", 0xFF44);
            show_reg("LYC ", 0xFF45);
            show_reg("BGP ", 0xFF47);
            show_reg("OBP0", 0xFF48);
            show_reg("OBP1", 0xFF49);
            show_reg("WY  ", 0xFF4A);
            show_reg("WX  ", 0xFF4B);
        }
        
        // Sound registers
        if (ImGui::CollapsingHeader("Sound")) {
            show_reg("NR10", 0xFF10);
            show_reg("NR11", 0xFF11);
            show_reg("NR12", 0xFF12);
            show_reg("NR13", 0xFF13);
            show_reg("NR14", 0xFF14);
            show_reg("NR50", 0xFF24);
            show_reg("NR51", 0xFF25);
            show_reg("NR52", 0xFF26);
        }
        
        // Timer registers
        if (ImGui::CollapsingHeader("Timer")) {
            show_reg("DIV ", 0xFF04);
            show_reg("TIMA", 0xFF05);
            show_reg("TMA ", 0xFF06);
            show_reg("TAC ", 0xFF07);
        }
        
        // Interrupt registers
        if (ImGui::CollapsingHeader("Interrupts")) {
            show_reg("IF  ", 0xFF0F);
            show_reg("IE  ", 0xFFFF);
        }
        
        // Joypad
        if (ImGui::CollapsingHeader("Input")) {
            show_reg("JOYP", 0xFF00);
        }
        
        // Serial
        if (ImGui::CollapsingHeader("Serial")) {
            show_reg("SB  ", 0xFF01);
            show_reg("SC  ", 0xFF02);
        }
        
        // DMA
        if (ImGui::CollapsingHeader("DMA")) {
            show_reg("DMA ", 0xFF46);
        }
        
        ImGui::EndChild();
    }
    ImGui::End();
}

} // namespace gbcrush
