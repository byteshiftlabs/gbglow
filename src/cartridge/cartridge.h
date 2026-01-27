#pragma once

#include "../core/types.h"
#include <vector>
#include <string>
#include <memory>

namespace emugbc {

/**
 * Cartridge Base Class
 * 
 * Game Boy cartridges contain:
 * - ROM (read-only memory)
 * - Optional RAM (battery-backed save data)
 * - Memory Bank Controller (MBC) for banking
 * 
 * Header is at 0x0100-0x014F in ROM
 */
class Cartridge {
public:
    virtual ~Cartridge() = default;
    
    /**
     * Load ROM from file
     */
    static std::unique_ptr<Cartridge> load_from_file(const std::string& path);
    
    /**
     * Read from cartridge address space (0x0000-0x7FFF ROM, 0xA000-0xBFFF RAM)
     */
    virtual u8 read(u16 address) const = 0;
    
    /**
     * Write to cartridge (typically for MBC control or RAM)
     */
    virtual void write(u16 address, u8 value) = 0;
    
    // Cartridge info
    const std::string& title() const { return title_; }
    u8 cartridge_type() const { return cartridge_type_; }
    bool has_battery() const { return has_battery_; }
    
protected:
    std::vector<u8> rom_;
    std::vector<u8> ram_;
    
    std::string title_;
    u8 cartridge_type_;
    bool has_battery_;
    
    /**
     * Parse cartridge header at 0x0100-0x014F
     */
    void parse_header();
};

/**
 * Simple ROM-only cartridge (no MBC)
 * Maximum 32KB ROM, no RAM
 */
class ROMOnly : public Cartridge {
public:
    explicit ROMOnly(std::vector<u8> rom_data);
    
    u8 read(u16 address) const override;
    void write(u16 address, u8 value) override;
};

} // namespace emugbc
