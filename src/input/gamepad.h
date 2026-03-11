// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 gbglow Contributors
// This file is part of gbglow. See LICENSE for details.

#pragma once

#include "../core/types.h"
#include <string>
#include <vector>
#include <map>

// Forward declare SDL types
struct _SDL_GameController;
typedef struct _SDL_GameController SDL_GameController;

namespace gbglow {

class Joypad;

/**
 * Gamepad Input Handler
 * 
 * Manages game controller input using SDL2 GameController API.
 * Supports standard mappings for Xbox, PlayStation, and Nintendo controllers.
 * 
 * Button mapping (default):
 *   Controller A/Cross     -> Game Boy A
 *   Controller B/Circle    -> Game Boy B
 *   Controller Start       -> Game Boy Start
 *   Controller Back/Select -> Game Boy Select
 *   D-Pad / Left Stick     -> Game Boy D-Pad
 */
class Gamepad {
public:
    Gamepad();
    ~Gamepad();
    
    // SDL axis maximum value
    static constexpr int SDL_AXIS_MAX = 32767;
    
    // Disable copy
    Gamepad(const Gamepad&) = delete;
    Gamepad& operator=(const Gamepad&) = delete;
    
    /**
     * Initialize gamepad subsystem
     * Must be called after SDL_Init(SDL_INIT_GAMECONTROLLER)
     */
    bool initialize();
    
    /**
     * Shutdown and release all controllers
     */
    void shutdown();
    
    /**
     * Handle controller connected event
     * @param device_index SDL device index from event
     */
    void on_controller_added(int device_index);
    
    /**
     * Handle controller disconnected event
     * @param instance_id SDL joystick instance ID from event
     */
    void on_controller_removed(int instance_id);
    
    /**
     * Handle controller button press
     * @param instance_id Controller instance ID
     * @param button SDL_GameControllerButton value
     * @param joypad Joypad to send input to
     */
    void on_button_down(int instance_id, int button, Joypad* joypad);
    
    /**
     * Handle controller button release
     * @param instance_id Controller instance ID
     * @param button SDL_GameControllerButton value
     * @param joypad Joypad to send input to
     */
    void on_button_up(int instance_id, int button, Joypad* joypad);
    
    /**
     * Handle controller axis motion
     * @param instance_id Controller instance ID
     * @param axis SDL_GameControllerAxis value
     * @param value Axis value (-32768 to 32767)
     * @param joypad Joypad to send input to
     */
    void on_axis_motion(int instance_id, int axis, int value, Joypad* joypad);
    
    /**
     * Check if any controller is connected
     */
    bool is_connected() const;
    
    /**
     * Get number of connected controllers
     */
    int get_controller_count() const;
    
    /**
     * Get name of connected controller
     * @param index Controller index (0 to count-1)
     * @return Controller name or empty string if invalid
     */
    std::string get_controller_name(int index) const;
    
    /**
     * Get all connected controller names
     */
    std::vector<std::string> get_controller_names() const;
    
    // Button mapping configuration
    struct ButtonMapping {
        int gb_a;       // SDL_GameControllerButton for GB A
        int gb_b;       // SDL_GameControllerButton for GB B
        int gb_start;   // SDL_GameControllerButton for GB Start
        int gb_select;  // SDL_GameControllerButton for GB Select
    };
    
    /**
     * Get current button mapping
     */
    const ButtonMapping& get_button_mapping() const;
    
    /**
     * Set button mapping
     */
    void set_button_mapping(const ButtonMapping& mapping);
    
    /**
     * Reset to default button mapping
     */
    void reset_default_mapping();
    
    /**
     * Get/set analog stick deadzone (0-32767)
     */
    int get_deadzone() const;
    void set_deadzone(int deadzone);
    
    /**
     * Load gamepad configuration from file
     */
    void load_config(const std::string& path);
    
    /**
     * Save gamepad configuration to file
     */
    void save_config(const std::string& path);

private:
    // Connected controllers: instance_id -> SDL_GameController*
    std::map<int, SDL_GameController*> controllers_;
    
    // Button mapping
    ButtonMapping button_mapping_;
    
    // Analog stick deadzone
    int deadzone_;
    
    // Track analog stick state for digital conversion
    bool left_stick_up_;
    bool left_stick_down_;
    bool left_stick_left_;
    bool left_stick_right_;
    
    // Default deadzone (about 25% of full range)
    static constexpr int DEFAULT_DEADZONE = 8000;
    
    // Helper to convert axis value to digital state
    void update_stick_state(int axis, int value, Joypad* joypad);
    
    // Get button name for config file
    static std::string button_to_string(int button);
    static int string_to_button(const std::string& str);
};

} // namespace gbglow
