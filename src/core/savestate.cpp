#include "savestate.h"
#include "cpu.h"
#include "memory.h"
#include "../video/ppu.h"
#include <filesystem>

namespace gbcrush {

std::string SaveState::get_path(int slot, const std::string& rom_path) {
    // Replace extension with .stateN
    std::string save_path = rom_path;
    
    size_t dot_pos = save_path.rfind('.');
    if (dot_pos != std::string::npos) {
        save_path = save_path.substr(0, dot_pos);
    }
    
    save_path += ".state" + std::to_string(slot);
    return save_path;
}

bool SaveState::exists(int slot, const std::string& rom_path) {
    return std::filesystem::exists(get_path(slot, rom_path));
}

bool SaveState::save(int slot, const std::string& rom_path,
                     const CPU& cpu, const Memory& memory, const PPU& ppu) {
    std::string path = get_path(slot, rom_path);
    std::ofstream file(path, std::ios::binary);
    
    if (!file.is_open()) {
        return false;
    }
    
    // Write header
    write_u32(file, MAGIC);
    write_u32(file, VERSION);
    
    // Placeholder offsets (will be filled later if needed)
    write_u32(file, 0);  // cpu_offset
    write_u32(file, 0);  // memory_offset
    write_u32(file, 0);  // ppu_offset
    
    // Save CPU state
    const auto& regs = cpu.registers();
    write_u8(file, regs.a());
    write_u8(file, regs.f());
    write_u8(file, regs.b());
    write_u8(file, regs.c());
    write_u8(file, regs.d());
    write_u8(file, regs.e());
    write_u8(file, regs.h());
    write_u8(file, regs.l());
    write_u16(file, regs.sp());
    write_u16(file, regs.pc());
    
    // Save CPU flags
    write_u8(file, cpu.ime() ? 1 : 0);
    write_u8(file, cpu.halted() ? 1 : 0);
    
    // Save Memory state
    // Work RAM (8KB)
    for (u16 addr = 0xC000; addr < 0xE000; addr++) {
        write_u8(file, memory.read(addr));
    }
    
    // High RAM (127 bytes)
    for (u16 addr = 0xFF80; addr < 0xFFFF; addr++) {
        write_u8(file, memory.read(addr));
    }
    
    // I/O registers (0xFF00-0xFF7F)
    for (u16 addr = 0xFF00; addr < 0xFF80; addr++) {
        write_u8(file, memory.read(addr));
    }
    
    // Interrupt Enable register
    write_u8(file, memory.read(0xFFFF));
    
    // VRAM (8KB)
    for (u16 addr = 0x8000; addr < 0xA000; addr++) {
        write_u8(file, memory.read(addr));
    }
    
    // OAM (160 bytes)
    for (u16 addr = 0xFE00; addr < 0xFEA0; addr++) {
        write_u8(file, memory.read(addr));
    }
    
    // Save cartridge RAM if present
    if (memory.cartridge() && memory.cartridge()->has_battery()) {
        auto ram = memory.cartridge()->get_ram_data();
        write_u32(file, static_cast<u32>(ram.size()));
        if (!ram.empty()) {
            write_bytes(file, ram.data(), ram.size());
        }
    } else {
        write_u32(file, 0);
    }
    
    // Save PPU state
    write_u8(file, ppu.get_ly());
    write_u8(file, ppu.get_mode());
    
    return file.good();
}

bool SaveState::load(int slot, const std::string& rom_path,
                     CPU& cpu, Memory& memory, PPU& ppu) {
    std::string path = get_path(slot, rom_path);
    std::ifstream file(path, std::ios::binary);
    
    if (!file.is_open()) {
        return false;
    }
    
    // Read and verify header
    u32 magic = read_u32(file);
    if (magic != MAGIC) {
        return false;
    }
    
    u32 version = read_u32(file);
    if (version != VERSION) {
        return false;
    }
    
    // Skip offsets (not used in this simple implementation)
    read_u32(file);  // cpu_offset
    read_u32(file);  // memory_offset
    read_u32(file);  // ppu_offset
    
    // Load CPU state
    auto& regs = cpu.registers();
    regs.set_a(read_u8(file));
    regs.set_f(read_u8(file));
    regs.set_b(read_u8(file));
    regs.set_c(read_u8(file));
    regs.set_d(read_u8(file));
    regs.set_e(read_u8(file));
    regs.set_h(read_u8(file));
    regs.set_l(read_u8(file));
    regs.set_sp(read_u16(file));
    regs.set_pc(read_u16(file));
    
    // Load CPU flags
    cpu.set_ime(read_u8(file) != 0);
    cpu.set_halted(read_u8(file) != 0);
    
    // Load Memory state
    // Work RAM (8KB)
    for (u16 addr = 0xC000; addr < 0xE000; addr++) {
        memory.write(addr, read_u8(file));
    }
    
    // High RAM (127 bytes)
    for (u16 addr = 0xFF80; addr < 0xFFFF; addr++) {
        memory.write(addr, read_u8(file));
    }
    
    // I/O registers (0xFF00-0xFF7F)
    for (u16 addr = 0xFF00; addr < 0xFF80; addr++) {
        memory.write(addr, read_u8(file));
    }
    
    // Interrupt Enable register
    memory.write(0xFFFF, read_u8(file));
    
    // VRAM (8KB)
    for (u16 addr = 0x8000; addr < 0xA000; addr++) {
        memory.write(addr, read_u8(file));
    }
    
    // OAM (160 bytes)
    for (u16 addr = 0xFE00; addr < 0xFEA0; addr++) {
        memory.write(addr, read_u8(file));
    }
    
    // Load cartridge RAM if present
    u32 ram_size = read_u32(file);
    if (ram_size > 0 && memory.cartridge()) {
        std::vector<u8> ram(ram_size);
        read_bytes(file, ram.data(), ram_size);
        memory.cartridge()->set_ram_data(ram);
    }
    
    // Load PPU state
    ppu.set_ly(read_u8(file));
    ppu.set_mode(read_u8(file));
    
    return file.good();
}

// Serialization helpers
void SaveState::write_u8(std::ofstream& file, u8 value) {
    file.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

void SaveState::write_u16(std::ofstream& file, u16 value) {
    file.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

void SaveState::write_u32(std::ofstream& file, u32 value) {
    file.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

void SaveState::write_bytes(std::ofstream& file, const u8* data, size_t size) {
    file.write(reinterpret_cast<const char*>(data), size);
}

u8 SaveState::read_u8(std::ifstream& file) {
    u8 value;
    file.read(reinterpret_cast<char*>(&value), sizeof(value));
    return value;
}

u16 SaveState::read_u16(std::ifstream& file) {
    u16 value;
    file.read(reinterpret_cast<char*>(&value), sizeof(value));
    return value;
}

u32 SaveState::read_u32(std::ifstream& file) {
    u32 value;
    file.read(reinterpret_cast<char*>(&value), sizeof(value));
    return value;
}

void SaveState::read_bytes(std::ifstream& file, u8* data, size_t size) {
    file.read(reinterpret_cast<char*>(data), size);
}

} // namespace gbcrush
