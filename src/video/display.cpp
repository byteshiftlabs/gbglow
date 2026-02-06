#include "display.h"
#include <SDL2/SDL.h>
#include <stdexcept>

namespace emugbc {

Display::Display()
    : window_(nullptr)
    , renderer_(nullptr)
    , texture_(nullptr)
    , should_close_(false)
    , scale_factor_(DEFAULT_SCALE)
{
}

Display::~Display() {
    // Clean up SDL resources in reverse order of creation
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
    
    // Initialize SDL video subsystem
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        return false;
    }
    
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
    
    // Create renderer with hardware acceleration
    renderer_ = SDL_CreateRenderer(
        window_,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    
    if (!renderer_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
        SDL_Quit();
        return false;
    }
    
    // Create texture for Game Boy framebuffer
    texture_ = SDL_CreateTexture(
        renderer_,
        SDL_PIXELFORMAT_RGBA8888,
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

void Display::poll_events() {
    SDL_Event event;
    
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                should_close_ = true;
                break;
                
            case SDL_KEYDOWN:
                // ESC key closes window
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    should_close_ = true;
                }
                break;
                
            default:
                break;
        }
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

} // namespace emugbc
