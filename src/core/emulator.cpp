#include "emulator.h"
#include "timer.h"
#include "../audio/apu.h"
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <cstdio>

namespace gbcrush {

Emulator::Emulator() {
    memory_ = std::make_unique<Memory>();
    cpu_ = std::make_unique<CPU>(*memory_);
    ppu_ = std::make_unique<PPU>(*memory_);
    memory_->set_ppu(ppu_.get());
    // Note: Joypad is owned by Memory, not Emulator
    
    // Try to load boot ROM (optional - emulator works without it)
    // Common locations: dmg_boot.bin, boot.gb, etc.
    if (memory_->load_boot_rom("dmg_boot.bin")) {
        std::cout << "Boot ROM loaded" << std::endl;
    } else if (memory_->load_boot_rom("boot.gb")) {
        std::cout << "Boot ROM loaded" << std::endl;
    } else {
        std::cout << "No boot ROM found (emulator will skip boot sequence)" << std::endl;
    }
}

bool Emulator::load_rom(const std::string& path) {
    try {
        auto cartridge = Cartridge::load_rom_from_file(path);
        memory_->load_cartridge(std::move(cartridge));
        rom_path_ = path;
        
        // Load save file if it exists
        std::string save_path = get_save_path();
        if (auto* cart = memory_->cartridge()) {
            if (cart->load_ram_from_file(save_path)) {
                std::cout << "Loaded save file: " << save_path << std::endl;
            }
        }
        
        reset();
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

void Emulator::reset() {
    cpu_->reset();
}

void Emulator::run_frame() {
    // One frame = ~70224 cycles (59.7 fps)
    // 154 scanlines × 456 dots = 70224 cycles
    run_cycles(CYCLES_PER_FRAME);
}

void Emulator::run_cycles(Cycles cycles) {
    Cycles remaining = cycles;
    
    while (remaining > 0) {
        // Execute one CPU instruction
        Cycles cpu_cycles = cpu_->step();
        
        // Handle interrupts
        cpu_->handle_interrupts();
        
        // Run PPU for the same number of cycles
        ppu_->step(cpu_cycles);
        
        // Run APU for audio generation
        memory_->apu().step(cpu_cycles);
        
        remaining = (remaining > cpu_cycles) ? (remaining - cpu_cycles) : 0;
    }
}

void Emulator::run(const std::string& window_title, int scale_factor) {
    // Initialize display
    Display display;
    if (!display.initialize(window_title, scale_factor)) {
        std::cerr << "Failed to initialize display" << std::endl;
        return;
    }
    
    // Set ROM path in display for slot tracking
    display.set_rom_path(rom_path_);
    
    std::cout << "Emulator running. Press ESC to exit." << std::endl;
    std::cout << "Controls: Arrow keys = D-pad, Z = A, X = B, Enter = Start, Shift = Select" << std::endl;
    
    // Main game loop - use audio-based synchronization for consistent timing
    // This approach is immune to system load variations (unlike sleep-based timing)
    // The audio subsystem acts as a natural clock: if audio queue is full, we wait
    
    // Target audio queue size: ~2 frames worth of audio (~60ms)
    // 44100 samples/sec * 2 channels * 2 bytes/sample * 2 frames / 60 fps ≈ 5880 bytes
    constexpr unsigned int TARGET_AUDIO_QUEUE = 44100 * 2 * 2 * 2 / 60;
    constexpr unsigned int MAX_AUDIO_QUEUE = TARGET_AUDIO_QUEUE * 2;  // Max ~4 frames
    
    while (!display.should_close()) {
        // Handle input events - get joypad from memory (single source of truth)
        display.poll_events(&memory_->joypad());
        
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
        
        // Skip frame processing if paused
        if (display.is_paused()) {
            // Still update display to show menu bar
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
                run_frame();
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
                run_frame();
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
            unsigned int adjusted_max = static_cast<unsigned int>(MAX_AUDIO_QUEUE / speed_multiplier);
            unsigned int queue_size = display.get_audio_queue_size();
            
            while (queue_size > adjusted_max) {
                // Small sleep to reduce CPU spinning while waiting
                std::this_thread::sleep_for(std::chrono::microseconds(500));
                queue_size = display.get_audio_queue_size();
            }
        }
    }
    
    // Save RAM to file on exit
    if (auto* cart = memory_->cartridge()) {
        std::string save_path = get_save_path();
        if (cart->save_ram_to_file(save_path)) {
            std::cout << "Saved game data to: " << save_path << std::endl;
        }
    }
    
    std::cout << "Emulator stopped." << std::endl;
}

const PPU& Emulator::ppu() const
{
    return *ppu_;
}

PPU& Emulator::ppu()
{
    return *ppu_;
}

const CPU& Emulator::cpu() const
{
    return *cpu_;
}

CPU& Emulator::cpu()
{
    return *cpu_;
}

Joypad& Emulator::joypad()
{
    return memory_->joypad();
}

Cartridge* Emulator::cartridge()
{
    return memory_->cartridge();
}

Memory& Emulator::memory()
{
    return *memory_;
}

std::string Emulator::get_save_path() const
{
    // Replace .gb, .gbc extension with .sav
    std::string save_path = rom_path_;
    
    // Find last dot
    size_t dot_pos = save_path.rfind('.');
    if (dot_pos != std::string::npos) {
        save_path = save_path.substr(0, dot_pos);
    }
    
    save_path += ".sav";
    return save_path;
}

bool Emulator::save_state(int slot) {
    if (slot < 0 || slot > 9) {
        return false;
    }
    
    std::string state_path = get_state_path(slot);
    
    std::ofstream file(state_path, std::ios::binary);
    if (!file) {
        return false;
    }
    
    // Write header with version
    const char header[] = "GBCRUSH_STATE_V2";
    file.write(header, sizeof(header));
    
    // Save CPU state (registers)
    const auto& regs = cpu_->registers();
    file.write(reinterpret_cast<const char*>(&regs.a), sizeof(regs.a));
    file.write(reinterpret_cast<const char*>(&regs.f), sizeof(regs.f));
    file.write(reinterpret_cast<const char*>(&regs.b), sizeof(regs.b));
    file.write(reinterpret_cast<const char*>(&regs.c), sizeof(regs.c));
    file.write(reinterpret_cast<const char*>(&regs.d), sizeof(regs.d));
    file.write(reinterpret_cast<const char*>(&regs.e), sizeof(regs.e));
    file.write(reinterpret_cast<const char*>(&regs.h), sizeof(regs.h));
    file.write(reinterpret_cast<const char*>(&regs.l), sizeof(regs.l));
    file.write(reinterpret_cast<const char*>(&regs.sp), sizeof(regs.sp));
    file.write(reinterpret_cast<const char*>(&regs.pc), sizeof(regs.pc));
    
    // Save CPU flags
    bool ime = cpu_->ime();
    bool halted = cpu_->is_halted();
    file.write(reinterpret_cast<const char*>(&ime), sizeof(ime));
    file.write(reinterpret_cast<const char*>(&halted), sizeof(halted));
    
    // Save memory state using Memory's serialize method
    std::vector<u8> mem_data;
    memory_->serialize(mem_data);
    size_t mem_size = mem_data.size();
    file.write(reinterpret_cast<const char*>(&mem_size), sizeof(mem_size));
    file.write(reinterpret_cast<const char*>(mem_data.data()), mem_data.size());
    
    // Save PPU state
    u8 ly = ppu_->scanline();
    u8 mode = static_cast<u8>(ppu_->mode());
    file.write(reinterpret_cast<const char*>(&ly), sizeof(ly));
    file.write(reinterpret_cast<const char*>(&mode), sizeof(mode));
    
    // Save Timer state
    Timer& timer = memory_->timer();
    u8 div = timer.read_div();
    u8 tima = timer.read_tima();
    u8 tma = timer.read_tma();
    u8 tac = timer.read_tac();
    file.write(reinterpret_cast<const char*>(&div), sizeof(div));
    file.write(reinterpret_cast<const char*>(&tima), sizeof(tima));
    file.write(reinterpret_cast<const char*>(&tma), sizeof(tma));
    file.write(reinterpret_cast<const char*>(&tac), sizeof(tac));
    
    file.close();
    return true;
}

bool Emulator::load_state(int slot) {
    if (slot < 0 || slot > 9) {
        return false;
    }
    
    std::string state_path = get_state_path(slot);
    
    std::ifstream file(state_path, std::ios::binary);
    if (!file) {
        return false;
    }
    
    // Verify header - support both V1 and V2
    char header[16];
    file.read(header, sizeof("GBCRUSH_STATE_V2"));
    std::string header_str(header);
    
    if (header_str == "GBCRUSH_STATE_V2") {
        // Load V2 format with full state
        
        // Load CPU state
        auto& regs = cpu_->registers();
        file.read(reinterpret_cast<char*>(&regs.a), sizeof(regs.a));
        file.read(reinterpret_cast<char*>(&regs.f), sizeof(regs.f));
        file.read(reinterpret_cast<char*>(&regs.b), sizeof(regs.b));
        file.read(reinterpret_cast<char*>(&regs.c), sizeof(regs.c));
        file.read(reinterpret_cast<char*>(&regs.d), sizeof(regs.d));
        file.read(reinterpret_cast<char*>(&regs.e), sizeof(regs.e));
        file.read(reinterpret_cast<char*>(&regs.h), sizeof(regs.h));
        file.read(reinterpret_cast<char*>(&regs.l), sizeof(regs.l));
        file.read(reinterpret_cast<char*>(&regs.sp), sizeof(regs.sp));
        file.read(reinterpret_cast<char*>(&regs.pc), sizeof(regs.pc));
        
        // Load CPU flags
        bool ime, halted;
        file.read(reinterpret_cast<char*>(&ime), sizeof(ime));
        file.read(reinterpret_cast<char*>(&halted), sizeof(halted));
        cpu_->set_ime(ime);
        cpu_->set_halted(halted);
        
        // Load memory state
        size_t mem_size;
        file.read(reinterpret_cast<char*>(&mem_size), sizeof(mem_size));
        
        // Validate memory size (expected size is ~25KB for Memory serialization)
        constexpr size_t EXPECTED_MEM_SIZE = 24996;  // VRAM + banks + WRAM + OAM + HRAM + IO + flags
        constexpr size_t MAX_VALID_SIZE = EXPECTED_MEM_SIZE * 2;  // Allow some margin
        if (mem_size == 0 || mem_size > MAX_VALID_SIZE) {
            std::cerr << "Corrupt save state: invalid memory size " << mem_size << std::endl;
            file.close();
            return false;
        }
        
        std::vector<u8> mem_data(mem_size);
        file.read(reinterpret_cast<char*>(mem_data.data()), mem_size);
        
        // Verify we read enough data
        if (!file.good()) {
            std::cerr << "Corrupt save state: truncated memory data" << std::endl;
            file.close();
            return false;
        }
        
        size_t offset = 0;
        memory_->deserialize(mem_data.data(), offset);
        
        // Load PPU state
        u8 ly, mode;
        file.read(reinterpret_cast<char*>(&ly), sizeof(ly));
        file.read(reinterpret_cast<char*>(&mode), sizeof(mode));
        // Note: PPU state restoration would require setters in PPU class
        // For now, we rely on memory state which includes LCD registers
        
        // Load Timer state
        u8 div, tima, tma, tac;
        file.read(reinterpret_cast<char*>(&div), sizeof(div));
        file.read(reinterpret_cast<char*>(&tima), sizeof(tima));
        file.read(reinterpret_cast<char*>(&tma), sizeof(tma));
        file.read(reinterpret_cast<char*>(&tac), sizeof(tac));
        
        Timer& timer = memory_->timer();
        timer.write_div(div);
        timer.write_tima(tima);
        timer.write_tma(tma);
        timer.write_tac(tac);
        
    } else if (header_str == "GBCRUSH_STATE_V1") {
        // Load V1 format (legacy - incomplete state)
        auto& regs = cpu_->registers();
        file.read(reinterpret_cast<char*>(&regs.a), sizeof(regs.a));
        file.read(reinterpret_cast<char*>(&regs.f), sizeof(regs.f));
        file.read(reinterpret_cast<char*>(&regs.b), sizeof(regs.b));
        file.read(reinterpret_cast<char*>(&regs.c), sizeof(regs.c));
        file.read(reinterpret_cast<char*>(&regs.d), sizeof(regs.d));
        file.read(reinterpret_cast<char*>(&regs.e), sizeof(regs.e));
        file.read(reinterpret_cast<char*>(&regs.h), sizeof(regs.h));
        file.read(reinterpret_cast<char*>(&regs.l), sizeof(regs.l));
        file.read(reinterpret_cast<char*>(&regs.sp), sizeof(regs.sp));
        file.read(reinterpret_cast<char*>(&regs.pc), sizeof(regs.pc));
        
        // Load partial memory state (work RAM only)
        auto& mem = memory();
        for (u16 addr = 0xC000; addr < 0xE000; addr++) {
            u8 byte;
            file.read(reinterpret_cast<char*>(&byte), 1);
            mem.write(addr, byte);
        }
    } else {
        file.close();
        return false;  // Unrecognized format
    }
    
    file.close();
    return true;
}

std::string Emulator::get_state_path(int slot) const {
    std::string state_path = rom_path_;
    
    // Remove extension
    size_t dot_pos = state_path.rfind('.');
    if (dot_pos != std::string::npos) {
        state_path = state_path.substr(0, dot_pos);
    }
    
    // Add slot number and .state extension
    state_path += ".slot" + std::to_string(slot + 1) + ".state";
    return state_path;
}

const std::string& Emulator::get_rom_path() const {
    return rom_path_;
}

bool Emulator::delete_state(int slot) {
    if (slot < 0 || slot > 9) {
        return false;
    }
    
    std::string state_path = get_state_path(slot);
    
    // Remove the file
    if (std::remove(state_path.c_str()) == 0) {
        return true;
    }
    
    return false;  // File doesn't exist or couldn't be deleted
}

} // namespace gbcrush
