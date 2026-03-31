// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 gbglow Contributors
// This file is part of gbglow. See LICENSE for details.

#pragma once

#include "../core/constants.h"
#include "../core/types.h"
#include <array>
#include <vector>
#include <string>
#include <map>
#include <memory>
#include <ctime>

// Forward declare SDL types to avoid including SDL in header
struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;

// Forward declare ImGui context
struct ImGuiContext;

namespace gbglow {

class Joypad;
class Gamepad;
class Debugger;
class DebuggerGUI;
class RecentRoms;
class Screenshot;

/**
 * Display Output System
 * 
 * Handles rendering Game Boy LCD output to window using SDL2
 * 
 * Game Boy LCD specifications:
 * - Resolution: 160×144 pixels
 * - Refresh rate: ~59.73 Hz
 * - 4 shades of gray (DMG mode)
 * 
 * This class:
 * - Creates SDL window and renderer
 * - Scales Game Boy framebuffer to window
 * - Handles display refresh
 * - Manages window events
 */
class Display {
public:
    Display();
    ~Display();
    
    // Disable copy/move
    Display(const Display&) = delete;
    Display& operator=(const Display&) = delete;
    Display(Display&&) = delete;
    Display& operator=(Display&&) = delete;
    
    /**
     * Initialize SDL and create window
     * Returns true on success, false on failure
     */
    bool initialize(const std::string& title, int scale_factor);
    
    /**
     * Attach debugger for GUI rendering
     */
    void attach_debugger(Debugger& debugger);
    
    /**
     * Check if debugger UI is currently visible.
     */
    bool debugger_visible() const;

    /**
     * Check if debugger requested emulation pause.
     */
    bool debugger_should_pause() const;

    /**
     * Check if the debugger requested a single step.
     */
    bool debugger_step_requested() const;

    /**
     * Clear the debugger step request after handling it.
     */
    void clear_debugger_step_request();

    /**
     * Check if the debugger requested continue.
     */
    bool debugger_continue_requested() const;

    /**
     * Clear the debugger continue request after handling it.
     */
    void clear_debugger_continue_request();

    /**
     * Pause the debugger without exposing GUI internals.
     */
    void pause_debugger();
    
    /**
     * Check if in debugger mode (full window with docking)
     */
    bool is_debugger_mode() const;
    
    /**
     * Enter/exit debugger mode (resizes window)
     */
    void set_debugger_mode(bool enabled);
    
    /**
     * Update display with framebuffer data
     * Framebuffer is 160×144 RGBA pixels
     */
    void update(const std::vector<u8>& framebuffer);
    
    /**
     * Check if a ROM should be loaded (from menu)
     */
    bool has_pending_rom() const;
    
    /**
     * Get and clear pending ROM path
     */
    std::string get_pending_rom();
    
    /**
        * Bind per-ROM UI context managed by the emulator.
        */
        void bind_session_context(const std::string& rom_path, RecentRoms& recent_roms);

        /**
     * Check if emulator should reset
     */
    bool should_reset() const;
    
    /**
     * Clear reset flag
     */
    void clear_reset_flag();
    
    /**
     * Check if emulator should pause
     */
    bool is_paused() const;
    
    /**
     * Toggle pause state
     */
    void toggle_pause();
    
    /**
     * Get current speed multiplier
     */
    float get_speed_multiplier() const;
    
    /**
     * Check if save state was requested
     */
    int get_save_state_slot() const;
    
    /**
     * Check if load state was requested
     */
    int get_load_state_slot() const;
    
    /**
     * Check if delete state was requested
     */
    int get_delete_state_slot() const;
    
    /**
     * Clear save/load/delete state request
     */
    void clear_state_request();
    
    /**
     * Update slot metadata after save
     */
    void update_slot_metadata(int slot);
    
    /**
     * Clear slot metadata after delete
     */
    void delete_slot_metadata(int slot);
    
    /**
     * Queue audio samples for playback
     * @param samples Vector of stereo samples (left, right) as 8-bit unsigned values (0-255)
     */
    void queue_audio(const std::vector<std::pair<u8, u8>>& samples);
    
    /**
     * Get the size of the audio queue in bytes
     * Used for audio-based synchronization
     */
    unsigned int get_audio_queue_size() const;
    
    /**
     * Clear the audio queue (used for turbo mode)
     */
    void clear_audio_queue();
    
    /**
     * Check if window should close
     */
    bool should_close() const;
    
    /**
     * Check if turbo mode is active (Space key held)
     */
    bool is_turbo_active() const;
    
    /**
     * Process window events (must be called each frame)
     * If joypad is provided, keyboard input will be routed to it
     */
    void poll_events(Joypad* joypad = nullptr);
    
    /**
     * Get window dimensions
     */
    int width() const;
    int height() const;
    
    /**
     * Toggle audio mute
     */
    void toggle_mute();
    
    /**
     * Check if audio is muted
     */
    bool is_muted() const;
    
    /**
     * Check if screenshot was requested
     */
    bool screenshot_requested() const;
    
    /**
     * Clear screenshot request flag
     */
    void clear_screenshot_request();
    
    /**
     * Capture screenshot to file
     * @param framebuffer RGBA framebuffer data
     * @param rom_path Path to currently loaded ROM
     */
    void capture_screenshot(const std::vector<u8>& framebuffer, const std::string& rom_path);
    
private:
    SDL_Window* window_;
    SDL_Renderer* renderer_;
    SDL_Texture* texture_;
    unsigned int audio_device_;  // SDL_AudioDeviceID is typedef'd to Uint32
    ImGuiContext* imgui_context_;
    
    // Gamepad support
    std::unique_ptr<Gamepad> gamepad_;
    
    // Debugger GUI
    std::unique_ptr<DebuggerGUI> debugger_gui_;
    bool debugger_mode_;  // True when in full debugger view
    int original_width_;  // Store original window size
    int original_height_;
    
    // Recent ROMs (not owned, just a pointer)
    RecentRoms* recent_roms_;
    
    // Screenshot support
    std::unique_ptr<Screenshot> screenshot_;
    bool screenshot_requested_;
    
    bool should_close_;
    bool turbo_mode_;  // Space key held for speedup
    int scale_factor_;
    bool is_muted_;  // Audio muted flag
    
    // Menu state
    std::string pending_rom_path_;
    bool should_reset_;
    bool is_paused_;
    bool show_about_dialog_;
    bool show_controller_config_;
    bool open_rom_dialog_requested_;
    bool open_rom_dialog_visible_;
    bool close_open_rom_dialog_requested_;
    float speed_multiplier_;  // 0.5, 1.0, 2.0, 4.0
    int save_state_slot_;  // -1 = none, 0-9 = save to slot
    int load_state_slot_;  // -1 = none, 0-9 = load from slot
    int delete_state_slot_;  // -1 = none, 0-9 = delete slot
    std::array<char, constants::display::kOpenRomPathBufferSize> open_rom_path_buffer_;
    std::string open_rom_error_message_;
    
    // Key bindings
    struct KeyBindings {
        int up;
        int down;
        int left;
        int right;
        int a;
        int b;
        int start;
        int select;
    };
    KeyBindings key_bindings_;
    int waiting_for_key_;  // -1 = not waiting, 0-9 = waiting for specific binding
    enum WaitingKeyIndex {
        KEY_UP = 0,
        KEY_DOWN = 1,
        KEY_LEFT = 2,
        KEY_RIGHT = 3,
        KEY_A = 4,
        KEY_B = 5,
        KEY_START = 6,
        KEY_SELECT = 7
    };
    
    // Slot metadata tracking
    struct SlotMetadata {
        bool used;
        time_t timestamp;
    };
    std::map<int, SlotMetadata> slot_metadata_;
    std::string current_rom_path_;  // Track current ROM path for slot labels
    
    // Game Boy LCD dimensions
    static constexpr int LCD_WIDTH = constants::display::kLcdWidth;
    static constexpr int LCD_HEIGHT = constants::display::kLcdHeight;
    static constexpr int BYTES_PER_PIXEL = constants::display::kBytesPerPixel;
    
    // Default scale factor
    static constexpr int DEFAULT_SCALE = constants::display::kDefaultScale;
    
    // Debugger mode window dimensions
    static constexpr int DEBUGGER_WINDOW_WIDTH = constants::display::kDebuggerWindowWidth;
    static constexpr int DEBUGGER_WINDOW_HEIGHT = constants::display::kDebuggerWindowHeight;
    
    // Audio conversion constants (U8 hardware format to S16)
    static constexpr int AUDIO_U8_MIDPOINT = constants::display::kAudioU8Midpoint;
    static constexpr int AUDIO_U8_SCALE = constants::display::kAudioU8Scale;
    
    // SDL clear color (opaque black)
    static constexpr u8 CLEAR_COLOR_R = constants::display::kClearColorR;
    static constexpr u8 CLEAR_COLOR_G = constants::display::kClearColorG;
    static constexpr u8 CLEAR_COLOR_B = constants::display::kClearColorB;
    static constexpr u8 CLEAR_COLOR_A = constants::display::kClearColorA;
    
    // Savestate slot label buffer size
    static constexpr size_t SLOT_LABEL_SIZE = constants::display::kSlotLabelSize;
    static constexpr size_t OPEN_ROM_PATH_BUFFER_SIZE = constants::display::kOpenRomPathBufferSize;
    
    /**
     * Convert SDL error to readable message
     */
    static std::string get_sdl_error();
    
    /**
     * Get path to keybindings config file (XDG-compliant)
     */
    static std::string get_keybindings_path();

    /**
     * Get path to gamepad config file (XDG-compliant)
     */
    static std::string get_gamepad_config_path();

    /**
     * Load key bindings from config file
     */
    void load_key_bindings();
    
    /**
     * Save key bindings to config file
     */
    void save_key_bindings();
    
    /**
     * Parse SDL keycode from string (e.g., "SDLK_a" -> SDLK_a)
     */
    static int parse_sdl_keycode(const std::string& keyname);
    
    /**
     * Convert SDL keycode to string (e.g., SDLK_a -> "SDLK_a")
     */
    static std::string sdl_keycode_to_string(int keycode);
    
    /**
     * Handle keyboard press events
     */
    void handle_keydown(int key, int modifiers, Joypad* joypad);

    /**
     * Check whether a shortcut should still be handled while ImGui owns the keyboard
     */
    bool is_global_shortcut(int key, int modifiers) const;
    
    /**
     * Handle keyboard release events
     */
    void handle_keyup(int key, Joypad* joypad);
    
    /**
     * Render ImGui menu bar
     */
    void render_menu_bar();

    /**
     * Check if a save-state slot exists on disk.
     */
    static bool check_slot_exists(const std::string& state_path);

    /**
     * Build a human-readable slot label for the current ROM.
     */
    std::string get_slot_label(int slot, const std::string& rom_path) const;

    /**
     * Enter debugger mode and synchronize debugger UI state.
     */
    void open_debugger_mode();

    /**
     * Exit debugger mode and synchronize debugger UI state.
     */
    void close_debugger_mode();

    /**
     * Toggle debugger mode on/off.
     */
    void toggle_debugger_mode();
    
    /**
     * Render about dialog
     */
    void render_about_dialog();

    /**
     * Queue the Open ROM modal for display
     */
    void request_open_rom_dialog();

    /**
     * Render the Open ROM modal dialog
     */
    void render_open_rom_dialog();

    /**
     * Open a native file picker if one is available
     */
    static std::string browse_for_rom_file(std::string& error_message);
    
    /**
     * Render controller configuration dialog
     */
    void render_controller_config();
};

} // namespace gbglow
