// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 gbglow Contributors
// This file is part of gbglow. See LICENSE for details.

#include "display.h"
#include "../core/constants.h"
#include "../input/joypad.h"
#include "../input/gamepad.h"
#include "../debug/debugger.h"
#include "../debug/debugger_gui.h"
#include "../ui/recent_roms.h"
#include "../ui/screenshot.h"
#include "../core/platform.h"
#include <SDL2/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <algorithm>
#include <limits>
#include <sys/wait.h>

namespace gbglow {

using namespace constants::application;
using namespace constants::emulator;
using namespace constants::savestate;

namespace {

struct CommandResult {
    std::string output;
    int exit_code;
    bool launched;
};

void trim_in_place(std::string& value) {
    const size_t first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        value.clear();
        return;
    }

    const size_t last = value.find_last_not_of(" \t\r\n");
    value = value.substr(first, last - first + 1);
}

bool write_text_file_atomically(const std::filesystem::path& path, const std::string& contents) {
    const std::filesystem::path temp_path = path.string() + ".tmp";

    {
        std::ofstream file(temp_path, std::ios::trunc);
        if (!file.is_open()) {
            return false;
        }

        file << contents;
        if (!file.good()) {
            file.close();
            std::error_code remove_error;
            std::filesystem::remove(temp_path, remove_error);
            return false;
        }
    }

    std::error_code error;
    std::filesystem::rename(temp_path, path, error);
    if (!error) {
        return true;
    }

    std::error_code remove_error;
    std::filesystem::remove(temp_path, remove_error);
    return false;
}

bool is_executable_file(const std::filesystem::path& path) {
    std::error_code error;
    const auto status = std::filesystem::status(path, error);
    if (error || !std::filesystem::exists(status)) {
        return false;
    }

    const auto perms = status.permissions();
    return (perms & std::filesystem::perms::owner_exec) != std::filesystem::perms::none ||
           (perms & std::filesystem::perms::group_exec) != std::filesystem::perms::none ||
           (perms & std::filesystem::perms::others_exec) != std::filesystem::perms::none;
}

bool command_exists(const char* command_name) {
    const char* path_env = std::getenv("PATH");
    if (!path_env || !*path_env) {
        return false;
    }

    std::stringstream path_stream(path_env);
    std::string path_entry;
    while (std::getline(path_stream, path_entry, ':')) {
        if (path_entry.empty()) {
            continue;
        }

        if (is_executable_file(std::filesystem::path(path_entry) / command_name)) {
            return true;
        }
    }

    return false;
}

CommandResult run_command_and_capture_stdout(const char* command) {
    FILE* pipe = popen(command, "r");
    if (!pipe) {
        return {{}, -1, false};
    }

    std::string output;
    char buffer[512];
    while (std::fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output += buffer;
    }

    const int wait_status = pclose(pipe);
    int exit_code = wait_status;
    if (wait_status != -1 && WIFEXITED(wait_status)) {
        exit_code = WEXITSTATUS(wait_status);
    }

    while (!output.empty() && (output.back() == '\n' || output.back() == '\r')) {
        output.pop_back();
    }

    return {output, exit_code, true};
}

}  // namespace

Display::Display()
    : window_(nullptr)
    , renderer_(nullptr)
    , texture_(nullptr)
    , audio_device_(0)
    , imgui_context_(nullptr)
    , gamepad_(std::make_unique<Gamepad>())
    , debugger_gui_(std::make_unique<DebuggerGUI>())
    , debugger_mode_(false)
    , original_width_(0)
    , original_height_(0)
    , recent_roms_(nullptr)
    , screenshot_(std::make_unique<Screenshot>())
    , screenshot_requested_(false)
    , should_close_(false)
    , turbo_mode_(false)
    , scale_factor_(DEFAULT_SCALE)
    , is_muted_(false)
    , should_reset_(false)
    , is_paused_(false)
    , show_about_dialog_(false)
    , show_controller_config_(false)
    , open_rom_dialog_requested_(false)
    , open_rom_dialog_visible_(false)
    , close_open_rom_dialog_requested_(false)
    , speed_multiplier_(1.0f)
    , save_state_slot_(-1)
    , load_state_slot_(-1)
    , delete_state_slot_(-1)
    , waiting_for_key_(-1)
{
    open_rom_path_buffer_.fill('\0');

    // Initialize default key bindings
    key_bindings_.up = SDLK_UP;
    key_bindings_.down = SDLK_DOWN;
    key_bindings_.left = SDLK_LEFT;
    key_bindings_.right = SDLK_RIGHT;
    key_bindings_.a = SDLK_z;
    key_bindings_.b = SDLK_x;
    key_bindings_.start = SDLK_RETURN;
    key_bindings_.select = SDLK_LSHIFT;
    
    // Load key bindings from config file (will override defaults if found)
    load_key_bindings();
    
    // Load gamepad configuration
    gamepad_->load_config(get_gamepad_config_path());
}

Display::~Display() {
    // Shut down gamepad first — it calls SDL_GameControllerClose(),
    // which must happen before SDL_Quit().
    if (gamepad_) {
        gamepad_.reset();
    }

    // Clean up ImGui
    if (imgui_context_) {
        ImGui_ImplSDLRenderer2_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext(imgui_context_);
        imgui_context_ = nullptr;
    }
    
    // Clean up SDL resources in reverse order of creation
    if (audio_device_) {
        SDL_CloseAudioDevice(audio_device_);
        audio_device_ = 0;
    }
    
    if (texture_) {
        SDL_DestroyTexture(texture_);
        texture_ = nullptr;
    }
    
    if (renderer_) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }
    
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    
    SDL_Quit();
}

void Display::attach_debugger(Debugger& debugger) {
    if (debugger_gui_) {
        debugger_gui_->attach(debugger);
    }
}

bool Display::debugger_visible() const {
    return debugger_gui_ && debugger_gui_->is_visible();
}

bool Display::debugger_should_pause() const {
    return debugger_gui_ && debugger_gui_->should_pause();
}

bool Display::debugger_step_requested() const {
    return debugger_gui_ && debugger_gui_->step_requested();
}

void Display::clear_debugger_step_request() {
    if (debugger_gui_) {
        debugger_gui_->clear_step_request();
    }
}

bool Display::debugger_continue_requested() const {
    return debugger_gui_ && debugger_gui_->should_continue();
}

void Display::clear_debugger_continue_request() {
    if (debugger_gui_) {
        debugger_gui_->clear_continue();
    }
}

void Display::pause_debugger() {
    if (debugger_gui_) {
        debugger_gui_->pause_execution();
    }
}

bool Display::is_debugger_mode() const {
    return debugger_mode_;
}

void Display::set_debugger_mode(bool enabled) {
    if (debugger_mode_ == enabled) {
        return;
    }
    
    debugger_mode_ = enabled;
    
    if (enabled) {
        // Store original size
        SDL_GetWindowSize(window_, &original_width_, &original_height_);
        
        // Resize to debugger mode (larger window for docking layout)
        // 1280x800 gives good space for emulator + debug panels
        SDL_SetWindowSize(window_, DEBUGGER_WINDOW_WIDTH, DEBUGGER_WINDOW_HEIGHT);
        SDL_SetWindowPosition(window_, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        
        // Recreate texture at original emulator size (it will be rendered inside ImGui)
        // Texture stays the same size - just the window is bigger
    } else {
        // Restore original size
        if (original_width_ > 0 && original_height_ > 0) {
            SDL_SetWindowSize(window_, original_width_, original_height_);
            SDL_SetWindowPosition(window_, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        }
    }
}

void Display::open_debugger_mode() {
    if (!debugger_gui_) {
        return;
    }

    set_debugger_mode(true);
    debugger_gui_->set_visible(true);
    debugger_gui_->set_docking_mode(true);
}

void Display::close_debugger_mode() {
    if (!debugger_gui_) {
        return;
    }

    set_debugger_mode(false);
    debugger_gui_->set_visible(false);
    debugger_gui_->set_docking_mode(false);
}

void Display::toggle_debugger_mode() {
    if (debugger_mode_) {
        close_debugger_mode();
    } else {
        open_debugger_mode();
    }
}

bool Display::initialize(const std::string& title, int scale_factor) {
    scale_factor_ = scale_factor;
    
    // Initialize SDL video, audio, and game controller subsystems
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) < 0) {
        return false;
    }
    
    // Initialize gamepad support
    gamepad_->initialize();
    
    // Hint: Don't throttle when window is unfocused (allows background execution)
    SDL_SetHint(SDL_HINT_VIDEO_ALLOW_SCREENSAVER, "1");
    
    // Calculate window dimensions based on scale factor
    const int window_width = LCD_WIDTH * scale_factor_;
    const int window_height = LCD_HEIGHT * scale_factor_;
    
    // Create window centered on screen
    window_ = SDL_CreateWindow(
        title.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        window_width,
        window_height,
        SDL_WINDOW_SHOWN
    );
    
    if (!window_) {
        return false;
    }
    
    // Create renderer with hardware acceleration (no VSync - we use software timing)
    // VSync would block the render loop when window is minimized/unfocused
    renderer_ = SDL_CreateRenderer(
        window_,
        -1,
        SDL_RENDERER_ACCELERATED
    );
    
    if (!renderer_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
        return false;
    }
    
    // Create texture for Game Boy framebuffer
    // Use RGBA32 format for maximum compatibility
    texture_ = SDL_CreateTexture(
        renderer_,
        SDL_PIXELFORMAT_RGBA32,
        SDL_TEXTUREACCESS_STREAMING,
        LCD_WIDTH,
        LCD_HEIGHT
    );
    
    if (!texture_) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
        SDL_DestroyWindow(window_);
        window_ = nullptr;
        return false;
    }
    
    // Set up audio specification
    SDL_AudioSpec audio_spec;
    SDL_zero(audio_spec);
    audio_spec.freq = static_cast<int>(kAudioSampleRate);
    audio_spec.format = AUDIO_S16SYS;     // 16-bit signed audio
    audio_spec.channels = 2;              // Stereo
    audio_spec.samples = 512;             // Smaller buffer for lower latency
    audio_spec.callback = nullptr;        // Use queue-based audio
    
    // Open audio device
    audio_device_ = SDL_OpenAudioDevice(nullptr, 0, &audio_spec, nullptr, 0);
    if (audio_device_ == 0) {
        std::cerr << "Failed to open audio device: " << SDL_GetError() << std::endl;
        // Continue without audio - not a fatal error
    } else {
        // Start audio playback (unpause)
        SDL_PauseAudioDevice(audio_device_, 0);
    }
    
    // Initialize ImGui
    IMGUI_CHECKVERSION();
    imgui_context_ = ImGui::CreateContext();
    if (!imgui_context_) {
        std::cerr << "Failed to create ImGui context" << std::endl;
        return false;
    }
    
    // Keep keyboard focus available for gameplay while the debugger is visible.
    // Text input widgets still receive key events when they are active, but we
    // avoid enabling global ImGui keyboard navigation because that causes the
    // debugger to capture normal gameplay controls.
    
    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    
    // Initialize ImGui SDL2 and SDL_Renderer backends
    if (!ImGui_ImplSDL2_InitForSDLRenderer(window_, renderer_)) {
        std::cerr << "Failed to initialize ImGui SDL2 backend" << std::endl;
        ImGui::DestroyContext(imgui_context_);
        imgui_context_ = nullptr;
        return false;
    }
    
    if (!ImGui_ImplSDLRenderer2_Init(renderer_)) {
        std::cerr << "Failed to initialize ImGui SDL_Renderer backend" << std::endl;
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext(imgui_context_);
        imgui_context_ = nullptr;
        return false;
    }
    
    return true;
}

void Display::update(const std::vector<u8>& framebuffer) {
    if (!texture_ || !renderer_) {
        return;
    }
    
    // Expected framebuffer size check
    const size_t expected_size = LCD_WIDTH * LCD_HEIGHT * BYTES_PER_PIXEL;
    if (framebuffer.size() != expected_size) {
        return;
    }
    
    // Update texture with framebuffer data
    SDL_UpdateTexture(
        texture_,
        nullptr,  // Update entire texture
        framebuffer.data(),
        LCD_WIDTH * BYTES_PER_PIXEL  // Pitch in bytes
    );
    
    // Clear renderer
    SDL_SetRenderDrawColor(renderer_, CLEAR_COLOR_R, CLEAR_COLOR_G, CLEAR_COLOR_B, CLEAR_COLOR_A);
    SDL_RenderClear(renderer_);
    
    // In debugger mode, we use ImGui for the entire layout
    // In normal mode, render framebuffer fullscreen then ImGui on top
    if (!debugger_mode_) {
        // Normal mode: render framebuffer fullscreen
        SDL_RenderCopy(renderer_, texture_, nullptr, nullptr);
    }
    
    // Start new ImGui frame
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    if (debugger_mode_) {
        if (debugger_gui_) {
            debugger_gui_->apply_debug_theme();
        }
    } else {
        ImGui::StyleColorsDark();
    }
    
    // Render menu bar
    render_menu_bar();
    
    // In debugger mode, render emulator as a panel
    if (debugger_mode_) {
        // Left panel: Emulator view (fixed size, no move/resize)
        ImGui::SetNextWindowPos(ImVec2(0, 20), ImGuiCond_Always);  // Below menu bar
        ImGui::SetNextWindowSize(ImVec2(480, 432), ImGuiCond_Always);  // 160x144 * 3
        ImGui::Begin("Emulator", nullptr, 
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | 
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);
        
        // Get available size and calculate aspect-correct scaling
        ImVec2 available = ImGui::GetContentRegionAvail();
        float scale_x = available.x / LCD_WIDTH;
        float scale_y = available.y / LCD_HEIGHT;
        float scale = std::min(scale_x, scale_y);
        
        ImVec2 size(LCD_WIDTH * scale, LCD_HEIGHT * scale);
        
        // Center the image
        ImVec2 pos = ImGui::GetCursorPos();
        pos.x += (available.x - size.x) * 0.5f;
        pos.y += (available.y - size.y) * 0.5f;
        ImGui::SetCursorPos(pos);
        
        // Render the framebuffer texture
        ImGui::Image((ImTextureID)(intptr_t)texture_, size);
        
        ImGui::End();
        
        // Render debugger windows
        if (debugger_gui_) {
            debugger_gui_->render();
        }
    } else {
        // Normal mode: render debugger GUI if visible (floating windows mode)
        if (debugger_gui_ && debugger_gui_->is_visible()) {
            debugger_gui_->render();
        }
    }
    
    // Render about dialog if open
    if (show_about_dialog_) {
        render_about_dialog();
    }
    
    // Render controller config dialog if open
    if (show_controller_config_) {
        render_controller_config();
    }

    render_open_rom_dialog();
    
    // Render ImGui
    ImGui::Render();
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer_);
    
    // Present to window
    SDL_RenderPresent(renderer_);
}

bool Display::should_close() const {
    return should_close_;
}

bool Display::is_turbo_active() const {
    return turbo_mode_;
}

void Display::poll_events(Joypad* joypad) {
    SDL_Event event;
    
    while (SDL_PollEvent(&event)) {
        // Let ImGui handle the event first
        ImGui_ImplSDL2_ProcessEvent(&event);
        
        // Check if ImGui wants to capture input
        const ImGuiIO& io = ImGui::GetIO();
        bool imgui_wants_keyboard = io.WantCaptureKeyboard;
        
        // Exception: If waiting for key rebinding, allow keyboard events through
        bool waiting_for_rebind = (waiting_for_key_ >= 0);
        
        switch (event.type) {
            case SDL_QUIT:
                should_close_ = true;
                break;
                
            case SDL_KEYDOWN:
                if (!imgui_wants_keyboard || waiting_for_rebind ||
                    is_global_shortcut(event.key.keysym.sym, event.key.keysym.mod)) {
                    handle_keydown(event.key.keysym.sym, event.key.keysym.mod, joypad);
                }
                break;
                
            case SDL_KEYUP:
                handle_keyup(event.key.keysym.sym, joypad);
                break;
                
            case SDL_CONTROLLERDEVICEADDED:
                gamepad_->on_controller_added(event.cdevice.which);
                break;
                
            case SDL_CONTROLLERDEVICEREMOVED:
                gamepad_->on_controller_removed(event.cdevice.which, joypad);
                break;
                
            case SDL_CONTROLLERBUTTONDOWN:
                gamepad_->on_button_down(event.cbutton.which, event.cbutton.button, joypad);
                break;
                
            case SDL_CONTROLLERBUTTONUP:
                gamepad_->on_button_up(event.cbutton.which, event.cbutton.button, joypad);
                break;
                
            case SDL_CONTROLLERAXISMOTION:
                gamepad_->on_axis_motion(event.caxis.which, event.caxis.axis, event.caxis.value, joypad);
                break;
                
            default:
                break;
        }
    }
}

bool Display::is_global_shortcut(int key, int modifiers) const {
    if ((modifiers & KMOD_CTRL) != 0) {
        return key == SDLK_o || key == SDLK_r;
    }

    if (key == SDLK_ESCAPE) {
        return !open_rom_dialog_visible_;
    }

    return key >= SDLK_F1 && key <= SDLK_F12;
}

void Display::handle_keydown(int key, int modifiers, Joypad* joypad) {
    // If waiting for key rebinding, capture it
    if (waiting_for_key_ >= 0) {
        switch (waiting_for_key_) {
            case KEY_UP: key_bindings_.up = key; break;
            case KEY_DOWN: key_bindings_.down = key; break;
            case KEY_LEFT: key_bindings_.left = key; break;
            case KEY_RIGHT: key_bindings_.right = key; break;
            case KEY_A: key_bindings_.a = key; break;
            case KEY_B: key_bindings_.b = key; break;
            case KEY_START: key_bindings_.start = key; break;
            case KEY_SELECT: key_bindings_.select = key; break;
        }
        waiting_for_key_ = -1;
        save_key_bindings();  // Persist the new binding
        return;
    }
    
    if (key == SDLK_ESCAPE) {
        if (open_rom_dialog_visible_) {
            open_rom_error_message_.clear();
            close_open_rom_dialog_requested_ = true;
            return;
        }

        // ESC key closes window
        should_close_ = true;
        return;
    }

    const bool ctrl_pressed = (modifiers & KMOD_CTRL) != 0;
    const bool shift_pressed = (modifiers & KMOD_SHIFT) != 0;

    if (ctrl_pressed && key == SDLK_o) {
        request_open_rom_dialog();
        return;
    }

    if (ctrl_pressed && key == SDLK_r) {
        should_reset_ = true;
        return;
    }

    if (!current_rom_path_.empty() && key >= SDLK_F1 && key < SDLK_F1 + kHotkeySlotCount) {
        const int slot = key - SDLK_F1;
        if (shift_pressed) {
            load_state_slot_ = slot;
        } else {
            save_state_slot_ = slot;
        }
        return;
    }
    
    // Space key activates turbo mode (fast-forward)
    if (key == SDLK_SPACE) {
        turbo_mode_ = true;
        return;
    }
    
    // P key toggles pause
    if (key == SDLK_p) {
        toggle_pause();
        return;
    }
    
    // M key toggles mute
    if (key == SDLK_m) {
        toggle_mute();
        return;
    }
    
    // F11 key toggles debugger mode
    if (key == SDLK_F11) {
        if (debugger_gui_) {
            toggle_debugger_mode();
        }
        return;
    }
    
    // F5 key continues/pauses in debugger
    if (key == SDLK_F5) {
        if (debugger_gui_ && debugger_mode_) {
            if (debugger_gui_->is_paused()) {
                debugger_gui_->continue_execution();
            } else {
                debugger_gui_->pause_execution();
            }
        }
        return;
    }
    
    // F10 key for step over in debugger
    if (key == SDLK_F10) {
        if (debugger_gui_ && debugger_mode_ && debugger_gui_->is_paused()) {
            debugger_gui_->request_step_over();
        }
        return;
    }
    
    // F12 key captures screenshot
    if (key == SDLK_F12) {
        screenshot_requested_ = true;
        return;
    }
    
    // Route keyboard input to joypad if available
    if (!joypad) {
        return;
    }
    
    // D-Pad using configured keys
    if (key == key_bindings_.up) {
        joypad->press_up();
    } else if (key == key_bindings_.down) {
        joypad->press_down();
    } else if (key == key_bindings_.left) {
        joypad->press_left();
    } else if (key == key_bindings_.right) {
        joypad->press_right();
    }
    // A Button
    else if (key == key_bindings_.a) {
        joypad->press_a();
    }
    // B Button
    else if (key == key_bindings_.b) {
        joypad->press_b();
    }
    // Start
    else if (key == key_bindings_.start) {
        joypad->press_start();
    }
    // Select
    else if (key == key_bindings_.select || key == SDLK_RSHIFT) {
        joypad->press_select();
    }
}

void Display::handle_keyup(int key, Joypad* joypad) {
    // Space key deactivates turbo mode
    if (key == SDLK_SPACE) {
        turbo_mode_ = false;
        return;
    }
    
    // Route keyboard input to joypad if available
    if (!joypad) {
        return;
    }
    
    // D-Pad
    if (key == key_bindings_.up) {
        joypad->release_up();
    } else if (key == key_bindings_.down) {
        joypad->release_down();
    } else if (key == key_bindings_.left) {
        joypad->release_left();
    } else if (key == key_bindings_.right) {
        joypad->release_right();
    }
    // A Button
    else if (key == key_bindings_.a) {
        joypad->release_a();
    }
    // B Button
    else if (key == key_bindings_.b) {
        joypad->release_b();
    }
    // Start
    else if (key == key_bindings_.start) {
        joypad->release_start();
    }
    // Select
    else if (key == key_bindings_.select || key == SDLK_RSHIFT) {
        joypad->release_select();
    }
}

int Display::width() const {
    return LCD_WIDTH * scale_factor_;
}

int Display::height() const {
    return LCD_HEIGHT * scale_factor_;
}

std::string Display::get_sdl_error() {
    const char* error = SDL_GetError();
    return error ? std::string(error) : "Unknown SDL error";
}

void Display::queue_audio(const std::vector<std::pair<u8, u8>>& samples) {
    if (audio_device_ == 0 || samples.empty()) {
        return;
    }

    constexpr size_t kChannelsPerSample = 2;
    constexpr size_t kBytesPerQueuedSample = kChannelsPerSample * sizeof(i16);
    if (samples.size() > (std::numeric_limits<Uint32>::max() / kBytesPerQueuedSample)) {
        return;
    }
    
    // If muted, queue silence instead of actual audio for timing purposes
    if (is_muted_) {
        // Queue silence to maintain timing synchronization
        std::vector<i16> silence(samples.size() * 2, 0);
        SDL_QueueAudio(audio_device_, silence.data(), silence.size() * sizeof(i16));
        return;
    }
    
    // Convert U8 samples (0-255, 128=silence) to S16 (-32768..32767, 0=silence)
    // and interleave to format (L, R, L, R, ...)
    std::vector<i16> interleaved(samples.size() * 2);
    for (size_t i = 0; i < samples.size(); i++) {
        // Convert U8 to S16: (sample - 128) * 128
        // Hardware uses U8, we use S16 for better quality
        // *128 gives moderate amplification (-16384 to +16256 range)
        interleaved[i * 2 + 0] = static_cast<i16>((samples[i].first - AUDIO_U8_MIDPOINT) * AUDIO_U8_SCALE);   // Left
        interleaved[i * 2 + 1] = static_cast<i16>((samples[i].second - AUDIO_U8_MIDPOINT) * AUDIO_U8_SCALE);  // Right
    }
    
    // Queue audio data
    SDL_QueueAudio(audio_device_, interleaved.data(), interleaved.size() * sizeof(i16));
}

unsigned int Display::get_audio_queue_size() const {
    if (audio_device_ == 0) {
        return 0;
    }
    return SDL_GetQueuedAudioSize(audio_device_);
}

void Display::clear_audio_queue() {
    if (audio_device_ != 0) {
        SDL_ClearQueuedAudio(audio_device_);
    }
}

bool Display::has_pending_rom() const {
    return !pending_rom_path_.empty();
}

std::string Display::get_pending_rom() {
    std::string rom = pending_rom_path_;
    pending_rom_path_.clear();
    return rom;
}

bool Display::should_reset() const {
    return should_reset_;
}

void Display::bind_session_context(const std::string& rom_path, RecentRoms& recent_roms) {
    if (current_rom_path_ != rom_path) {
        slot_metadata_.clear();
    }

    current_rom_path_ = rom_path;
    recent_roms_ = &recent_roms;
}

void Display::clear_reset_flag() {
    should_reset_ = false;
}

bool Display::is_paused() const {
    return is_paused_;
}

void Display::toggle_pause() {
    is_paused_ = !is_paused_;
}

void Display::toggle_mute() {
    is_muted_ = !is_muted_;
    // When muting, clear the queue to prevent audio backlog
    if (is_muted_ && audio_device_ != 0) {
        clear_audio_queue();
    }
}

bool Display::is_muted() const {
    return is_muted_;
}

bool Display::screenshot_requested() const {
    return screenshot_requested_;
}

void Display::clear_screenshot_request() {
    screenshot_requested_ = false;
}

void Display::capture_screenshot(const std::vector<u8>& framebuffer, const std::string& rom_path) {
    if (screenshot_ && screenshot_->capture(framebuffer, rom_path)) {
        std::cout << "Screenshot saved to: " << screenshot_->get_last_screenshot_path() << std::endl;
    }
}

float Display::get_speed_multiplier() const {
    return speed_multiplier_;
}

int Display::get_save_state_slot() const {
    return save_state_slot_;
}

int Display::get_load_state_slot() const {
    return load_state_slot_;
}

int Display::get_delete_state_slot() const {
    return delete_state_slot_;
}

void Display::clear_state_request() {
    save_state_slot_ = -1;
    load_state_slot_ = -1;
    delete_state_slot_ = -1;
}

void Display::render_menu_bar() {
    if (ImGui::BeginMainMenuBar()) {
        const bool has_rom_loaded = !current_rom_path_.empty();

        // File Menu
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open ROM...", "Ctrl+O")) {
                request_open_rom_dialog();
            }
            
            ImGui::Separator();
            
            if (ImGui::BeginMenu("Recent ROMs")) {
                if (recent_roms_ && !recent_roms_->is_empty()) {
                    const auto& roms = recent_roms_->get_roms();
                    for (const auto& entry : roms) {
                        if (ImGui::MenuItem(entry.display_name.c_str())) {
                            pending_rom_path_ = entry.file_path;
                        }
                    }
                    
                    ImGui::Separator();
                    if (ImGui::MenuItem("Clear Recent ROMs")) {
                        recent_roms_->clear();
                    }
                } else {
                    ImGui::MenuItem("(No recent ROMs)", nullptr, false, false);
                }
                ImGui::EndMenu();
            }
            
            ImGui::Separator();
            
            if (ImGui::MenuItem("Exit", "ESC")) {
                should_close_ = true;
            }
            
            ImGui::EndMenu();
        }
        
        // Emulation Menu
        if (ImGui::BeginMenu("Emulation")) {
            if (ImGui::MenuItem("Pause", "P", is_paused_)) {
                toggle_pause();
            }
            
            if (ImGui::MenuItem("Reset", "Ctrl+R")) {
                should_reset_ = true;
            }
            
            ImGui::Separator();
            
            if (ImGui::BeginMenu("Save State", has_rom_loaded)) {
                // Only compute labels when menu is open to avoid 10 stat() calls per frame
                for (int i = 0; i < kSlotCount; i++) {
                    std::string label = get_slot_label(i, current_rom_path_);
                    if (ImGui::MenuItem(label.c_str())) {
                        save_state_slot_ = i;
                    }
                }
                ImGui::EndMenu();
            }
            
            if (ImGui::BeginMenu("Load State", has_rom_loaded)) {
                // Only compute labels when menu is open to avoid 10 stat() calls per frame
                for (int i = 0; i < kSlotCount; i++) {
                    std::string label = get_slot_label(i, current_rom_path_);
                    // Disable menu item if slot is free
                    bool is_free = (label.find("free") != std::string::npos);
                    if (ImGui::MenuItem(label.c_str(), nullptr, false, !is_free)) {
                        load_state_slot_ = i;
                    }
                }
                ImGui::EndMenu();
            }
            
            if (ImGui::BeginMenu("Delete State", has_rom_loaded)) {
                // Only compute labels when menu is open to avoid 10 stat() calls per frame
                for (int i = 0; i < kSlotCount; i++) {
                    std::string label = get_slot_label(i, current_rom_path_);
                    // Disable menu item if slot is free
                    bool is_free = (label.find("free") != std::string::npos);
                    if (ImGui::MenuItem(label.c_str(), nullptr, false, !is_free)) {
                        delete_state_slot_ = i;
                    }
                }
                ImGui::EndMenu();
            }
            
            ImGui::Separator();
            
            if (ImGui::MenuItem("Turbo Mode", "Space", turbo_mode_, false)) {
                // Display only - controlled by Space key
            }
            
            if (ImGui::MenuItem("Mute Audio", "M", is_muted_)) {
                toggle_mute();
            }
            
            ImGui::Separator();
            
            bool is_debugger_visible = debugger_gui_ && debugger_gui_->is_visible();
            if (ImGui::MenuItem("Debugger", "F11", is_debugger_visible)) {
                if (debugger_gui_) {
                    toggle_debugger_mode();
                }
            }
            
            ImGui::EndMenu();
        }
        
        // Debug Menu (only in debugger mode)
        if (debugger_mode_ && debugger_gui_) {
            if (ImGui::BeginMenu("Debug")) {
                // Execution controls
                bool paused = debugger_gui_->is_paused();
                if (ImGui::MenuItem(paused ? "Continue" : "Pause", "F5")) {
                    if (paused) {
                        debugger_gui_->continue_execution();
                    } else {
                        debugger_gui_->pause_execution();
                    }
                }
                
                if (ImGui::MenuItem("Step Over", "F10", false, paused)) {
                    debugger_gui_->request_step_over();
                }
                
                ImGui::Separator();
                
                // View toggles
                debugger_gui_->render_window_menu_items();
                
                ImGui::Separator();
                
                if (ImGui::MenuItem("Exit Debugger", "F11")) {
                    close_debugger_mode();
                }
                
                ImGui::EndMenu();
            }
        }
        
        // Input Menu
        if (ImGui::BeginMenu("Input")) {
            ImGui::Text("Keyboard Controls:");
            ImGui::Separator();
            
            // Helper to get key name
            auto get_key_name = [](int keycode) -> std::string {
                if (keycode == SDLK_UNKNOWN) {
                    return "None";
                }
                const char* name = SDL_GetKeyName(keycode);
                return name ? std::string(name) : "Unknown";
            };
            
            // Display current bindings dynamically
            ImGui::Text("D-Pad Up: %s", get_key_name(key_bindings_.up).c_str());
            ImGui::Text("D-Pad Down: %s", get_key_name(key_bindings_.down).c_str());
            ImGui::Text("D-Pad Left: %s", get_key_name(key_bindings_.left).c_str());
            ImGui::Text("D-Pad Right: %s", get_key_name(key_bindings_.right).c_str());
            ImGui::Text("Button A: %s", get_key_name(key_bindings_.a).c_str());
            ImGui::Text("Button B: %s", get_key_name(key_bindings_.b).c_str());
            ImGui::Text("Start: %s", get_key_name(key_bindings_.start).c_str());
            ImGui::Text("Select: %s", get_key_name(key_bindings_.select).c_str());
            ImGui::Separator();
            ImGui::Text("Space - Turbo Mode");
            ImGui::Text("P - Pause");
            ImGui::Text("M - Mute");
            ImGui::Separator();
            
            if (ImGui::MenuItem("Configure Controller...")) {
                show_controller_config_ = true;
            }
            
            ImGui::EndMenu();
        }
        
        // Help Menu
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About")) {
                show_about_dialog_ = true;
            }
            
            ImGui::EndMenu();
        }
        
        ImGui::EndMainMenuBar();
    }
}

void Display::render_about_dialog() {
    ImGui::SetNextWindowSize(ImVec2(400, 250), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("About gbglow", &show_about_dialog_)) {
        ImGui::Text("gbglow");
        ImGui::Text("Game Boy Color Emulator");
        ImGui::Separator();
        
        ImGui::Text("Version: 0.1.0");
        ImGui::Text("Build: %s %s", __DATE__, __TIME__);
        ImGui::Separator();
        
        ImGui::Text("A Game Boy Color emulator");
        ImGui::Text("written in C++17 with SDL2 and ImGui.");
        ImGui::Separator();
        
        ImGui::Text("Features:");
        ImGui::BulletText("CPU, PPU, audio, and cartridge subsystems");
        ImGui::BulletText("PPU with DMG/CGB color support");
        ImGui::BulletText("APU with 4 sound channels");
        ImGui::BulletText("Cartridge support (MBC1, MBC3, MBC5)");
        ImGui::BulletText("Debugger overlay and save states");
        ImGui::BulletText("Turbo mode and audio output");
        ImGui::Separator();
        
        if (ImGui::Button("Close")) {
            show_about_dialog_ = false;
        }
    }
    ImGui::End();
}

void Display::request_open_rom_dialog() {
    open_rom_dialog_requested_ = true;
    open_rom_dialog_visible_ = true;
    close_open_rom_dialog_requested_ = false;
    open_rom_error_message_.clear();

    const std::string& initial_path = current_rom_path_.empty() ? pending_rom_path_ : current_rom_path_;
    open_rom_path_buffer_.fill('\0');
    if (!initial_path.empty()) {
        std::snprintf(open_rom_path_buffer_.data(), open_rom_path_buffer_.size(), "%s", initial_path.c_str());
    }
}

std::string Display::browse_for_rom_file(std::string& error_message) {
    error_message.clear();

    if (command_exists("zenity")) {
        const std::string zenity_command =
            std::string("zenity --file-selection --title='") + kOpenRomDialogTitle + "' "
            "--file-filter='Game Boy ROMs | *.gb *.gbc *.bin *.rom' "
            "--file-filter='All Files | *' 2>/dev/null";
        const CommandResult result = run_command_and_capture_stdout(zenity_command.c_str());
        if (!result.launched) {
            error_message = "Failed to launch zenity.";
            return {};
        }
        if (result.exit_code == 0) {
            return result.output;
        }
        if (result.exit_code == 1) {
            error_message = "No ROM selected.";
            return {};
        }
        error_message = "zenity failed while opening the file picker.";
        return {};
    }

    if (command_exists("kdialog")) {
        const char* kdialog_command =
            "kdialog --getopenfilename \"$HOME\" '*.gb *.gbc *.bin *.rom|Game Boy ROMs' 2>/dev/null";
        const CommandResult result = run_command_and_capture_stdout(kdialog_command);
        if (!result.launched) {
            error_message = "Failed to launch kdialog.";
            return {};
        }
        if (result.exit_code == 0) {
            return result.output;
        }
        if (result.exit_code == 1) {
            error_message = "No ROM selected.";
            return {};
        }
        error_message = "kdialog failed while opening the file picker.";
        return {};
    }

    error_message = "No native file picker found. Install zenity or kdialog, or enter the ROM path manually.";
    return {};
}

void Display::render_open_rom_dialog() {
    if (open_rom_dialog_requested_) {
        ImGui::OpenPopup(kOpenRomDialogTitle);
        open_rom_dialog_requested_ = false;
    }

    if (!open_rom_dialog_visible_) {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(560.0f, 0.0f), ImGuiCond_Appearing);
    if (ImGui::BeginPopupModal(kOpenRomDialogTitle, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (close_open_rom_dialog_requested_) {
            close_open_rom_dialog_requested_ = false;
            open_rom_dialog_visible_ = false;
            open_rom_error_message_.clear();
            ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
            return;
        }

        ImGui::TextWrapped("Enter a ROM path manually or use Browse when a native file picker is available.");
        ImGui::Spacing();

        ImGui::SetNextItemWidth(420.0f);
        if (ImGui::InputText("ROM Path", open_rom_path_buffer_.data(), open_rom_path_buffer_.size(),
                             ImGuiInputTextFlags_EnterReturnsTrue)) {
            open_rom_error_message_.clear();
        }

        ImGui::SameLine();
        if (ImGui::Button("Browse")) {
            std::string browse_error;
            const std::string selected_path = browse_for_rom_file(browse_error);
            if (!selected_path.empty()) {
                std::snprintf(open_rom_path_buffer_.data(), open_rom_path_buffer_.size(), "%s", selected_path.c_str());
                open_rom_error_message_.clear();
            } else if (!browse_error.empty() && browse_error != "No ROM selected.") {
                open_rom_error_message_ = browse_error;
            }
        }

        if (!open_rom_error_message_.empty()) {
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.45f, 1.0f), "%s", open_rom_error_message_.c_str());
        }

        ImGui::Spacing();
        if (ImGui::Button("Open", ImVec2(120.0f, 0.0f))) {
            const std::filesystem::path rom_path = open_rom_path_buffer_.data();
            std::error_code error;
            if (rom_path.empty()) {
                open_rom_error_message_ = "Enter a ROM path first.";
            } else if (!std::filesystem::exists(rom_path, error) || error) {
                open_rom_error_message_ = "Selected ROM path does not exist.";
            } else if (!std::filesystem::is_regular_file(rom_path, error) || error) {
                open_rom_error_message_ = "Selected path is not a file.";
            } else {
                pending_rom_path_ = rom_path.string();
                open_rom_dialog_visible_ = false;
                close_open_rom_dialog_requested_ = false;
                open_rom_error_message_.clear();
                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120.0f, 0.0f))) {
            open_rom_dialog_visible_ = false;
            close_open_rom_dialog_requested_ = false;
            open_rom_error_message_.clear();
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void Display::render_controller_config() {
    ImGui::SetNextWindowSize(ImVec2(500, 450), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Controller Configuration", &show_controller_config_)) {
        ImGui::TextWrapped("Click 'Change' next to any control and press the desired key.");
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        ImGui::Text("Game Controls:");
        ImGui::Spacing();
        
        // Helper lambda to get key name
        auto get_key_name = [](int keycode) -> std::string {
            const char* name = SDL_GetKeyName(keycode);
            return name ? std::string(name) : "Unknown";
        };
        
        // Helper lambda for rebind button
        auto rebind_button = [this](const char* label, int key_index, const std::string& current_key) {
            ImGui::Text("%s", label);
            ImGui::SameLine(150);
            ImGui::Text("%s", current_key.c_str());
            ImGui::SameLine(250);
            if (waiting_for_key_ == key_index) {
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Press a key...");
                if (ImGui::Button("Cancel")) {
                    waiting_for_key_ = -1;
                }
            } else {
                char button_id[32];
                snprintf(button_id, sizeof(button_id), "Change##%d", key_index);
                if (ImGui::Button(button_id)) {
                    waiting_for_key_ = key_index;
                }
            }
        };
        
        // D-Pad
        rebind_button("D-Pad Up:", KEY_UP, get_key_name(key_bindings_.up));
        rebind_button("D-Pad Down:", KEY_DOWN, get_key_name(key_bindings_.down));
        rebind_button("D-Pad Left:", KEY_LEFT, get_key_name(key_bindings_.left));
        rebind_button("D-Pad Right:", KEY_RIGHT, get_key_name(key_bindings_.right));
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Buttons
        rebind_button("A Button:", KEY_A, get_key_name(key_bindings_.a));
        rebind_button("B Button:", KEY_B, get_key_name(key_bindings_.b));
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Special
        rebind_button("Start:", KEY_START, get_key_name(key_bindings_.start));
        rebind_button("Select:", KEY_SELECT, get_key_name(key_bindings_.select));
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Fixed Emulator Controls:");
        ImGui::Text("Pause: P");
        ImGui::Text("Turbo Mode: Space (hold)");
        ImGui::Text("Exit: ESC");
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        if (ImGui::Button("Reset to Defaults", ImVec2(150, 0))) {
            key_bindings_.up = SDLK_UP;
            key_bindings_.down = SDLK_DOWN;
            key_bindings_.left = SDLK_LEFT;
            key_bindings_.right = SDLK_RIGHT;
            key_bindings_.a = SDLK_z;
            key_bindings_.b = SDLK_x;
            key_bindings_.start = SDLK_RETURN;
            key_bindings_.select = SDLK_LSHIFT;
            waiting_for_key_ = -1;
            save_key_bindings();  // Persist the defaults
        }
        
        ImGui::SameLine();
        
        if (ImGui::Button("Close", ImVec2(120, 0))) {
            show_controller_config_ = false;
            waiting_for_key_ = -1;
        }
    }
    ImGui::End();
}

void Display::update_slot_metadata(int slot) {
    SlotMetadata metadata;
    metadata.used = true;
    metadata.timestamp = std::time(nullptr);
    slot_metadata_[slot] = metadata;
}

void Display::delete_slot_metadata(int slot) {
    slot_metadata_.erase(slot);
}

bool Display::check_slot_exists(const std::string& state_path) {
    std::error_code error;
    return std::filesystem::exists(state_path, error) && !error;
}

std::string Display::get_slot_label(int slot, const std::string& rom_path) const {
    char label[SLOT_LABEL_SIZE];
    
    // Build state path from rom_path first
    std::string state_path;
    size_t last_dot = rom_path.find_last_of('.');
    if (last_dot != std::string::npos) {
        state_path = rom_path.substr(0, last_dot);
    } else {
        state_path = rom_path;
    }
    state_path += ".slot" + std::to_string(slot + 1) + ".state";
    
    // Check if file actually exists
    if (!check_slot_exists(state_path)) {
        // File doesn't exist - slot is free
        snprintf(label, sizeof(label), "Slot %d - free", slot + 1);
        return label;
    }
    
    // File exists - check if we have cached metadata
    auto it = slot_metadata_.find(slot);
    if (it != slot_metadata_.end() && it->second.used) {
        // Format timestamp from cache
        std::tm tm_buf{};
        portable_localtime(&it->second.timestamp, &tm_buf);
        char time_str[64];
        std::strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &tm_buf);
        snprintf(label, sizeof(label), "Slot %d - %s", slot + 1, time_str);
        return label;
    }
    
    // Get file modification time via std::filesystem
    std::error_code ec;
    auto ftime = std::filesystem::last_write_time(state_path, ec);
    if (!ec) {
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
        std::time_t mtime = std::chrono::system_clock::to_time_t(sctp);
        std::tm tm_buf{};
        portable_localtime(&mtime, &tm_buf);
        char time_str[64];
        std::strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &tm_buf);
        snprintf(label, sizeof(label), "Slot %d - %s", slot + 1, time_str);
        return label;
    }
    
    // Keep occupied slots actionable even if metadata lookup fails.
    snprintf(label, sizeof(label), "Slot %d - used", slot + 1);
    return label;
}

std::string Display::get_keybindings_path() {
    const char* xdg_config = std::getenv("XDG_CONFIG_HOME");
    std::string base_dir;
    if (xdg_config) {
        base_dir = xdg_config;
    } else {
        const char* home = std::getenv("HOME");
        base_dir = home ? std::string(home) + "/.config" : ".";
    }
    return base_dir + "/" + kConfigDirectoryName + "/" + kKeybindingsFileName;
}

std::string Display::get_gamepad_config_path() {
    const char* xdg_config = std::getenv("XDG_CONFIG_HOME");
    std::string base_dir;
    if (xdg_config) {
        base_dir = xdg_config;
    } else {
        const char* home = std::getenv("HOME");
        base_dir = home ? std::string(home) + "/.config" : ".";
    }
    return base_dir + "/" + kConfigDirectoryName + "/" + kGamepadConfigFileName;
}

void Display::load_key_bindings() {
    std::string config_path = get_keybindings_path();
    std::ifstream file(config_path);
    if (!file.is_open()) {
        std::cout << "No key bindings config found, using defaults" << std::endl;
        return;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        // Parse key=value format
        size_t eq_pos = line.find('=');
        if (eq_pos == std::string::npos) {
            continue;
        }
        
        std::string key = line.substr(0, eq_pos);
        std::string value = line.substr(eq_pos + 1);
        
        // Trim whitespace
        trim_in_place(key);
        trim_in_place(value);
        if (key.empty() || value.empty()) {
            continue;
        }
        
        int keycode = parse_sdl_keycode(value);
        if (keycode == SDLK_UNKNOWN) {
            continue;
        }
        
        // Assign to appropriate binding
        if (key == "up") key_bindings_.up = keycode;
        else if (key == "down") key_bindings_.down = keycode;
        else if (key == "left") key_bindings_.left = keycode;
        else if (key == "right") key_bindings_.right = keycode;
        else if (key == "a") key_bindings_.a = keycode;
        else if (key == "b") key_bindings_.b = keycode;
        else if (key == "start") key_bindings_.start = keycode;
        else if (key == "select") key_bindings_.select = keycode;
    }
    
    file.close();
    std::cout << "Loaded key bindings from " << config_path << std::endl;
}

void Display::save_key_bindings() {
    std::string config_path = get_keybindings_path();
    std::error_code error;
    std::filesystem::create_directories(std::filesystem::path(config_path).parent_path(), error);
    if (error) {
        std::cerr << "Failed to create key bindings config directory: " << error.message() << std::endl;
        return;
    }

    std::ostringstream contents;
    contents << "# gbglow Key Bindings Configuration\n";
    contents << "# Format: action=SDLK_keyname\n";
    contents << "# \n";
    contents << "# Available keys: https://wiki.libsdl.org/SDL2/SDLKeycodeLookup\n";
    contents << "# Examples: SDLK_a, SDLK_UP, SDLK_SPACE, SDLK_RETURN, SDLK_LSHIFT\n";
    contents << "#\n";
    contents << "# Game Boy Controls:\n";
    contents << "up=" << sdl_keycode_to_string(key_bindings_.up) << "\n";
    contents << "down=" << sdl_keycode_to_string(key_bindings_.down) << "\n";
    contents << "left=" << sdl_keycode_to_string(key_bindings_.left) << "\n";
    contents << "right=" << sdl_keycode_to_string(key_bindings_.right) << "\n";
    contents << "a=" << sdl_keycode_to_string(key_bindings_.a) << "\n";
    contents << "b=" << sdl_keycode_to_string(key_bindings_.b) << "\n";
    contents << "start=" << sdl_keycode_to_string(key_bindings_.start) << "\n";
    contents << "select=" << sdl_keycode_to_string(key_bindings_.select) << "\n";

    if (!write_text_file_atomically(config_path, contents.str())) {
        std::cerr << "Failed to save key bindings config" << std::endl;
        return;
    }

    std::cout << "Saved key bindings to " << config_path << std::endl;
}

int Display::parse_sdl_keycode(const std::string& keyname) {
    // Map common SDL keycode names to their values
    if (keyname == "SDLK_UNKNOWN") return SDLK_UNKNOWN;
    
    // Try SDL's built-in function first
    SDL_Keycode code = SDL_GetKeyFromName(keyname.c_str());
    if (code != SDLK_UNKNOWN) {
        return code;
    }
    
    // If it has SDLK_ prefix, try without it
    if (keyname.rfind("SDLK_", 0) == 0) {
        std::string name_without_prefix = keyname.substr(5);
        code = SDL_GetKeyFromName(name_without_prefix.c_str());
        if (code != SDLK_UNKNOWN) {
            return code;
        }
    }
    
    return SDLK_UNKNOWN;
}

std::string Display::sdl_keycode_to_string(int keycode) {
    if (keycode == SDLK_UNKNOWN) {
        return "SDLK_UNKNOWN";
    }
    
    const char* name = SDL_GetKeyName(keycode);
    if (name && strlen(name) > 0) {
        return std::string("SDLK_") + name;
    }
    
    return "SDLK_UNKNOWN";
}

} // namespace gbglow
