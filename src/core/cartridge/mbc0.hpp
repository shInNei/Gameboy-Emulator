#pragma once

#include "cartridge.hpp"

namespace gb {

class MBC0 : public MBC {
public:
    explicit MBC0(const std::vector<u8>& rom_data, size_t ram_size)
        : rom(rom_data), ram(ram_size, 0) {}

    u8 read_rom(Address addr) override {
        if (addr < rom.size()) return rom[addr];
        return 0xFF;
    }

    void write_rom(Address, u8) override {
        // ROM is read-only
    }

    u8 read_ram(Address addr) override {
        size_t offset = addr - 0xA000;
        if (offset < ram.size()) return ram[offset];
        return 0xFF;
    }

    void write_ram(Address addr, u8 val) override {
        size_t offset = addr - 0xA000;
        if (offset < ram.size()) ram[offset] = val;
    }

    std::vector<u8>& get_ram() override { return ram; }
    void set_ram(const std::vector<u8>& data) override {
        if (data.size() == ram.size()) ram = data;
    }

private:
    const std::vector<u8>& rom;
    std::vector<u8> ram;
};

} // namespace gb
