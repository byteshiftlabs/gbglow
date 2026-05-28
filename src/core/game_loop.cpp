// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 gbglow Contributors
// This file is part of gbglow. See LICENSE for details.

#include "emulator.h"
#include "constants.h"
#include "logging.h"
#include "../audio/apu.h"
#include "timer.h"
#include <fstream>
#include <chrono>
#include <thread>
#include <stdexcept>

namespace gbglow {

using namespace constants::emulator;

void Emulator::attach_display_state(Display& display) {
    display.attach_debugger(*debugger_);
    display.bind_session_context(rom_path_, *recent_roms_);
}

Cycles Emulator::execute_component_step() {
    const u16 executed_pc = cpu_->registers().pc;
    Cycles cpu_cycles = cpu_->step();
    ppu_->step(cpu_cycles);
    memory_->timer().step(cpu_cycles);
    memory_->apu().step(cpu_cycles);
    debugger_->record_execution(executed_pc);
    return cpu_cycles;
}

bool Emulator::run_emulation_cycles(Display& display, Cycles cycle_budget) {
    Cycles remaining = cycle_budget;

    while (remaining > 0) {
        if (display.debugger_visible()) {
            const u16 current_pc = cpu_->registers().pc;
            if (debugger_->should_break(current_pc) || debugger_->should_stop_step_over(current_pc)) {
                display.pause_debugger();
                debugger_->clear_step_over();
                return false;
            }
        }

        const Cycles cpu_cycles = execute_component_step();

        if (debugger_->update_watches() && display.debugger_visible()) {
            display.pause_debugger();
            debugger_->clear_step_over();
            return false;
        }

        remaining = (remaining > cpu_cycles) ? (remaining - cpu_cycles) : 0;
    }

    return true;
}

bool Emulator::handle_pending_rom_request(Display& display) {
    if (!display.has_pending_rom()) {
        return false;
    }

    const std::string pending_rom = display.get_pending_rom();
    if (load_rom(pending_rom)) {
        display.bind_session_context(rom_path_, *recent_roms_);
        log::info("Loaded ROM: " + pending_rom);
    } else {
        log::error("Failed to load ROM from menu: " + pending_rom);
    }

    return true;
}

void Emulator::handle_reset_request(Display& display) {
    if (!display.should_reset()) {
        return;
    }

    reset();
    display.clear_reset_flag();
    log::info("Emulator reset");
}

void Emulator::handle_save_state_request(Display& display) {
    int save_slot = display.get_save_state_slot();
    if (save_slot < 0) {
        return;
    }

    if (save_state(save_slot)) {
        display.update_slot_metadata(save_slot);
        log::info("State saved to slot " + std::to_string(save_slot + 1));
    } else {
        log::error("Failed to save state to slot " + std::to_string(save_slot + 1));
    }

    display.clear_state_request();
}

void Emulator::handle_load_state_request(Display& display) {
    int load_slot = display.get_load_state_slot();
    if (load_slot < 0) {
        return;
    }

    std::string state_path = get_state_path(load_slot);
    bool file_exists = std::ifstream(state_path).good();

    if (load_state(load_slot)) {
        log::info("State loaded from slot " + std::to_string(load_slot + 1));
    } else if (file_exists) {
        log::error("Failed to load state from slot " + std::to_string(load_slot + 1) + " - invalid, incompatible, or corrupted format");
    } else {
        log::warning("No save file found in slot " + std::to_string(load_slot + 1));
    }

    display.clear_state_request();
}

void Emulator::handle_delete_state_request(Display& display) {
    int delete_slot = display.get_delete_state_slot();
    if (delete_slot < 0) {
        return;
    }

    if (delete_state(delete_slot)) {
        display.delete_slot_metadata(delete_slot);
        log::info("State deleted from slot " + std::to_string(delete_slot + 1));
    } else {
        log::error("Failed to delete state from slot " + std::to_string(delete_slot + 1));
    }

    display.clear_state_request();
}

bool Emulator::handle_debugger_step_request(Display& display) {
    if (!display.debugger_should_pause() || !display.debugger_step_requested()) {
        return false;
    }

    execute_component_step();
    debugger_->update_watches();
    display.clear_debugger_step_request();
    update_display_frame(display, false);
    return true;
}

void Emulator::handle_screenshot_request(Display& display) {
    if (!display.screenshot_requested()) {
        return;
    }

    display.capture_screenshot(ppu_->get_rgba_framebuffer(), rom_path_);
    display.clear_screenshot_request();
}

void Emulator::update_display_frame(Display& display, bool clear_frame_ready) {
    display.update(ppu_->get_rgba_framebuffer());
    if (clear_frame_ready) {
        ppu_->clear_frame_ready();
    }
}

void Emulator::update_paused_display(Display& display) {
    if (ppu_->frame_ready()) {
        update_display_frame(display, true);
    } else {
        update_display_frame(display, false);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(16));
}

void Emulator::run_turbo_mode(Display& display) {
    display.clear_audio_queue();
    memory_->apu().clear_audio_buffer();

    for (int i = 0; i < 10; i++) {
        if (!run_emulation_cycles(display, CYCLES_PER_FRAME)) {
            break;
        }
    }

    if (ppu_->frame_ready()) {
        update_display_frame(display, true);
    }
}

void Emulator::run_normal_mode(Display& display, float speed_multiplier) {
    int frames_to_run = static_cast<int>(speed_multiplier);
    if (frames_to_run < 1) {
        frames_to_run = 1;
    }

    for (int i = 0; i < frames_to_run; i++) {
        if (!run_emulation_cycles(display, CYCLES_PER_FRAME)) {
            break;
        }
    }

    if (ppu_->frame_ready()) {
        update_display_frame(display, true);
    }

    const auto& audio_samples = memory_->apu().get_audio_buffer();
    if (!audio_samples.empty()) {
        display.queue_audio(audio_samples);
        memory_->apu().clear_audio_buffer();
    }

    unsigned int adjusted_max = static_cast<unsigned int>(kMaxAudioQueueBytes / speed_multiplier);
    unsigned int queue_size = display.get_audio_queue_size();

    while (queue_size > adjusted_max) {
        std::this_thread::sleep_for(std::chrono::microseconds(500));
        queue_size = display.get_audio_queue_size();
    }
}

void Emulator::save_ram_on_exit() {
    if (auto* loaded_cartridge = memory_->cartridge()) {
        std::string save_path = get_save_path();
        if (loaded_cartridge->save_ram_to_file(save_path)) {
            log::info("Saved game data to: " + save_path);
        }
    }
}

void Emulator::run(const std::string& window_title, int scale_factor) {
    // Initialize display
    Display display;
    if (!display.initialize(window_title, scale_factor)) {
        throw std::runtime_error("Failed to initialize display");
    }

    attach_display_state(display);
    
    log::info("Emulator running. Press ESC to exit, F11 to open debugger, F12 to capture a screenshot.");
    log::info("Controls: Arrow keys = D-pad, Z = A, X = B, Enter = Start, Shift = Select");
    
    // Main game loop - use audio-based synchronization for consistent timing
    // This approach is immune to system load variations (unlike sleep-based timing)
    // The audio subsystem acts as a natural clock: if audio queue is full, we wait
    
    // Target audio queue size: ~2 frames worth of audio (~60ms)
    // 44100 samples/sec * 2 channels * 2 bytes/sample * 2 frames / 60 fps ≈ 5880 bytes
    while (!display.should_close()) {
        display.poll_events(&memory_->joypad());

        if (handle_pending_rom_request(display)) {
            continue;
        }

        handle_reset_request(display);
        handle_save_state_request(display);
        handle_load_state_request(display);
        handle_delete_state_request(display);

        bool debugger_paused = display.debugger_should_pause();

        if (handle_debugger_step_request(display)) {
            continue;
        }

        if (display.debugger_continue_requested()) {
            debugger_->skip_breakpoint_once(cpu_->registers().pc);
            display.clear_debugger_continue_request();
        }

        handle_screenshot_request(display);

        if (display.is_paused() || debugger_paused) {
            update_paused_display(display);
            continue;
        }

        float speed_multiplier = display.get_speed_multiplier();
        bool turbo_active = display.is_turbo_active();

        if (turbo_active) {
            run_turbo_mode(display);
        } else {
            run_normal_mode(display, speed_multiplier);
        }
    }

    save_ram_on_exit();
    
    log::info("Emulator stopped.");
}

}  // namespace gbglow
