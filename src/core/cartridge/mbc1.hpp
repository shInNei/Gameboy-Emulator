#pragma once

#include "cartridge.hpp"

namespace gb {

class MBC1 : public MBC {
public:
    explicit MBC1(const std::vector<u8>& rom_data, size_t ram_size)
        : rom(rom_data), ram(ram_size, 0) {}

    u8 read_rom(Address addr) override {
        if (addr <= 0x3FFF) {
            size_t bank = (banking_mode == 1) ? (secondary_bank << 5) : 0;
            size_t final_bank = bank % (rom.size() / 0x4000);
            size_t offset = (final_bank * 0x4000) + addr;
            return offset < rom.size() ? rom[offset] : 0xFF;
        } else if (addr >= 0x4000 && addr <= 0x7FFF) {
            size_t bank = (secondary_bank << 5) | rom_bank;
            if ((rom_bank & 0x1F) == 0) bank |= 1; // Bank 0, 0x20, 0x40, 0x60 translated to +1
            size_t final_bank = bank % (rom.size() / 0x4000);
            size_t offset = (final_bank * 0x4000) + (addr - 0x4000);
            return offset < rom.size() ? rom[offset] : 0xFF;
        }
        return 0xFF;
    }

    void write_rom(Address addr, u8 val) override {
        if (addr <= 0x1FFF) {
            ram_enabled = ((val & 0x0F) == 0x0A);
        } else if (addr >= 0x2000 && addr <= 0x3FFF) {
            rom_bank = val & 0x1F;
            if (rom_bank == 0) rom_bank = 1;
        } else if (addr >= 0x4000 && addr <= 0x5FFF) {
            secondary_bank = val & 0x03;
        } else if (addr >= 0x6000 && addr <= 0x7FFF) {
            banking_mode = val & 0x01;
        }
    }

    u8 read_ram(Address addr) override {
        if (!ram_enabled || ram.empty()) return 0xFF;
        size_t bank = (banking_mode == 1) ? secondary_bank : 0;
        size_t offset = (bank * 0x2000) + (addr - 0xA000);
        return offset < ram.size() ? ram[offset] : 0xFF;
    }

    void write_ram(Address addr, u8 val) override {
        if (!ram_enabled || ram.empty()) return;
        size_t bank = (banking_mode == 1) ? secondary_bank : 0;
        size_t offset = (bank * 0x2000) + (addr - 0xA000);
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
    u8 rom_bank{1};
    u8 secondary_bank{0};
    u8 banking_mode{0};
};

} // namespace gb
