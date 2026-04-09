#include "display.h"
#include "../input/joypad.h"
#include <SDL2/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <sys/stat.h>

namespace emugbc {

Display::Display()
    : window_(nullptr)
    , renderer_(nullptr)
    , texture_(nullptr)
    , audio_device_(0)
    , imgui_context_(nullptr)
    , should_close_(false)
    , turbo_mode_(false)
    , scale_factor_(DEFAULT_SCALE)
    , is_muted_(false)
    , should_reset_(false)
    , is_paused_(false)
    , show_about_dialog_(false)
    , show_controller_config_(false)
    , speed_multiplier_(1.0f)
    , save_state_slot_(-1)
    , load_state_slot_(-1)
    , delete_state_slot_(-1)
    , waiting_for_key_(-1)
{
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
}

Display::~Display() {
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

bool Display::initialize(const std::string& title, int scale_factor) {
    scale_factor_ = scale_factor;
    
    // Initialize SDL video and audio subsystems
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        return false;
    }
    
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
        SDL_Quit();
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
        SDL_Quit();
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
        SDL_Quit();
        return false;
    }
    
    // Set up audio specification
    SDL_AudioSpec audio_spec;
    SDL_zero(audio_spec);
    audio_spec.freq = 44100;              // 44.1 kHz sample rate
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
    
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable keyboard navigation
    
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
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);
    
    // Copy texture to renderer (scales automatically)
    SDL_RenderCopy(renderer_, texture_, nullptr, nullptr);
    
    // Start new ImGui frame
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    
    // Render menu bar
    render_menu_bar();
    
    // Render about dialog if open
    if (show_about_dialog_) {
        render_about_dialog();
    }
    
    // Render controller config dialog if open
    if (show_controller_config_) {
        render_controller_config();
    }
    
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
        ImGuiIO& io = ImGui::GetIO();
        bool imgui_wants_keyboard = io.WantCaptureKeyboard;
        
        // Exception: If waiting for key rebinding, allow keyboard events through
        bool waiting_for_rebind = (waiting_for_key_ >= 0);
        
        switch (event.type) {
            case SDL_QUIT:
                should_close_ = true;
                break;
                
            case SDL_KEYDOWN:
                if (!imgui_wants_keyboard || waiting_for_rebind) {
                    handle_keydown(event.key.keysym.sym, joypad);
                }
                break;
                
            case SDL_KEYUP:
                if (!imgui_wants_keyboard) {
                    handle_keyup(event.key.keysym.sym, joypad);
                }
                break;
                
            default:
                break;
        }
    }
}

void Display::handle_keydown(int key, Joypad* joypad) {
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
    
    // ESC key closes window
    if (key == SDLK_ESCAPE) {
        should_close_ = true;
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

std::string Display::get_sdl_error() const {
    const char* error = SDL_GetError();
    return error ? std::string(error) : "Unknown SDL error";
}

void Display::queue_audio(const std::vector<std::pair<u8, u8>>& samples) {
    if (audio_device_ == 0 || samples.empty()) {
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
        interleaved[i * 2 + 0] = static_cast<i16>((samples[i].first - 128) * 128);   // Left
        interleaved[i * 2 + 1] = static_cast<i16>((samples[i].second - 128) * 128);  // Right
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
        // File Menu
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open ROM...", "Ctrl+O")) {
                // For now, just show a message - file dialog would need nativefiledialog or similar
                std::cout << "Open ROM dialog would appear here" << std::endl;
                // In a real implementation, you'd use a file dialog library
            }
            
            ImGui::Separator();
            
            if (ImGui::BeginMenu("Recent ROMs")) {
                ImGui::MenuItem("(No recent ROMs)", nullptr, false, false);
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
            
            if (ImGui::BeginMenu("Save State")) {
                // Only compute labels when menu is open to avoid 10 stat() calls per frame
                for (int i = 0; i < 10; i++) {
                    std::string label = get_slot_label(i, current_rom_path_);
                    if (ImGui::MenuItem(label.c_str())) {
                        save_state_slot_ = i;
                    }
                }
                ImGui::EndMenu();
            }
            
            if (ImGui::BeginMenu("Load State")) {
                // Only compute labels when menu is open to avoid 10 stat() calls per frame
                for (int i = 0; i < 10; i++) {
                    std::string label = get_slot_label(i, current_rom_path_);
                    // Disable menu item if slot is free
                    bool is_free = (label.find("free") != std::string::npos);
                    if (ImGui::MenuItem(label.c_str(), nullptr, false, !is_free)) {
                        load_state_slot_ = i;
                    }
                }
                ImGui::EndMenu();
            }
            
            if (ImGui::BeginMenu("Delete State")) {
                // Only compute labels when menu is open to avoid 10 stat() calls per frame
                for (int i = 0; i < 10; i++) {
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
            
            ImGui::EndMenu();
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
    
    if (ImGui::Begin("About GBCrush", &show_about_dialog_)) {
        ImGui::Text("GBCrush");
        ImGui::Text("Game Boy Color Emulator");
        ImGui::Separator();
        
        ImGui::Text("Version: 0.1.0");
        ImGui::Text("Build: %s %s", __DATE__, __TIME__);
        ImGui::Separator();
        
        ImGui::Text("A cycle-accurate Game Boy Color emulator");
        ImGui::Text("written in C++17 with SDL2 and ImGui.");
        ImGui::Separator();
        
        ImGui::Text("Features:");
        ImGui::BulletText("Full CPU emulation");
        ImGui::BulletText("PPU with DMG/CGB color support");
        ImGui::BulletText("APU with 4 sound channels");
        ImGui::BulletText("Cartridge support (MBC1, MBC3, MBC5)");
        ImGui::BulletText("Real-time audio playback");
        ImGui::BulletText("Turbo mode for fast-forward");
        ImGui::Separator();
        
        if (ImGui::Button("Close")) {
            show_about_dialog_ = false;
        }
    }
    ImGui::End();
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

bool Display::check_slot_exists(const std::string& state_path) const {
    struct stat buffer;
    return (stat(state_path.c_str(), &buffer) == 0);
}

std::string Display::get_slot_label(int slot, const std::string& rom_path) const {
    char label[128];
    
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
        std::tm* tm_info = std::localtime(&it->second.timestamp);
        char time_str[64];
        std::strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
        snprintf(label, sizeof(label), "Slot %d - %s", slot + 1, time_str);
        return label;
    }
    
    // Get file modification time
    struct stat file_stat;
    if (stat(state_path.c_str(), &file_stat) == 0) {
        std::tm* tm_info = std::localtime(&file_stat.st_mtime);
        char time_str[64];
        std::strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
        snprintf(label, sizeof(label), "Slot %d - %s", slot + 1, time_str);
        return label;
    }
    
    // Fallback to free
    snprintf(label, sizeof(label), "Slot %d - free", slot + 1);
    return label;
}

void Display::set_rom_path(const std::string& rom_path) {
    current_rom_path_ = rom_path;
}

void Display::load_key_bindings() {
    std::ifstream file("config/keybindings.conf");
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
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);
        
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
    std::cout << "Loaded key bindings from config/keybindings.conf" << std::endl;
}

void Display::save_key_bindings() {
    std::ofstream file("config/keybindings.conf");
    if (!file.is_open()) {
        std::cerr << "Failed to save key bindings config" << std::endl;
        return;
    }
    
    file << "# GBCrush Key Bindings Configuration\n";
    file << "# Format: action=SDLK_keyname\n";
    file << "# \n";
    file << "# Available keys: https://wiki.libsdl.org/SDL2/SDLKeycodeLookup\n";
    file << "# Examples: SDLK_a, SDLK_UP, SDLK_SPACE, SDLK_RETURN, SDLK_LSHIFT\n";
    file << "#\n";
    file << "# Game Boy Controls:\n";
    file << "up=" << sdl_keycode_to_string(key_bindings_.up) << "\n";
    file << "down=" << sdl_keycode_to_string(key_bindings_.down) << "\n";
    file << "left=" << sdl_keycode_to_string(key_bindings_.left) << "\n";
    file << "right=" << sdl_keycode_to_string(key_bindings_.right) << "\n";
    file << "a=" << sdl_keycode_to_string(key_bindings_.a) << "\n";
    file << "b=" << sdl_keycode_to_string(key_bindings_.b) << "\n";
    file << "start=" << sdl_keycode_to_string(key_bindings_.start) << "\n";
    file << "select=" << sdl_keycode_to_string(key_bindings_.select) << "\n";
    
    file.close();
    std::cout << "Saved key bindings to config/keybindings.conf" << std::endl;
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
    if (keyname.find("SDLK_") == 0) {
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

} // namespace emugbc
