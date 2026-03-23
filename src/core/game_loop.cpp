// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 gbglow Contributors
// This file is part of gbglow. See LICENSE for details.

#include "emulator.h"
#include "constants.h"
#include "../audio/apu.h"
#include "../debug/debugger_gui.h"
#include "timer.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <stdexcept>

namespace gbglow {

using namespace constants::emulator;

void Emulator::run(const std::string& window_title, int scale_factor) {
    // Initialize display
    Display display;
    if (!display.initialize(window_title, scale_factor)) {
        throw std::runtime_error("Failed to initialize display");
    }
    
    // Attach debugger to display
    display.attach_debugger(debugger_.get());
    
    // Set ROM path in display for slot tracking
    display.set_rom_path(rom_path_);
    
    // Pass recent ROMs manager to display
    display.set_recent_roms(recent_roms_.get());
    
    std::cout << "Emulator running. Press ESC to exit, F11 to open debugger, F12 to capture a screenshot." << std::endl;
    std::cout << "Controls: Arrow keys = D-pad, Z = A, X = B, Enter = Start, Shift = Select" << std::endl;
    
    // Main game loop - use audio-based synchronization for consistent timing
    // This approach is immune to system load variations (unlike sleep-based timing)
    // The audio subsystem acts as a natural clock: if audio queue is full, we wait
    
    // Target audio queue size: ~2 frames worth of audio (~60ms)
    // 44100 samples/sec * 2 channels * 2 bytes/sample * 2 frames / 60 fps ≈ 5880 bytes
    DebuggerGUI* debugger_gui = display.get_debugger_gui();

    auto run_emulation_cycles = [&](Cycles cycle_budget) {
        Cycles remaining = cycle_budget;

        while (remaining > 0) {
            if (debugger_gui && debugger_gui->is_visible()) {
                const u16 current_pc = cpu_->registers().pc;
                if (debugger_->should_break(current_pc) || debugger_->should_stop_step_over(current_pc)) {
                    debugger_gui->set_paused(true);
                    debugger_->clear_step_over();
                    return false;
                }
            }

            const u16 executed_pc = cpu_->registers().pc;
            Cycles cpu_cycles = cpu_->step();
            ppu_->step(cpu_cycles);
            memory_->timer().step(cpu_cycles);
            memory_->apu().step(cpu_cycles);
            debugger_->record_execution(executed_pc);

            if (debugger_->update_watches() && debugger_gui && debugger_gui->is_visible()) {
                debugger_gui->set_paused(true);
                debugger_->clear_step_over();
                return false;
            }

            remaining = (remaining > cpu_cycles) ? (remaining - cpu_cycles) : 0;
        }

        return true;
    };
    
    while (!display.should_close()) {
        // Handle input events - get joypad from memory (single source of truth)
        display.poll_events(&memory_->joypad());

        // Handle ROM load requests from UI actions such as Recent ROMs.
        if (display.has_pending_rom()) {
            const std::string pending_rom = display.get_pending_rom();
            if (load_rom(pending_rom)) {
                display.set_rom_path(rom_path_);
                std::cout << "Loaded ROM: " << pending_rom << std::endl;
            } else {
                std::cerr << "Failed to load ROM from menu: " << pending_rom << std::endl;
            }
            continue;
        }
        
        // Handle reset request from menu
        if (display.should_reset()) {
            reset();
            display.clear_reset_flag();
            std::cout << "Emulator reset" << std::endl;
        }
        
        // Handle save state request
        int save_slot = display.get_save_state_slot();
        if (save_slot >= 0) {
            if (save_state(save_slot)) {
                display.update_slot_metadata(save_slot);
                std::cout << "State saved to slot " << (save_slot + 1) << std::endl;
            } else {
                std::cerr << "Failed to save state to slot " << (save_slot + 1) << std::endl;
            }
            display.clear_state_request();
        }
        
        // Handle load state request
        int load_slot = display.get_load_state_slot();
        if (load_slot >= 0) {
            std::string state_path = get_state_path(load_slot);
            bool file_exists = (std::ifstream(state_path).good());
            
            if (load_state(load_slot)) {
                std::cout << "State loaded from slot " << (load_slot + 1) << std::endl;
            } else {
                if (file_exists) {
                    std::cerr << "Failed to load state from slot " << (load_slot + 1) << " - corrupted or invalid format" << std::endl;
                    // Delete corrupted save file
                    if (delete_state(load_slot)) {
                        display.delete_slot_metadata(load_slot);
                        std::cout << "Corrupted save file deleted from slot " << (load_slot + 1) << std::endl;
                    } else {
                        std::cerr << "Failed to delete corrupted save from slot " << (load_slot + 1) << std::endl;
                    }
                } else {
                    std::cerr << "No save file found in slot " << (load_slot + 1) << std::endl;
                }
            }
            display.clear_state_request();
        }
        
        // Handle delete state request
        int delete_slot = display.get_delete_state_slot();
        if (delete_slot >= 0) {
            if (delete_state(delete_slot)) {
                display.delete_slot_metadata(delete_slot);
                std::cout << "State deleted from slot " << (delete_slot + 1) << std::endl;
            } else {
                std::cerr << "Failed to delete state from slot " << (delete_slot + 1) << std::endl;
            }
            display.clear_state_request();
        }
        
        // Check debugger pause state
        bool debugger_paused = debugger_gui && debugger_gui->should_pause();
        
        // Handle debugger step request
        if (debugger_paused && debugger_gui->step_requested()) {
            const u16 executed_pc = cpu_->registers().pc;
            Cycles cpu_cycles = cpu_->step();
            ppu_->step(cpu_cycles);
            memory_->timer().step(cpu_cycles);
            memory_->apu().step(cpu_cycles);
            debugger_->record_execution(executed_pc);
            debugger_->update_watches();
            
            debugger_gui->clear_step_request();
            
            // Update display after step
            const auto& framebuffer = ppu_->get_rgba_framebuffer();
            display.update(framebuffer);
            continue;
        }
        
        // Handle debugger continue
        if (debugger_gui && debugger_gui->should_continue()) {
            debugger_->skip_breakpoint_once(cpu_->registers().pc);
            debugger_gui->clear_continue();
        }
        
        // Handle screenshot request
        if (display.screenshot_requested()) {
            const auto& framebuffer = ppu_->get_rgba_framebuffer();
            display.capture_screenshot(framebuffer, rom_path_);
            display.clear_screenshot_request();
        }
        
        // Skip frame processing if paused (either by menu or debugger)
        if (display.is_paused() || debugger_paused) {
            // Still update display to show menu bar and debugger windows
            if (ppu_->frame_ready()) {
                const auto& framebuffer = ppu_->get_rgba_framebuffer();
                display.update(framebuffer);
                ppu_->clear_frame_ready();
            } else {
                // Force display update even without new frame
                const auto& framebuffer = ppu_->get_rgba_framebuffer();
                display.update(framebuffer);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
            continue;
        }
        
        // Get speed multiplier from menu
        float speed_multiplier = display.get_speed_multiplier();
        
        // Check if turbo mode is active (Space key held)
        bool turbo_active = display.is_turbo_active();
        
        if (turbo_active) {
            // TURBO MODE: Run as fast as possible
            // Clear audio queue to prevent backpressure from slowing us down
            display.clear_audio_queue();
            memory_->apu().clear_audio_buffer();
            
            // Run multiple frames without any synchronization
            for (int i = 0; i < 10; i++) {
                if (!run_emulation_cycles(CYCLES_PER_FRAME)) {
                    break;
                }
            }
            
            // Update display to show progress
            if (ppu_->frame_ready()) {
                const auto& framebuffer = ppu_->get_rgba_framebuffer();
                display.update(framebuffer);
                ppu_->clear_frame_ready();
            }
            // No audio in turbo - it would just cause stuttering
        } else {
            // NORMAL MODE: Run frame(s) with audio synchronization based on speed multiplier
            // Speed multiplier affects how many frames we run AND audio queue timing
            
            // For speeds > 1.0, run multiple frames
            // For speeds < 1.0, we still run 1 frame but adjust audio queue target
            int frames_to_run = static_cast<int>(speed_multiplier);
            if (frames_to_run < 1) frames_to_run = 1;
            
            for (int i = 0; i < frames_to_run; i++) {
                if (!run_emulation_cycles(CYCLES_PER_FRAME)) {
                    break;
                }
            }
            
            // Update display if frame is ready
            if (ppu_->frame_ready()) {
                const auto& framebuffer = ppu_->get_rgba_framebuffer();
                display.update(framebuffer);
                ppu_->clear_frame_ready();
            }
            
            // Queue audio samples
            const auto& audio_samples = memory_->apu().get_audio_buffer();
            if (!audio_samples.empty()) {
                display.queue_audio(audio_samples);
                memory_->apu().clear_audio_buffer();
            }
            
            // Audio-based synchronization with speed adjustment
            // At 50%, we want to wait longer (more audio buffered)
            // At 200%, we want less buffering (faster processing)
            unsigned int adjusted_max = static_cast<unsigned int>(kMaxAudioQueueBytes / speed_multiplier);
            unsigned int queue_size = display.get_audio_queue_size();
            
            while (queue_size > adjusted_max) {
                // Small sleep to reduce CPU spinning while waiting
                std::this_thread::sleep_for(std::chrono::microseconds(500));
                queue_size = display.get_audio_queue_size();
            }
        }
    }
    
    // Save RAM to file on exit
    if (auto* loaded_cartridge = memory_->cartridge()) {
        std::string save_path = get_save_path();
        if (loaded_cartridge->save_ram_to_file(save_path)) {
            std::cout << "Saved game data to: " << save_path << std::endl;
        }
    }
    
    std::cout << "Emulator stopped." << std::endl;
}

}  // namespace gbglow
