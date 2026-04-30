#pragma once

#include "types.h"
#include <string>
#include <vector>
#include <fstream>

namespace gbglow {

// Forward declarations
class CPU;
class Memory;
class PPU;

/**
 * Save State System
 * 
 * Handles serialization and deserialization of emulator state
 * for quick save/load functionality.
 * 
 * Save state format:
 * - Header (magic, version)
 * - CPU state (registers, flags, cycles)
 * - Memory state (RAM, VRAM, OAM, I/O registers)
 * - PPU state (scanline, mode, etc.)
 */
class SaveState {
public:
    // Save state file extension
    static constexpr const char* EXTENSION = ".state";
    
    // Maximum number of save slots
    static constexpr int MAX_SLOTS = 10;
    
    /**
     * Save emulator state to file
     * @param slot Save slot number (0-9)
     * @param rom_path Base ROM path for save file naming
     * @param cpu CPU state to save
     * @param memory Memory state to save
     * @param ppu PPU state to save
     * @return true on success
     */
    static bool save(int slot, const std::string& rom_path,
                     const CPU& cpu, const Memory& memory, const PPU& ppu);
    
    /**
     * Load emulator state from file
     * @param slot Save slot number (0-9)
     * @param rom_path Base ROM path for save file naming
     * @param cpu CPU state to restore
     * @param memory Memory state to restore
     * @param ppu PPU state to restore
     * @return true on success
     */
    static bool load(int slot, const std::string& rom_path,
                     CPU& cpu, Memory& memory, PPU& ppu);
    
    /**
     * Check if a save state exists for a slot
     * @param slot Save slot number (0-9)
     * @param rom_path Base ROM path
     * @return true if save state exists
     */
    static bool exists(int slot, const std::string& rom_path);
    
    /**
     * Get the file path for a save state slot
     * @param slot Save slot number (0-9)
     * @param rom_path Base ROM path
     * @return Full path to save state file
     */
    static std::string get_path(int slot, const std::string& rom_path);

private:
    // File format constants
    static constexpr u32 MAGIC = 0x47424353;  // "GBCS" in little-endian
    static constexpr u32 VERSION = 1;
    
    // Header structure
    struct Header {
        u32 magic;
        u32 version;
        u32 cpu_offset;
        u32 memory_offset;
        u32 ppu_offset;
    };
    
    // Helper functions for serialization
    static void write_u8(std::ofstream& file, u8 value);
    static void write_u16(std::ofstream& file, u16 value);
    static void write_u32(std::ofstream& file, u32 value);
    static void write_bytes(std::ofstream& file, const u8* data, size_t size);
    
    static u8 read_u8(std::ifstream& file);
    static u16 read_u16(std::ifstream& file);
    static u32 read_u32(std::ifstream& file);
    static void read_bytes(std::ifstream& file, u8* data, size_t size);
};

} // namespace gbglow
