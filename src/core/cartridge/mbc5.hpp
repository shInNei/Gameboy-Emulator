#pragma once

#include "cartridge.hpp"

namespace gb {

class MBC5 : public MBC {
public:
    explicit MBC5(const std::vector<u8>& rom_data, size_t ram_size)
        : rom(rom_data), ram(ram_size, 0) {}

    u8 read_rom(Address addr) override {
        if (addr <= 0x3FFF) {
            return addr < rom.size() ? rom[addr] : 0xFF;
        } else if (addr >= 0x4000 && addr <= 0x7FFF) {
            size_t bank = (static_cast<size_t>(rom_bank_high & 0x01) << 8) | rom_bank_low;
            size_t final_bank = bank % (rom.size() / 0x4000);
            size_t offset = (final_bank * 0x4000) + (addr - 0x4000);
            return offset < rom.size() ? rom[offset] : 0xFF;
        }
        return 0xFF;
    }

    void write_rom(Address addr, u8 val) override {
        if (addr <= 0x1FFF) {
            ram_enabled = ((val & 0x0F) == 0x0A);
        } else if (addr >= 0x2000 && addr <= 0x2FFF) {
            rom_bank_low = val;
        } else if (addr >= 0x3000 && addr <= 0x3FFF) {
            rom_bank_high = val & 0x01;
        } else if (addr >= 0x4000 && addr <= 0x5FFF) {
            ram_bank = val & 0x0F;
        }
    }

    u8 read_ram(Address addr) override {
        if (!ram_enabled || ram.empty()) return 0xFF;
        size_t offset = (ram_bank * 0x2000) + (addr - 0xA000);
        return offset < ram.size() ? ram[offset] : 0xFF;
    }

    void write_ram(Address addr, u8 val) override {
        if (!ram_enabled || ram.empty()) return;
        size_t offset = (ram_bank * 0x2000) + (addr - 0xA000);
        if (offset < ram.size()) ram[offset] = val;
    }

    std::vector<u8>& get_ram() override { return ram; }
    void set_ram(const std::vector<u8>& data) override {
        if (data.size() == ram.size()) ram = data;
    }

private:
    const std::vector<u8>& rom;
    std::vector<u8> ram;

    bool ram_enabled{false};
    u8 rom_bank_low{1};
    u8 rom_bank_high{0};
    u8 ram_bank{0};
};

} // namespace gb
