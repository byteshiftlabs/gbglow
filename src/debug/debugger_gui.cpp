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
    , docking_mode_(false)
    , show_registers_(true)
    , show_disassembly_(true)
    , show_memory_(true)
    , show_breakpoints_(false)
    , show_watches_(false)
    , show_stack_(false)
    , show_io_registers_(false)
    , show_sprites_(true)
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

void DebuggerGUI::set_docking_mode(bool enabled) {
    docking_mode_ = enabled;
}

void DebuggerGUI::apply_debug_theme() {
    ImGuiStyle& style = ImGui::GetStyle();
    
    // Dark blue/purple theme for debugger
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.08f, 0.12f, 1.00f);
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.06f, 0.06f, 0.10f, 1.00f);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.10f, 0.10f, 0.14f, 1.00f);
    style.Colors[ImGuiCol_Border] = ImVec4(0.30f, 0.30f, 0.50f, 0.50f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.12f, 0.12f, 0.18f, 1.00f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.20f, 0.30f, 1.00f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.25f, 0.25f, 0.40f, 1.00f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.10f, 0.20f, 1.00f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.15f, 0.35f, 1.00f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.08f, 0.08f, 0.15f, 1.00f);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.10f, 0.18f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.06f, 0.06f, 0.10f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.25f, 0.25f, 0.40f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.35f, 0.35f, 0.55f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.45f, 0.45f, 0.70f, 1.00f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.50f, 0.70f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.20f, 0.25f, 0.40f, 1.00f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.35f, 0.55f, 1.00f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.40f, 0.45f, 0.70f, 1.00f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.20f, 0.25f, 0.40f, 1.00f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.35f, 0.55f, 1.00f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.40f, 0.45f, 0.70f, 1.00f);
    style.Colors[ImGuiCol_Separator] = ImVec4(0.30f, 0.30f, 0.50f, 0.50f);
    style.Colors[ImGuiCol_Text] = ImVec4(0.85f, 0.90f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.60f, 1.00f);
}

void DebuggerGUI::render() {
    if (!debugger_ || !debugger_->is_attached()) {
        return;
    }
    
    // In non-docking mode, check visibility and render menu bar
    if (!docking_mode_) {
        if (!visible_) {
            return;
        }
        render_menu_bar();
    }
    
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
    if (show_sprites_) {
        render_sprites_window();
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
            if (ImGui::MenuItem("Sprites (OAM)", nullptr, show_sprites_)) {
                show_sprites_ = !show_sprites_;
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
    // Fixed position: right of emulator panel
    if (docking_mode_) {
        ImGui::SetNextWindowPos(ImVec2(485, 20), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(200, 350), ImGuiCond_Always);
    } else {
        ImGui::SetNextWindowSize(ImVec2(200, 300), ImGuiCond_FirstUseEver);
    }
    
    ImGuiWindowFlags flags = docking_mode_ ? 
        (ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse) : 0;
    
    if (ImGui::Begin("CPU Registers", docking_mode_ ? nullptr : &show_registers_, flags)) {
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
    // Fixed position: below registers
    if (docking_mode_) {
        ImGui::SetNextWindowPos(ImVec2(485, 375), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(400, 405), ImGuiCond_Always);
    } else {
        ImGui::SetNextWindowSize(ImVec2(400, 400), ImGuiCond_FirstUseEver);
    }
    
    ImGuiWindowFlags flags = docking_mode_ ? 
        (ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse) : 0;
    
    if (ImGui::Begin("Disassembly", docking_mode_ ? nullptr : &show_disassembly_, flags)) {
        // Controls row 1
        ImGui::Checkbox("Follow PC", &follow_pc_);
        ImGui::SameLine();
        
        ImGui::SetNextItemWidth(60);
        if (ImGui::InputText("Go to", disasm_goto_address_, sizeof(disasm_goto_address_),
                             ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue)) {
            u16 addr = static_cast<u16>(std::strtol(disasm_goto_address_, nullptr, 16));
            disasm_address_ = addr;
            follow_pc_ = false;
        }
        
        // Quick jump buttons for common Game Boy code locations
        ImGui::SameLine();
        if (ImGui::Button("PC")) { follow_pc_ = true; }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Jump to current PC");
        
        ImGui::SameLine();
        if (ImGui::Button("Entry")) { disasm_address_ = 0x0100; follow_pc_ = false; }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("ROM Entry Point ($0100)");
        
        ImGui::SameLine();
        if (ImGui::Button("RST")) { disasm_address_ = 0x0000; follow_pc_ = false; }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("RST vectors ($0000-$00FF)");
        
        ImGui::SameLine();
        if (ImGui::Button("INT")) { disasm_address_ = 0x0040; follow_pc_ = false; }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Interrupt vectors ($0040-$0060)");
        
        ImGui::Separator();
        
        // Show current PC for reference
        u16 pc = debugger_->get_pc();
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.5f, 1.0f), "PC: $%04X", pc);
        ImGui::SameLine();
        ImGui::TextDisabled("(double-click to set breakpoint)");
        
        ImGui::Separator();
        
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
    // Fixed position: right of registers
    if (docking_mode_) {
        ImGui::SetNextWindowPos(ImVec2(690, 20), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(585, 350), ImGuiCond_Always);
    } else {
        ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
    }
    
    ImGuiWindowFlags flags = docking_mode_ ? 
        (ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse) : 0;
    
    if (ImGui::Begin("Memory Viewer", docking_mode_ ? nullptr : &show_memory_, flags)) {
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
    // Fixed position: below emulator
    if (docking_mode_) {
        ImGui::SetNextWindowPos(ImVec2(0, 455), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(240, 325), ImGuiCond_Always);
    } else {
        ImGui::SetNextWindowSize(ImVec2(250, 300), ImGuiCond_FirstUseEver);
    }
    
    ImGuiWindowFlags flags = docking_mode_ ? 
        (ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse) : 0;
    
    if (ImGui::Begin("Breakpoints", docking_mode_ ? nullptr : &show_breakpoints_, flags)) {
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
    // Fixed position: right of breakpoints
    if (docking_mode_) {
        ImGui::SetNextWindowPos(ImVec2(245, 455), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(240, 325), ImGuiCond_Always);
    } else {
        ImGui::SetNextWindowSize(ImVec2(300, 250), ImGuiCond_FirstUseEver);
    }
    
    ImGuiWindowFlags flags = docking_mode_ ? 
        (ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse) : 0;
    
    if (ImGui::Begin("Watches", docking_mode_ ? nullptr : &show_watches_, flags)) {
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
    // Fixed position: right of disassembly
    if (docking_mode_) {
        ImGui::SetNextWindowPos(ImVec2(890, 375), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(180, 405), ImGuiCond_Always);
    } else {
        ImGui::SetNextWindowSize(ImVec2(200, 300), ImGuiCond_FirstUseEver);
    }
    
    ImGuiWindowFlags flags = docking_mode_ ? 
        (ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse) : 0;
    
    if (ImGui::Begin("Stack", docking_mode_ ? nullptr : &show_stack_, flags)) {
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
    // Fixed position: right of stack
    if (docking_mode_) {
        ImGui::SetNextWindowPos(ImVec2(1075, 375), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(200, 405), ImGuiCond_Always);
    } else {
        ImGui::SetNextWindowSize(ImVec2(350, 400), ImGuiCond_FirstUseEver);
    }
    
    ImGuiWindowFlags flags = docking_mode_ ? 
        (ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse) : 0;
    
    if (ImGui::Begin("I/O Registers", docking_mode_ ? nullptr : &show_io_registers_, flags)) {
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

void DebuggerGUI::render_sprites_window() {
    // Fixed position: below emulator
    if (docking_mode_) {
        ImGui::SetNextWindowPos(ImVec2(0, 455), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(480, 325), ImGuiCond_Always);
    } else {
        ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
    }
    
    ImGuiWindowFlags flags = docking_mode_ ? 
        (ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse) : 0;
    
    if (ImGui::Begin("Sprites (OAM)", docking_mode_ ? nullptr : &show_sprites_, flags)) {
        // LCDC register to check sprite settings
        u8 lcdc = debugger_->read_memory(0xFF40);
        bool sprites_enabled = (lcdc & 0x02) != 0;
        bool large_sprites = (lcdc & 0x04) != 0;  // 8x16 mode
        
        // Get palettes
        u8 obp0 = debugger_->read_memory(0xFF48);
        u8 obp1 = debugger_->read_memory(0xFF49);
        
        // DMG grayscale colors (sprite color 0 is always transparent)
        auto get_color = [](u8 palette, int color_idx) -> ImU32 {
            if (color_idx == 0) return IM_COL32(0, 0, 0, 0);  // Transparent
            int shade = (palette >> (color_idx * 2)) & 0x03;
            // 0=white, 1=light gray, 2=dark gray, 3=black
            const u8 shades[] = {255, 170, 85, 0};
            u8 c = shades[shade];
            return IM_COL32(c, c, c, 255);
        };
        
        // Display sprite mode info
        if (sprites_enabled) {
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "ON");
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "OFF");
        }
        ImGui::SameLine();
        ImGui::Text("| %s | OBP0:%02X OBP1:%02X", large_sprites ? "8x16" : "8x8", obp0, obp1);
        
        ImGui::Separator();
        
        // Sprite grid - show all 40 sprites as actual graphics
        ImGui::BeginChild("SpriteGrid", ImVec2(0, 0), true);
        
        const u16 OAM_START = 0xFE00;
        const u16 TILE_DATA = 0x8000;  // Sprites always use $8000 addressing
        const int NUM_SPRITES = 40;
        const float PIXEL_SIZE = 2.0f;  // Scale factor for pixels
        const int SPRITE_HEIGHT = large_sprites ? 16 : 8;
        const float CELL_WIDTH = 8 * PIXEL_SIZE + 4;
        const float CELL_HEIGHT = SPRITE_HEIGHT * PIXEL_SIZE + 20;  // Extra for label
        
        int sprites_per_row = static_cast<int>(ImGui::GetContentRegionAvail().x / CELL_WIDTH);
        if (sprites_per_row < 1) sprites_per_row = 1;
        
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        
        int visible_count = 0;
        
        for (int i = 0; i < NUM_SPRITES; ++i) {
            u16 oam_addr = OAM_START + (i * 4);
            
            u8 y_pos = debugger_->read_memory(oam_addr);
            u8 x_pos = debugger_->read_memory(oam_addr + 1);
            u8 tile_num = debugger_->read_memory(oam_addr + 2);
            u8 attr = debugger_->read_memory(oam_addr + 3);
            
            // In 8x16 mode, bit 0 of tile number is ignored
            if (large_sprites) {
                tile_num &= 0xFE;
            }
            
            bool y_flip = (attr & 0x40) != 0;
            bool x_flip = (attr & 0x20) != 0;
            bool use_obp1 = (attr & 0x10) != 0;
            u8 palette = use_obp1 ? obp1 : obp0;
            
            bool on_screen = (y_pos > 0 && y_pos < 160) && (x_pos > 0 && x_pos < 168);
            if (on_screen) visible_count++;
            
            // Calculate grid position
            int col = i % sprites_per_row;
            int row = i / sprites_per_row;
            
            ImVec2 cell_pos = ImGui::GetCursorScreenPos();
            cell_pos.x += col * CELL_WIDTH;
            cell_pos.y += row * CELL_HEIGHT;
            
            // Draw border (yellow for visible, gray for off-screen)
            ImU32 border_color = on_screen ? IM_COL32(255, 255, 100, 255) : IM_COL32(80, 80, 80, 255);
            draw_list->AddRect(
                ImVec2(cell_pos.x, cell_pos.y),
                ImVec2(cell_pos.x + 8 * PIXEL_SIZE + 2, cell_pos.y + SPRITE_HEIGHT * PIXEL_SIZE + 2),
                border_color
            );
            
            // Draw sprite pixels
            for (int tile_row = 0; tile_row < SPRITE_HEIGHT; ++tile_row) {
                int actual_row = y_flip ? (SPRITE_HEIGHT - 1 - tile_row) : tile_row;
                
                // Calculate which tile and row within tile
                int tile_offset = (actual_row >= 8) ? 1 : 0;
                int row_in_tile = actual_row & 7;
                
                u8 current_tile = tile_num + tile_offset;
                u16 tile_addr = TILE_DATA + (current_tile * 16) + (row_in_tile * 2);
                
                u8 byte1 = debugger_->read_memory(tile_addr);
                u8 byte2 = debugger_->read_memory(tile_addr + 1);
                
                for (int px = 0; px < 8; ++px) {
                    int actual_px = x_flip ? px : (7 - px);
                    
                    int color_idx = ((byte1 >> actual_px) & 1) | (((byte2 >> actual_px) & 1) << 1);
                    ImU32 color = get_color(palette, color_idx);
                    
                    if (color_idx != 0) {  // Don't draw transparent pixels
                        float x = cell_pos.x + 1 + (7 - px) * PIXEL_SIZE;
                        float y = cell_pos.y + 1 + tile_row * PIXEL_SIZE;
                        draw_list->AddRectFilled(
                            ImVec2(x, y),
                            ImVec2(x + PIXEL_SIZE, y + PIXEL_SIZE),
                            color
                        );
                    }
                }
            }
            
            // Draw sprite number below
            char label[8];
            snprintf(label, sizeof(label), "%02d", i);
            draw_list->AddText(
                ImVec2(cell_pos.x + 2, cell_pos.y + SPRITE_HEIGHT * PIXEL_SIZE + 3),
                on_screen ? IM_COL32(255, 255, 100, 255) : IM_COL32(150, 150, 150, 255),
                label
            );
        }
        
        // Reserve space for the grid
        int total_rows = (NUM_SPRITES + sprites_per_row - 1) / sprites_per_row;
        ImGui::Dummy(ImVec2(sprites_per_row * CELL_WIDTH, total_rows * CELL_HEIGHT));
        
        ImGui::EndChild();
        
        // Status bar
        ImGui::Separator();
        ImGui::Text("Visible: %d/40", visible_count);
        ImGui::SameLine();
        ImGui::TextDisabled("(yellow = on screen)");
    }
    ImGui::End();
}

} // namespace gbcrush
