#pragma once

#include "../core/types.h"
#include <vector>
#include <string>

// Forward declare SDL types to avoid including SDL in header
struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;

namespace emugbc {

class Joypad;

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
    
    /**
     * Initialize SDL and create window
     * Returns true on success, false on failure
     */
    bool initialize(const std::string& title, int scale_factor);
    
    /**
     * Update display with framebuffer data
     * Framebuffer is 160×144 RGBA pixels
     */
    void update(const std::vector<u8>& framebuffer);
    
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
     * Check if window should close
     */
    bool should_close() const;
    
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
    
private:
    SDL_Window* window_;
    SDL_Renderer* renderer_;
    SDL_Texture* texture_;
    unsigned int audio_device_;  // SDL_AudioDeviceID is typedef'd to Uint32
    
    bool should_close_;
    int scale_factor_;
    
    // Game Boy LCD dimensions
    static constexpr int LCD_WIDTH = 160;
    static constexpr int LCD_HEIGHT = 144;
    static constexpr int BYTES_PER_PIXEL = 4;  // RGBA
    
    // Default scale factor
    static constexpr int DEFAULT_SCALE = 4;
    
    /**
     * Convert SDL error to readable message
     */
    std::string get_sdl_error() const;
    
    /**
     * Handle keyboard press events
     */
    void handle_keydown(int key, Joypad* joypad);
    
    /**
     * Handle keyboard release events
     */
    void handle_keyup(int key, Joypad* joypad);
};

} // namespace emugbc
