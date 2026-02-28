#pragma once

#include "../core/types.h"
#include <vector>
#include <string>
#include <memory>

namespace gbglow {

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
    static std::unique_ptr<Cartridge> load_rom_from_file(const std::string& path);
    
    /**
     * Read from cartridge address space (0x0000-0x7FFF ROM, 0xA000-0xBFFF RAM)
     */
    virtual u8 read(u16 address) const = 0;
    
    /**
     * Write to cartridge (typically for MBC control or RAM)
     */
    virtual void write(u16 address, u8 value) = 0;
    
    /**
     * Save RAM to file (for battery-backed saves)
     * Returns true on success, false on failure
     */
    virtual bool save_ram_to_file(const std::string& path);
    
    /**
     * Load RAM from file (for battery-backed saves)
     * Returns true on success, false if file doesn't exist or error
     */
    virtual bool load_ram_from_file(const std::string& path);
    
    // Cartridge info
    const std::string& title() const;
    u8 cartridge_type() const;
    bool has_battery() const;
    bool is_cgb_supported() const;  // Returns true if game supports CGB features
    bool is_cgb_only() const;        // Returns true if game requires CGB hardware
    
    // RAM access for save states
    std::vector<u8> get_ram_data() const { return ram_; }
    void set_ram_data(const std::vector<u8>& data) { 
        if (data.size() <= ram_.size()) {
            std::copy(data.begin(), data.end(), ram_.begin());
        }
    }
    
protected:
    /**
     * Constructor for derived cartridge types
     * Initializes ROM and RAM
     */
    Cartridge(std::vector<u8> rom_data, size_t ram_size)
        : rom_(std::move(rom_data))
        , ram_(ram_size)
        , title_()
        , cartridge_type_(0)
        , cgb_flag_(0)
        , has_battery_(false)
    {
        parse_header();
    }
    
    std::vector<u8> rom_;
    std::vector<u8> ram_;
    
    std::string title_;
    u8 cartridge_type_;
    u8 cgb_flag_;      // CGB support flag from header 0x0143
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

} // namespace gbglow
