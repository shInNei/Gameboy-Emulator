#pragma once

#include "core/types.hpp"
#include <vector>
#include <string>
#include <memory>
#include <filesystem>

namespace gb {

enum class CartridgeType : u8 {
    ROM_ONLY = 0x00,
    MBC1 = 0x01,
    MBC1_RAM = 0x02,
    MBC1_RAM_BATTERY = 0x03,
    MBC2 = 0x05,
    MBC2_BATTERY = 0x06,
    MBC3_TIMER_BATTERY = 0x0F,
    MBC3_TIMER_RAM_BATTERY = 0x10,
    MBC3 = 0x11,
    MBC3_RAM = 0x12,
    MBC3_RAM_BATTERY = 0x13,
    MBC5 = 0x19,
    MBC5_RAM = 0x1A,
    MBC5_RAM_BATTERY = 0x1B,
    MBC5_RUMBLE = 0x1C,
    MBC5_RUMBLE_RAM = 0x1D,
    MBC5_RUMBLE_RAM_BATTERY = 0x1E
};

struct CartridgeHeader {
    std::string title;
    bool is_cgb{false};
    CartridgeType type{CartridgeType::ROM_ONLY};
    size_t rom_size{0};
    size_t ram_size{0};
    u8 checksum{0};
    bool valid_checksum{false};
};

class MBC {
public:
    virtual ~MBC() = default;
    virtual u8 read_rom(Address addr) = 0;
    virtual void write_rom(Address addr, u8 val) = 0;
    virtual u8 read_ram(Address addr) = 0;
    virtual void write_ram(Address addr, u8 val) = 0;

    virtual std::vector<u8>& get_ram() = 0;
    virtual void set_ram(const std::vector<u8>& data) = 0;
};

class Cartridge {
public:
    bool load_rom(const std::vector<u8>& rom_bytes);
    bool load_rom_file(const std::string& filepath);

    u8 read_rom(Address addr) const;
    void write_rom(Address addr, u8 val);
    u8 read_ram(Address addr) const;
    void write_ram(Address addr, u8 val);

    [[nodiscard]] const CartridgeHeader& header() const { return header_info; }
    [[nodiscard]] bool is_loaded() const { return loaded; }
    [[nodiscard]] const std::vector<u8>& rom_data() const { return rom; }

    void save_battery(const std::string& save_path);
    void load_battery(const std::string& save_path);

private:
    void parse_header();
    void create_mbc();

    std::vector<u8> rom;
    CartridgeHeader header_info;
    std::unique_ptr<MBC> mbc;
    bool loaded{false};
};

} // namespace gb
