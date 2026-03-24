#include "display.h"
#include "../input/joypad.h"
#include <SDL2/SDL.h>
#include <stdexcept>
#include <iostream>

namespace emugbc {

Display::Display()
    : window_(nullptr)
    , renderer_(nullptr)
    , texture_(nullptr)
    , audio_device_(0)
    , should_close_(false)
    , turbo_mode_(false)
    , scale_factor_(DEFAULT_SCALE)
{
}

Display::~Display() {
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
        switch (event.type) {
            case SDL_QUIT:
                should_close_ = true;
                break;
                
            case SDL_KEYDOWN:
                handle_keydown(event.key.keysym.sym, joypad);
                break;
                
            case SDL_KEYUP:
                handle_keyup(event.key.keysym.sym, joypad);
                break;
                
            default:
                break;
        }
    }
}

void Display::handle_keydown(int key, Joypad* joypad) {
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
    
    // Route keyboard input to joypad if available
    if (!joypad) {
        return;
    }
    
    switch (key) {
        // D-Pad - Arrow keys
        case SDLK_UP:
            joypad->press_up();
            break;
        case SDLK_DOWN:
            joypad->press_down();
            break;
        case SDLK_LEFT:
            joypad->press_left();
            break;
        case SDLK_RIGHT:
            joypad->press_right();
            break;
            
        // Action buttons - handle both upper and lowercase
        case SDLK_z:
        case SDLK_a:  // Alternative: A key for A button
            joypad->press_a();
            break;
        case SDLK_x:
        case SDLK_s:  // Alternative: S key for B button
            joypad->press_b();
            break;
            
        // Start/Select
        case SDLK_RETURN:
            joypad->press_start();
            break;
        case SDLK_RSHIFT:
        case SDLK_LSHIFT:
            joypad->press_select();
            break;
            
        default:
            break;
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
    
    switch (key) {
        // D-Pad - Arrow keys
        case SDLK_UP:
            joypad->release_up();
            break;
        case SDLK_DOWN:
            joypad->release_down();
            break;
        case SDLK_LEFT:
            joypad->release_left();
            break;
        case SDLK_RIGHT:
            joypad->release_right();
            break;
            
        // Action buttons - handle both upper and lowercase
        case SDLK_z:
        case SDLK_a:  // Alternative: A key for A button
            joypad->release_a();
            break;
        case SDLK_x:
        case SDLK_s:  // Alternative: S key for B button
            joypad->release_b();
            break;
            
        // Start/Select
        case SDLK_RETURN:
            joypad->release_start();
            break;
        case SDLK_RSHIFT:
        case SDLK_LSHIFT:
            joypad->release_select();
            break;
            
        default:
            break;
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

} // namespace emugbc
