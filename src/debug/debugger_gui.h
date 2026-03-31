// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 gbglow Contributors
// This file is part of gbglow. See LICENSE for details.

#pragma once

#include "../core/types.h"
#include <string>

namespace gbglow {

// Forward declarations
class Debugger;
class CPU;
class Memory;
class PPU;

/**
 * Debugger GUI using ImGui
 * 
 * Renders debugger windows:
 * - CPU Registers
 * - Disassembly
 * - Memory Viewer
 * - Breakpoints
 * - Watches
 */
class DebuggerGUI {
public:
    DebuggerGUI();
    ~DebuggerGUI() = default;
    DebuggerGUI(const DebuggerGUI&) = delete;
    DebuggerGUI& operator=(const DebuggerGUI&) = delete;
    DebuggerGUI(DebuggerGUI&&) = delete;
    DebuggerGUI& operator=(DebuggerGUI&&) = delete;
    
    /**
     * Attach to debugger instance
     */
    void attach(Debugger& debugger);
    
    /**
     * Render all debugger windows
     * Call this within ImGui render loop
     */
    void render();
    
    /**
     * Toggle debugger visibility
     */
    void toggle_visible();
    
    /**
     * Check if debugger is visible
     */
    bool is_visible() const;
    
    /**
     * Set visibility
     */
    void set_visible(bool visible);
    
    /**
     * Check if emulator should be paused (debugger controls)
     */
    bool should_pause() const;
    
    /**
     * Check if step was requested
     */
    bool step_requested() const;
    
    /**
     * Clear step request
     */
    void clear_step_request();

    /**
     * Request a single-instruction step
     */
    void request_step();

    /**
     * Request step over for the current instruction when possible.
     */
    void request_step_over();
    
    /**
     * Signal that execution should continue
     */
    void continue_execution();
    
    /**
     * Check if execution should continue
     */
    bool should_continue() const;
    
    /**
     * Clear continue flag
     */
    void clear_continue();

    /**
     * Clear any pending execution requests.
     */
    void clear_execution_requests();

    /**
     * Pause execution without exposing direct state mutation.
     */
    void pause_execution();
    
    /**
     * Set docking mode (when true, doesn't render its own menu bar)
     */
    void set_docking_mode(bool enabled);
    
    /**
        * Render window visibility toggles for an external Debug menu.
     */
        void render_window_menu_items();
    
    /**
     * Get pause state
     */
    bool is_paused() const { return paused_; }
    
    /**
     * Apply debug color theme
     */
    static void apply_debug_theme();
    
private:
    Debugger* debugger_;
    bool visible_;
    bool paused_;
    bool step_requested_;
    bool continue_requested_;
    bool docking_mode_;
    
    // Window visibility flags
    bool show_registers_;
    bool show_disassembly_;
    bool show_memory_;
    bool show_breakpoints_;
    bool show_watches_;
    bool show_stack_;
    bool show_io_registers_;
    bool show_sprites_;
    
    // Memory viewer state
    u16 memory_view_address_;
    int memory_view_columns_;
    char memory_goto_address_[8];
    
    // Disassembly state
    bool follow_pc_;
    u16 disasm_address_;
    char disasm_goto_address_[8];
    
    // Breakpoint input
    char breakpoint_address_[8];
    
    // Watch input
    char watch_address_[8];
    char watch_label_[32];
    bool watch_break_on_change_;
    
    // Rendering methods
    void render_menu_bar();
    void render_registers_window();
    void render_disassembly_window();
    void render_memory_window();
    void render_breakpoints_window();
    void render_watches_window();
    void render_stack_window();
    void render_io_registers_window();
    void render_sprites_window();
    
    // Helper methods
    static std::string format_flags(u8 f);
    static void render_register_row(const char* name, u16 value, bool highlight = false);
    static void render_register_row_8(const char* name, u8 value, bool highlight = false);
};

} // namespace gbglow
