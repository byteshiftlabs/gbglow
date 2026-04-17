#include "gamepad.h"
#include "joypad.h"
#include <SDL2/SDL.h>
#include <fstream>
#include <sstream>
#include <algorithm>

namespace gbcrush {

Gamepad::Gamepad()
    : deadzone_(DEFAULT_DEADZONE)
    , left_stick_up_(false)
    , left_stick_down_(false)
    , left_stick_left_(false)
    , left_stick_right_(false)
{
    reset_default_mapping();
}

Gamepad::~Gamepad() {
    shutdown();
}

bool Gamepad::initialize() {
    // SDL_INIT_GAMECONTROLLER should already be initialized by Display
    // Open any controllers that are already connected
    int num_joysticks = SDL_NumJoysticks();
    for (int i = 0; i < num_joysticks; ++i) {
        if (SDL_IsGameController(i)) {
            on_controller_added(i);
        }
    }
    return true;
}

void Gamepad::shutdown() {
    for (auto& pair : controllers_) {
        if (pair.second) {
            SDL_GameControllerClose(pair.second);
        }
    }
    controllers_.clear();
}

void Gamepad::on_controller_added(int device_index) {
    if (!SDL_IsGameController(device_index)) {
        return;
    }
    
    SDL_GameController* controller = SDL_GameControllerOpen(device_index);
    if (controller) {
        SDL_Joystick* joystick = SDL_GameControllerGetJoystick(controller);
        int instance_id = SDL_JoystickInstanceID(joystick);
        controllers_[instance_id] = controller;
        
        const char* name = SDL_GameControllerName(controller);
        SDL_Log("Gamepad connected: %s (instance %d)", 
                name ? name : "Unknown", instance_id);
    }
}

void Gamepad::on_controller_removed(int instance_id) {
    auto it = controllers_.find(instance_id);
    if (it != controllers_.end()) {
        const char* name = SDL_GameControllerName(it->second);
        SDL_Log("Gamepad disconnected: %s (instance %d)",
                name ? name : "Unknown", instance_id);
        
        SDL_GameControllerClose(it->second);
        controllers_.erase(it);
    }
}

void Gamepad::on_button_down(int instance_id, int button, Joypad* joypad) {
    (void)instance_id;  // Reserved for multi-controller support
    if (!joypad) return;
    
    // D-Pad buttons
    if (button == SDL_CONTROLLER_BUTTON_DPAD_UP) {
        joypad->press_up();
    } else if (button == SDL_CONTROLLER_BUTTON_DPAD_DOWN) {
        joypad->press_down();
    } else if (button == SDL_CONTROLLER_BUTTON_DPAD_LEFT) {
        joypad->press_left();
    } else if (button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT) {
        joypad->press_right();
    }
    // Mapped buttons
    else if (button == button_mapping_.gb_a) {
        joypad->press_a();
    } else if (button == button_mapping_.gb_b) {
        joypad->press_b();
    } else if (button == button_mapping_.gb_start) {
        joypad->press_start();
    } else if (button == button_mapping_.gb_select) {
        joypad->press_select();
    }
}

void Gamepad::on_button_up(int instance_id, int button, Joypad* joypad) {
    (void)instance_id;  // Reserved for multi-controller support
    if (!joypad) return;
    
    // D-Pad buttons
    if (button == SDL_CONTROLLER_BUTTON_DPAD_UP) {
        joypad->release_up();
    } else if (button == SDL_CONTROLLER_BUTTON_DPAD_DOWN) {
        joypad->release_down();
    } else if (button == SDL_CONTROLLER_BUTTON_DPAD_LEFT) {
        joypad->release_left();
    } else if (button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT) {
        joypad->release_right();
    }
    // Mapped buttons
    else if (button == button_mapping_.gb_a) {
        joypad->release_a();
    } else if (button == button_mapping_.gb_b) {
        joypad->release_b();
    } else if (button == button_mapping_.gb_start) {
        joypad->release_start();
    } else if (button == button_mapping_.gb_select) {
        joypad->release_select();
    }
}

void Gamepad::on_axis_motion(int instance_id, int axis, int value, Joypad* joypad) {
    (void)instance_id;  // Reserved for multi-controller support
    if (!joypad) return;
    
    // Only handle left stick axes
    if (axis == SDL_CONTROLLER_AXIS_LEFTX || axis == SDL_CONTROLLER_AXIS_LEFTY) {
        update_stick_state(axis, value, joypad);
    }
}

void Gamepad::update_stick_state(int axis, int value, Joypad* joypad) {
    if (axis == SDL_CONTROLLER_AXIS_LEFTX) {
        // Horizontal axis: negative = left, positive = right
        bool new_left = (value < -deadzone_);
        bool new_right = (value > deadzone_);
        
        if (new_left != left_stick_left_) {
            left_stick_left_ = new_left;
            if (new_left) {
                joypad->press_left();
            } else {
                joypad->release_left();
            }
        }
        
        if (new_right != left_stick_right_) {
            left_stick_right_ = new_right;
            if (new_right) {
                joypad->press_right();
            } else {
                joypad->release_right();
            }
        }
    } else if (axis == SDL_CONTROLLER_AXIS_LEFTY) {
        // Vertical axis: negative = up, positive = down
        bool new_up = (value < -deadzone_);
        bool new_down = (value > deadzone_);
        
        if (new_up != left_stick_up_) {
            left_stick_up_ = new_up;
            if (new_up) {
                joypad->press_up();
            } else {
                joypad->release_up();
            }
        }
        
        if (new_down != left_stick_down_) {
            left_stick_down_ = new_down;
            if (new_down) {
                joypad->press_down();
            } else {
                joypad->release_down();
            }
        }
    }
}

bool Gamepad::is_connected() const {
    return !controllers_.empty();
}

int Gamepad::get_controller_count() const {
    return static_cast<int>(controllers_.size());
}

std::string Gamepad::get_controller_name(int index) const {
    if (index < 0 || index >= static_cast<int>(controllers_.size())) {
        return "";
    }
    
    auto it = controllers_.begin();
    std::advance(it, index);
    
    const char* name = SDL_GameControllerName(it->second);
    return name ? name : "Unknown Controller";
}

std::vector<std::string> Gamepad::get_controller_names() const {
    std::vector<std::string> names;
    for (const auto& pair : controllers_) {
        const char* name = SDL_GameControllerName(pair.second);
        names.push_back(name ? name : "Unknown Controller");
    }
    return names;
}

const Gamepad::ButtonMapping& Gamepad::get_button_mapping() const {
    return button_mapping_;
}

void Gamepad::set_button_mapping(const ButtonMapping& mapping) {
    button_mapping_ = mapping;
}

void Gamepad::reset_default_mapping() {
    // Default: Xbox-style layout
    // A (bottom) -> GB A, B (right) -> GB B
    // This feels natural for most games
    button_mapping_.gb_a = SDL_CONTROLLER_BUTTON_A;
    button_mapping_.gb_b = SDL_CONTROLLER_BUTTON_B;
    button_mapping_.gb_start = SDL_CONTROLLER_BUTTON_START;
    button_mapping_.gb_select = SDL_CONTROLLER_BUTTON_BACK;
}

int Gamepad::get_deadzone() const {
    return deadzone_;
}

void Gamepad::set_deadzone(int deadzone) {
    deadzone_ = std::clamp(deadzone, 0, 32767);
}

std::string Gamepad::button_to_string(int button) {
    switch (button) {
        case SDL_CONTROLLER_BUTTON_A: return "A";
        case SDL_CONTROLLER_BUTTON_B: return "B";
        case SDL_CONTROLLER_BUTTON_X: return "X";
        case SDL_CONTROLLER_BUTTON_Y: return "Y";
        case SDL_CONTROLLER_BUTTON_BACK: return "BACK";
        case SDL_CONTROLLER_BUTTON_GUIDE: return "GUIDE";
        case SDL_CONTROLLER_BUTTON_START: return "START";
        case SDL_CONTROLLER_BUTTON_LEFTSTICK: return "LEFTSTICK";
        case SDL_CONTROLLER_BUTTON_RIGHTSTICK: return "RIGHTSTICK";
        case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: return "LB";
        case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return "RB";
        default: return "UNKNOWN";
    }
}

int Gamepad::string_to_button(const std::string& str) {
    if (str == "A") return SDL_CONTROLLER_BUTTON_A;
    if (str == "B") return SDL_CONTROLLER_BUTTON_B;
    if (str == "X") return SDL_CONTROLLER_BUTTON_X;
    if (str == "Y") return SDL_CONTROLLER_BUTTON_Y;
    if (str == "BACK") return SDL_CONTROLLER_BUTTON_BACK;
    if (str == "GUIDE") return SDL_CONTROLLER_BUTTON_GUIDE;
    if (str == "START") return SDL_CONTROLLER_BUTTON_START;
    if (str == "LEFTSTICK") return SDL_CONTROLLER_BUTTON_LEFTSTICK;
    if (str == "RIGHTSTICK") return SDL_CONTROLLER_BUTTON_RIGHTSTICK;
    if (str == "LB") return SDL_CONTROLLER_BUTTON_LEFTSHOULDER;
    if (str == "RB") return SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;
    return SDL_CONTROLLER_BUTTON_INVALID;
}

void Gamepad::load_config(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        // Parse key=value
        size_t eq_pos = line.find('=');
        if (eq_pos == std::string::npos) {
            continue;
        }
        
        std::string key = line.substr(0, eq_pos);
        std::string value = line.substr(eq_pos + 1);
        
        // Trim whitespace
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);
        
        // Parse gamepad settings
        if (key == "gamepad_a") {
            button_mapping_.gb_a = string_to_button(value);
        } else if (key == "gamepad_b") {
            button_mapping_.gb_b = string_to_button(value);
        } else if (key == "gamepad_start") {
            button_mapping_.gb_start = string_to_button(value);
        } else if (key == "gamepad_select") {
            button_mapping_.gb_select = string_to_button(value);
        } else if (key == "gamepad_deadzone") {
            deadzone_ = std::stoi(value);
        }
    }
}

void Gamepad::save_config(const std::string& path) {
    std::ofstream file(path);
    if (!file.is_open()) {
        return;
    }
    
    file << "# Gamepad Button Mappings\n";
    file << "# Available buttons: A, B, X, Y, BACK, START, LB, RB, LEFTSTICK, RIGHTSTICK\n";
    file << "#\n";
    file << "gamepad_a=" << button_to_string(button_mapping_.gb_a) << "\n";
    file << "gamepad_b=" << button_to_string(button_mapping_.gb_b) << "\n";
    file << "gamepad_start=" << button_to_string(button_mapping_.gb_start) << "\n";
    file << "gamepad_select=" << button_to_string(button_mapping_.gb_select) << "\n";
    file << "gamepad_deadzone=" << deadzone_ << "\n";
}

} // namespace gbcrush
