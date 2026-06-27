// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 gbglow Contributors
// This file is part of gbglow. See LICENSE for details.

#include "debugger_gui.h"
#include "debugger.h"
#include "../core/registers.h"
#include "../core/io_registers.h"
#include <imgui.h>
#include <sstream>
#include <iomanip>
#include <cstring>

namespace gbglow {

using namespace io_reg;

namespace {
    // Memory map regions (for quick-nav)
    constexpr u16 REGION_ROM0 = 0x0000;
    constexpr u16 REGION_VRAM = 0x8000;
    constexpr u16 REGION_WRAM = 0xC000;
    constexpr u16 REGION_OAM  = 0xFE00;
    constexpr u16 REGION_IO   = 0xFF00;
    constexpr u16 REGION_HRAM = 0xFF80;
    
    // Disassembly quick-jump addresses
    constexpr u16 ENTRY_POINT      = 0x0100;
    constexpr u16 RST_VECTORS      = 0x0000;
    constexpr u16 INT_VECTORS      = 0x0040;
    
    // LCDC bit flags
    constexpr u8 LCDC_OBJ_ENABLE = 0x02;   // Bit 1: OBJ display enable
    constexpr u8 LCDC_OBJ_SIZE   = 0x04;   // Bit 2: OBJ size (0=8x8, 1=8x16)
    
    // Sprite attribute flags
    constexpr u8 ATTR_Y_FLIP  = 0x40;   // Bit 6: Y flip
    constexpr u8 ATTR_X_FLIP  = 0x20;   // Bit 5: X flip
    constexpr u8 ATTR_PALETTE = 0x10;   // Bit 4: palette selection (0=OBP0, 1=OBP1)
    constexpr u8 TILE_NUM_MASK_8x16 = 0xFE;  // In 8x16 mode, bit 0 of tile is ignored
    
    // DMG palette shade mask
    constexpr u8 SHADE_MASK = 0x03;
    
    // Memory page size for navigation
    constexpr u16 PAGE_SIZE = 0x100;
    
    // Address space limit
    constexpr u16 ADDR_MAX = 0xFFFF;
    constexpr u16 STACK_ADDR_LIMIT = 0xFFFE;
    constexpr u32 ADDRESS_SPACE_SIZE = 0x10000;
    
    // Sprite constants
    constexpr u16 OAM_BASE   = 0xFE00;
    constexpr u16 TILE_DATA  = 0x8000;
    constexpr int NUM_SPRITES = 40;
    constexpr int SPRITE_WIDTH = 8;
    constexpr int TILE_BYTES_PER_ROW = 2;
    constexpr int TILE_SIZE_BYTES = 16;
    constexpr int PIXELS_PER_TILE_ROW = 8;

    // ---- Debugger panel layout (docked mode) ----
    // Emulator viewport occupies (0,0)-(480,450).
    // Panels are arranged around it.
    namespace layout {
        // Registers panel (right of viewport, top)
        constexpr float REG_X = 485.0f;
        constexpr float REG_Y = 20.0f;
        constexpr float REG_W = 200.0f;
        constexpr float REG_H = 350.0f;
        constexpr float REG_FREE_W = 200.0f;
        constexpr float REG_FREE_H = 300.0f;

        // Disassembly panel (right of viewport, below registers)
        constexpr float DISASM_X = 485.0f;
        constexpr float DISASM_Y = 375.0f;
        constexpr float DISASM_W = 400.0f;
        constexpr float DISASM_H = 405.0f;
        constexpr float DISASM_FREE_W = 400.0f;
        constexpr float DISASM_FREE_H = 400.0f;

        // Memory viewer (far right, top)
        constexpr float MEM_X = 690.0f;
        constexpr float MEM_Y = 20.0f;
        constexpr float MEM_W = 585.0f;
        constexpr float MEM_H = 350.0f;
        constexpr float MEM_FREE_W = 500.0f;
        constexpr float MEM_FREE_H = 400.0f;

        // Breakpoints (bottom left)
        constexpr float BP_X = 0.0f;
        constexpr float BP_Y = 455.0f;
        constexpr float BP_W = 240.0f;
        constexpr float BP_H = 325.0f;
        constexpr float BP_FREE_W = 250.0f;
        constexpr float BP_FREE_H = 300.0f;

        // Watch expressions (bottom, right of breakpoints)
        constexpr float WATCH_X = 245.0f;
        constexpr float WATCH_Y = 455.0f;
        constexpr float WATCH_W = 240.0f;
        constexpr float WATCH_H = 325.0f;
        constexpr float WATCH_FREE_W = 300.0f;
        constexpr float WATCH_FREE_H = 250.0f;

        // Stack viewer (below disassembly, right side)
        constexpr float STACK_X = 890.0f;
        constexpr float STACK_Y = 375.0f;
        constexpr float STACK_W = 180.0f;
        constexpr float STACK_H = 405.0f;
        constexpr float STACK_FREE_W = 200.0f;
        constexpr float STACK_FREE_H = 300.0f;

        // IO registers (far right, below memory viewer)
        constexpr float IO_X = 1075.0f;
        constexpr float IO_Y = 375.0f;
        constexpr float IO_W = 200.0f;
        constexpr float IO_H = 405.0f;
        constexpr float IO_FREE_W = 350.0f;
        constexpr float IO_FREE_H = 400.0f;

        // Sprite viewer (bottom, replaces BP+Watch when active)
        constexpr float SPRITE_X = 0.0f;
        constexpr float SPRITE_Y = 455.0f;
        constexpr float SPRITE_W = 480.0f;
        constexpr float SPRITE_H = 325.0f;
        constexpr float SPRITE_FREE_W = 500.0f;
        constexpr float SPRITE_FREE_H = 400.0f;
    } // namespace layout

bool parse_hex_u16(const char* text, u16& value) {
    if (!text || *text == '\0') {
        return false;
    }

    char* end = nullptr;
    const unsigned long parsed = std::strtoul(text, &end, 16);
    if (end == text || *end != '\0' || parsed > ADDR_MAX) {
        return false;
    }

    value = static_cast<u16>(parsed);
    return true;
}

u16 clamp_memory_view_start(u32 start_address, int rows, int cols) {
    const u32 visible_bytes = static_cast<u32>(rows) * static_cast<u32>(cols);
    if (visible_bytes == 0 || visible_bytes >= ADDRESS_SPACE_SIZE) {
        return 0;
    }

    const u32 max_start = ADDRESS_SPACE_SIZE - visible_bytes;
    return static_cast<u16>(std::min(start_address, max_start));
}
} // anonymous namespace

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

void DebuggerGUI::attach(Debugger& debugger) {
    debugger_ = &debugger;
}

void DebuggerGUI::toggle_visible() {
    set_visible(!visible_);
}

bool DebuggerGUI::is_visible() const {
    return visible_;
}

void DebuggerGUI::set_visible(bool visible) {
    visible_ = visible;
    if (visible) {
        paused_ = true;
    } else {
        paused_ = false;
        clear_execution_requests();
    }
}

bool DebuggerGUI::should_pause() const {
    return visible_ && paused_;
}

bool DebuggerGUI::step_requested() const {
    return step_requested_;
}

void DebuggerGUI::request_step() {
    step_requested_ = true;
    continue_requested_ = false;
}

void DebuggerGUI::request_step_over() {
    if (debugger_ && debugger_->prepare_step_over_for_current_instruction()) {
        continue_requested_ = true;
        paused_ = false;
        step_requested_ = false;
        return;
    }

    request_step();
}

void DebuggerGUI::clear_step_request() {
    step_requested_ = false;
}

void DebuggerGUI::continue_execution() {
    continue_requested_ = true;
    step_requested_ = false;
    paused_ = false;
}

bool DebuggerGUI::should_continue() const {
    return continue_requested_;
}

void DebuggerGUI::clear_continue() {
    continue_requested_ = false;
}

void DebuggerGUI::clear_execution_requests() {
    step_requested_ = false;
    continue_requested_ = false;
}

void DebuggerGUI::pause_execution() {
    paused_ = true;
    continue_requested_ = false;
}

void DebuggerGUI::set_docking_mode(bool enabled) {
    docking_mode_ = enabled;
}

void DebuggerGUI::render_window_menu_items() {
    ImGui::MenuItem("Registers", nullptr, &show_registers_);
    ImGui::MenuItem("Disassembly", nullptr, &show_disassembly_);
    ImGui::MenuItem("Memory", nullptr, &show_memory_);
    ImGui::MenuItem("Breakpoints", nullptr, &show_breakpoints_);
    ImGui::MenuItem("Watches", nullptr, &show_watches_);
    ImGui::MenuItem("Stack", nullptr, &show_stack_);
    ImGui::MenuItem("I/O Registers", nullptr, &show_io_registers_);
    ImGui::MenuItem("Sprites (OAM)", nullptr, &show_sprites_);
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
            
            if (ImGui::MenuItem("Close Debugger", "F11")) {
                set_visible(false);
            }
            
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

std::string DebuggerGUI::format_flags(u8 f) {
    std::string result;
    result += (f & Registers::FLAG_Z) ? 'Z' : '-';
    result += (f & Registers::FLAG_N) ? 'N' : '-';
    result += (f & Registers::FLAG_H) ? 'H' : '-';
    result += (f & Registers::FLAG_C) ? 'C' : '-';
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
        ImGui::SetNextWindowPos(ImVec2(layout::REG_X, layout::REG_Y), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(layout::REG_W, layout::REG_H), ImGuiCond_Always);
    } else {
        ImGui::SetNextWindowSize(ImVec2(layout::REG_FREE_W, layout::REG_FREE_H), ImGuiCond_FirstUseEver);
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
                if (ImGui::Button("Step Over (F10)")) {
                    request_step_over();
                }
                ImGui::SameLine();
                if (ImGui::Button("Step Into")) {
                    request_step();
                }
            } else {
                if (ImGui::Button("Pause (F5)")) {
                    pause_execution();
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
        ImGui::SetNextWindowPos(ImVec2(layout::DISASM_X, layout::DISASM_Y), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(layout::DISASM_W, layout::DISASM_H), ImGuiCond_Always);
    } else {
        ImGui::SetNextWindowSize(ImVec2(layout::DISASM_FREE_W, layout::DISASM_FREE_H), ImGuiCond_FirstUseEver);
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
            u16 addr = 0;
            if (parse_hex_u16(disasm_goto_address_, addr)) {
                disasm_address_ = addr;
                follow_pc_ = false;
            }
        }
        
        // Quick jump buttons for common Game Boy code locations
        ImGui::SameLine();
        if (ImGui::Button("PC")) { follow_pc_ = true; }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Jump to current PC");
        
        ImGui::SameLine();
        if (ImGui::Button("Entry")) { disasm_address_ = ENTRY_POINT; follow_pc_ = false; }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("ROM Entry Point ($0100)");
        
        ImGui::SameLine();
        if (ImGui::Button("RST")) { disasm_address_ = RST_VECTORS; follow_pc_ = false; }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("RST vectors ($0000-$00FF)");
        
        ImGui::SameLine();
        if (ImGui::Button("INT")) { disasm_address_ = INT_VECTORS; follow_pc_ = false; }
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
        ImGui::SetNextWindowPos(ImVec2(layout::MEM_X, layout::MEM_Y), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(layout::MEM_W, layout::MEM_H), ImGuiCond_Always);
    } else {
        ImGui::SetNextWindowSize(ImVec2(layout::MEM_FREE_W, layout::MEM_FREE_H), ImGuiCond_FirstUseEver);
    }
    
    ImGuiWindowFlags flags = docking_mode_ ? 
        (ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse) : 0;
    
    if (ImGui::Begin("Memory Viewer", docking_mode_ ? nullptr : &show_memory_, flags)) {
        constexpr int rows = 16;
        const int cols = memory_view_columns_;
        memory_view_address_ = clamp_memory_view_start(memory_view_address_, rows, cols);

        // Controls
        ImGui::SetNextItemWidth(60);
        if (ImGui::InputText("Address", memory_goto_address_, sizeof(memory_goto_address_),
                             ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue)) {
            u16 addr = 0;
            if (parse_hex_u16(memory_goto_address_, addr)) {
                memory_view_address_ = clamp_memory_view_start(addr, rows, cols);
            }
        }
        
        ImGui::SameLine();
        if (ImGui::Button("ROM0")) memory_view_address_ = REGION_ROM0;
        ImGui::SameLine();
        if (ImGui::Button("VRAM")) memory_view_address_ = REGION_VRAM;
        ImGui::SameLine();
        if (ImGui::Button("WRAM")) memory_view_address_ = REGION_WRAM;
        ImGui::SameLine();
        if (ImGui::Button("OAM")) memory_view_address_ = REGION_OAM;
        ImGui::SameLine();
        if (ImGui::Button("IO")) memory_view_address_ = REGION_IO;
        ImGui::SameLine();
        if (ImGui::Button("HRAM")) memory_view_address_ = REGION_HRAM;
        
        ImGui::Separator();
        
        // Memory region info
        ImGui::Text("Region: %s", Debugger::get_memory_region_name(memory_view_address_).c_str());
        
        ImGui::Separator();
        
        // Memory display
        ImGui::BeginChild("MemoryScroll", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
        
        // Use a monospace font appearance
        for (int row = 0; row < rows; ++row) {
            const u32 row_addr = static_cast<u32>(memory_view_address_) + static_cast<u32>(row * cols);
            if (row_addr > ADDR_MAX) {
                break;
            }
            
            // Address
            ImGui::Text("%04X: ", static_cast<u16>(row_addr));
            ImGui::SameLine();
            
            // Hex values
            std::string hex_line;
            std::string ascii_line;
            
            for (int col = 0; col < cols; ++col) {
                const u32 addr = row_addr + static_cast<u32>(col);
                if (addr > ADDR_MAX) {
                    break;
                }

                u8 val = debugger_->read_memory(static_cast<u16>(addr));
                
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
            const u32 current = memory_view_address_;
            memory_view_address_ = clamp_memory_view_start(
                current >= PAGE_SIZE ? current - PAGE_SIZE : 0,
                rows,
                cols);
        }
        ImGui::SameLine();
        if (ImGui::Button("◄")) {
            const u32 current = memory_view_address_;
            memory_view_address_ = clamp_memory_view_start(
                current >= static_cast<u32>(cols) ? current - static_cast<u32>(cols) : 0,
                rows,
                cols);
        }
        ImGui::SameLine();
        if (ImGui::Button("►")) {
            memory_view_address_ = clamp_memory_view_start(
                static_cast<u32>(memory_view_address_) + static_cast<u32>(cols),
                rows,
                cols);
        }
        ImGui::SameLine();
        if (ImGui::Button("►►")) {
            memory_view_address_ = clamp_memory_view_start(
                static_cast<u32>(memory_view_address_) + PAGE_SIZE,
                rows,
                cols);
        }
    }
    ImGui::End();
}

void DebuggerGUI::render_breakpoints_window() {
    // Fixed position: below emulator
    if (docking_mode_) {
        ImGui::SetNextWindowPos(ImVec2(layout::BP_X, layout::BP_Y), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(layout::BP_W, layout::BP_H), ImGuiCond_Always);
    } else {
        ImGui::SetNextWindowSize(ImVec2(layout::BP_FREE_W, layout::BP_FREE_H), ImGuiCond_FirstUseEver);
    }
    
    ImGuiWindowFlags flags = docking_mode_ ? 
        (ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse) : 0;
    
    if (ImGui::Begin("Breakpoints", docking_mode_ ? nullptr : &show_breakpoints_, flags)) {
        // Add breakpoint
        ImGui::SetNextItemWidth(60);
        if (ImGui::InputText("##bp_addr", breakpoint_address_, sizeof(breakpoint_address_),
                             ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue)) {
            u16 addr = 0;
            if (parse_hex_u16(breakpoint_address_, addr)) {
                debugger_->add_breakpoint(addr);
                std::memset(breakpoint_address_, 0, sizeof(breakpoint_address_));
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Add")) {
            u16 addr = 0;
            if (parse_hex_u16(breakpoint_address_, addr)) {
                debugger_->add_breakpoint(addr);
                std::memset(breakpoint_address_, 0, sizeof(breakpoint_address_));
            }
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
        ImGui::SetNextWindowPos(ImVec2(layout::WATCH_X, layout::WATCH_Y), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(layout::WATCH_W, layout::WATCH_H), ImGuiCond_Always);
    } else {
        ImGui::SetNextWindowSize(ImVec2(layout::WATCH_FREE_W, layout::WATCH_FREE_H), ImGuiCond_FirstUseEver);
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
            u16 addr = 0;
            if (parse_hex_u16(watch_address_, addr)) {
                debugger_->add_watch(addr, watch_label_, watch_break_on_change_);
                std::memset(watch_address_, 0, sizeof(watch_address_));
                std::memset(watch_label_, 0, sizeof(watch_label_));
            }
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
        ImGui::SetNextWindowPos(ImVec2(layout::STACK_X, layout::STACK_Y), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(layout::STACK_W, layout::STACK_H), ImGuiCond_Always);
    } else {
        ImGui::SetNextWindowSize(ImVec2(layout::STACK_FREE_W, layout::STACK_FREE_H), ImGuiCond_FirstUseEver);
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
                const u32 addr = static_cast<u32>(regs->sp) + static_cast<u32>(i * 2);
                if (addr > STACK_ADDR_LIMIT) break;
                
                u8 lo = debugger_->read_memory(static_cast<u16>(addr));
                u8 hi = debugger_->read_memory(static_cast<u16>(addr + 1));
                u16 val = (hi << 8) | lo;
                
                if (i == 0) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
                }
                
                ImGui::Text("$%04X: %04X", static_cast<u16>(addr), val);
                
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
        ImGui::SetNextWindowPos(ImVec2(layout::IO_X, layout::IO_Y), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(layout::IO_W, layout::IO_H), ImGuiCond_Always);
    } else {
        ImGui::SetNextWindowSize(ImVec2(layout::IO_FREE_W, layout::IO_FREE_H), ImGuiCond_FirstUseEver);
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
            show_reg("LCDC", REG_LCDC);
            show_reg("STAT", REG_STAT);
            show_reg("SCY ", REG_SCY);
            show_reg("SCX ", REG_SCX);
            show_reg("LY  ", REG_LY);
            show_reg("LYC ", REG_LYC);
            show_reg("BGP ", REG_BGP);
            show_reg("OBP0", REG_OBP0);
            show_reg("OBP1", REG_OBP1);
            show_reg("WY  ", REG_WY);
            show_reg("WX  ", REG_WX);
        }
        
        // Sound registers
        if (ImGui::CollapsingHeader("Sound")) {
            show_reg("NR10", REG_NR10);
            show_reg("NR11", REG_NR11);
            show_reg("NR12", REG_NR12);
            show_reg("NR13", REG_NR13);
            show_reg("NR14", REG_NR14);
            show_reg("NR50", REG_NR50);
            show_reg("NR51", REG_NR51);
            show_reg("NR52", REG_NR52);
        }
        
        // Timer registers
        if (ImGui::CollapsingHeader("Timer")) {
            show_reg("DIV ", REG_DIV);
            show_reg("TIMA", REG_TIMA);
            show_reg("TMA ", REG_TMA);
            show_reg("TAC ", REG_TAC);
        }
        
        // Interrupt registers
        if (ImGui::CollapsingHeader("Interrupts")) {
            show_reg("IF  ", REG_IF);
            show_reg("IE  ", REG_IE);
        }
        
        // Joypad
        if (ImGui::CollapsingHeader("Input")) {
            show_reg("JOYP", REG_JOYP);
        }
        
        // Serial
        if (ImGui::CollapsingHeader("Serial")) {
            show_reg("SB  ", REG_SB);
            show_reg("SC  ", REG_SC);
        }
        
        // DMA
        if (ImGui::CollapsingHeader("DMA")) {
            show_reg("DMA ", REG_DMA);
        }
        
        ImGui::EndChild();
    }
    ImGui::End();
}

void DebuggerGUI::render_sprites_window() {
    // Fixed position: below emulator
    if (docking_mode_) {
        ImGui::SetNextWindowPos(ImVec2(layout::SPRITE_X, layout::SPRITE_Y), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(layout::SPRITE_W, layout::SPRITE_H), ImGuiCond_Always);
    } else {
        ImGui::SetNextWindowSize(ImVec2(layout::SPRITE_FREE_W, layout::SPRITE_FREE_H), ImGuiCond_FirstUseEver);
    }
    
    ImGuiWindowFlags flags = docking_mode_ ? 
        (ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse) : 0;
    
    if (ImGui::Begin("Sprite Layer", docking_mode_ ? nullptr : &show_sprites_, flags)) {
        // LCDC register to check sprite settings
        u8 lcdc = debugger_->read_memory(REG_LCDC);
        bool sprites_enabled = (lcdc & LCDC_OBJ_ENABLE) != 0;
        bool large_sprites = (lcdc & LCDC_OBJ_SIZE) != 0;  // 8x16 mode
        
        // Get palettes
        u8 obp0 = debugger_->read_memory(REG_OBP0);
        u8 obp1 = debugger_->read_memory(REG_OBP1);
        
        // DMG grayscale colors (sprite color 0 is always transparent)
        auto get_color = [](u8 palette, int color_idx) -> ImU32 {
            if (color_idx == 0) return IM_COL32(0, 0, 0, 0);  // Transparent
            int shade = (palette >> (color_idx * 2)) & SHADE_MASK;
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
        
        // Screen-like view showing sprites at their actual positions
        // Game Boy screen is 160x144, we'll scale it to fit
        const int GB_WIDTH = 160;
        const int GB_HEIGHT = 144;
        const float SCALE = 1.8f;  // Scale to make it visible
        const float SCREEN_W = GB_WIDTH * SCALE;
        const float SCREEN_H = GB_HEIGHT * SCALE;
        
        ImGui::BeginChild("SpriteScreen", ImVec2(SCREEN_W + 10, SCREEN_H + 10), true);
        
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 screen_pos = ImGui::GetCursorScreenPos();
        
        // Draw screen background (dark)
        draw_list->AddRectFilled(
            screen_pos,
            ImVec2(screen_pos.x + SCREEN_W, screen_pos.y + SCREEN_H),
            IM_COL32(20, 20, 30, 255)
        );
        
        // Draw screen border
        draw_list->AddRect(
            screen_pos,
            ImVec2(screen_pos.x + SCREEN_W, screen_pos.y + SCREEN_H),
            IM_COL32(100, 100, 150, 255)
        );
        
        const u16 OAM_START = OAM_BASE;
        const int SPRITE_HEIGHT = large_sprites ? 16 : 8;
        
        int visible_count = 0;
        
        // Draw all sprites at their screen positions (reverse order for correct priority)
        for (int i = NUM_SPRITES - 1; i >= 0; --i) {
            u16 oam_addr = OAM_START + (i * 4);
            
            u8 y_pos = debugger_->read_memory(oam_addr);
            u8 x_pos = debugger_->read_memory(oam_addr + 1);
            u8 tile_num = debugger_->read_memory(oam_addr + 2);
            u8 attr = debugger_->read_memory(oam_addr + 3);
            
            // Convert to screen coordinates (OAM stores Y+16, X+8)
            int screen_x = static_cast<int>(x_pos) - SPRITE_WIDTH;
            int screen_y = static_cast<int>(y_pos) - 16;
            
            // Skip sprites completely off-screen
            if (screen_y <= -SPRITE_HEIGHT || screen_y >= GB_HEIGHT) continue;
            if (screen_x <= -SPRITE_WIDTH || screen_x >= GB_WIDTH) continue;
            
            visible_count++;
            
            // In 8x16 mode, bit 0 of tile number is ignored
            if (large_sprites) {
                tile_num &= TILE_NUM_MASK_8x16;
            }
            
            bool y_flip = (attr & ATTR_Y_FLIP) != 0;
            bool x_flip = (attr & ATTR_X_FLIP) != 0;
            bool use_obp1 = (attr & ATTR_PALETTE) != 0;
            u8 palette = use_obp1 ? obp1 : obp0;
            
            // Draw sprite pixels at screen position
            for (int tile_row = 0; tile_row < SPRITE_HEIGHT; ++tile_row) {
                int py = screen_y + tile_row;
                if (py < 0 || py >= GB_HEIGHT) continue;
                
                int actual_row = y_flip ? (SPRITE_HEIGHT - 1 - tile_row) : tile_row;
                
                // Calculate which tile and row within tile
                int tile_offset = (actual_row >= 8) ? 1 : 0;
                int row_in_tile = actual_row & 7;
                
                u8 current_tile = tile_num + tile_offset;
                u16 tile_addr = TILE_DATA + (current_tile * TILE_SIZE_BYTES) + (row_in_tile * TILE_BYTES_PER_ROW);
                
                u8 byte1 = debugger_->read_memory(tile_addr);
                u8 byte2 = debugger_->read_memory(tile_addr + 1);
                
                for (int px = 0; px < SPRITE_WIDTH; ++px) {
                    int pix_x = screen_x + px;
                    if (pix_x < 0 || pix_x >= GB_WIDTH) continue;
                    
                    int actual_px = x_flip ? px : (PIXELS_PER_TILE_ROW - 1 - px);
                    
                    int color_idx = ((byte1 >> actual_px) & 1) | (((byte2 >> actual_px) & 1) << 1);
                    
                    if (color_idx != 0) {  // Don't draw transparent pixels
                        ImU32 color = get_color(palette, color_idx);
                        
                        float draw_x = screen_pos.x + pix_x * SCALE;
                        float draw_y = screen_pos.y + py * SCALE;
                        
                        draw_list->AddRectFilled(
                            ImVec2(draw_x, draw_y),
                            ImVec2(draw_x + SCALE, draw_y + SCALE),
                            color
                        );
                    }
                }
            }
        }
        
        // Draw scanline indicator (current LY)
        u8 ly = debugger_->read_memory(REG_LY);
        if (ly < GB_HEIGHT) {
            float ly_y = screen_pos.y + ly * SCALE;
            draw_list->AddLine(
                ImVec2(screen_pos.x, ly_y),
                ImVec2(screen_pos.x + SCREEN_W, ly_y),
                IM_COL32(255, 100, 100, 150),
                1.0f
            );
        }
        
        ImGui::Dummy(ImVec2(SCREEN_W, SCREEN_H));
        ImGui::EndChild();
        
        // Info panel on the right
        ImGui::SameLine();
        ImGui::BeginChild("SpriteInfo", ImVec2(0, 0), true);
        
        ImGui::Text("Sprites on screen: %d", visible_count);
        ImGui::Text("Scanline (LY): %d", ly);
        ImGui::Separator();
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "-- Red line = LY");
        ImGui::Separator();
        
        // List visible sprites with their info
        ImGui::Text("Active sprites:");
        ImGui::BeginChild("SpriteList", ImVec2(0, 0), false);
        
        for (int i = 0; i < NUM_SPRITES; ++i) {
            u16 oam_addr = OAM_START + (i * 4);
            u8 y_pos = debugger_->read_memory(oam_addr);
            u8 x_pos = debugger_->read_memory(oam_addr + 1);
            
            int sx = static_cast<int>(x_pos) - 8;
            int sy = static_cast<int>(y_pos) - 16;
            
            // Only show on-screen sprites
            if (sy > -SPRITE_HEIGHT && sy < GB_HEIGHT && sx > -SPRITE_WIDTH && sx < GB_WIDTH) {
                u8 tile_num = debugger_->read_memory(oam_addr + 2);
                u8 attr = debugger_->read_memory(oam_addr + 3);
                bool use_obp1 = (attr & ATTR_PALETTE) != 0;
                ImGui::Text("#%02d (%3d,%3d) T:%02X %s", 
                    i, sx, sy, tile_num, use_obp1 ? "P1" : "P0");
            }
        }
        
        ImGui::EndChild();
        ImGui::EndChild();
    }
    ImGui::End();
}

} // namespace gbglow
