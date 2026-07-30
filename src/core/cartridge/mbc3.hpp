#pragma once

#include "cartridge.hpp"
#include <chrono>

namespace gb {

class MBC3 : public MBC {
public:
    explicit MBC3(const std::vector<u8>& rom_data, size_t ram_size)
        : rom(rom_data), ram(ram_size, 0) {}

    u8 read_rom(Address addr) override {
        if (addr <= 0x3FFF) {
            return addr < rom.size() ? rom[addr] : 0xFF;
        } else if (addr >= 0x4000 && addr <= 0x7FFF) {
            size_t bank = rom_bank == 0 ? 1 : rom_bank;
            size_t final_bank = bank % (rom.size() / 0x4000);
            size_t offset = (final_bank * 0x4000) + (addr - 0x4000);
            return offset < rom.size() ? rom[offset] : 0xFF;
        }
        return 0xFF;
    }

    void write_rom(Address addr, u8 val) override {
        if (addr <= 0x1FFF) {
            ram_rtc_enabled = ((val & 0x0F) == 0x0A);
        } else if (addr >= 0x2000 && addr <= 0x3FFF) {
            rom_bank = val & 0x7F;
            if (rom_bank == 0) rom_bank = 1;
        } else if (addr >= 0x4000 && addr <= 0x5FFF) {
            ram_rtc_bank = val;
        } else if (addr >= 0x6000 && addr <= 0x7FFF) {
            if (rtc_latch_written && val == 0x01) {
                latch_rtc();
            }
            rtc_latch_written = (val == 0x00);
        }
    }

    u8 read_ram(Address addr) override {
        if (!ram_rtc_enabled) return 0xFF;

        if (ram_rtc_bank <= 0x03) {
            size_t offset = (ram_rtc_bank * 0x2000) + (addr - 0xA000);
            return offset < ram.size() ? ram[offset] : 0xFF;
        } else if (ram_rtc_bank >= 0x08 && ram_rtc_bank <= 0x0C) {
            return rtc_registers[ram_rtc_bank - 0x08];
        }
        return 0xFF;
    }

    void write_ram(Address addr, u8 val) override {
        if (!ram_rtc_enabled) return;

        if (ram_rtc_bank <= 0x03) {
            size_t offset = (ram_rtc_bank * 0x2000) + (addr - 0xA000);
            if (offset < ram.size()) ram[offset] = val;
        } else if (ram_rtc_bank >= 0x08 && ram_rtc_bank <= 0x0C) {
            rtc_registers[ram_rtc_bank - 0x08] = val;
        }
    }

    std::vector<u8>& get_ram() override { return ram; }
    void set_ram(const std::vector<u8>& data) override {
        if (data.size() == ram.size()) ram = data;
    }

private:
    void latch_rtc() {
        auto now = std::chrono::system_clock::now().time_since_epoch();
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(now).count();
        rtc_registers[0] = static_cast<u8>(seconds % 60);
        rtc_registers[1] = static_cast<u8>((seconds / 60) % 60);
        rtc_registers[2] = static_cast<u8>((seconds / 3600) % 24);
        u64 days = seconds / 86400;
        rtc_registers[3] = static_cast<u8>(days & 0xFF);
        rtc_registers[4] = static_cast<u8>((days >> 8) & 0x01);
    }

    const std::vector<u8>& rom;
    std::vector<u8> ram;

    bool ram_rtc_enabled{false};
    u8 rom_bank{1};
    u8 ram_rtc_bank{0};
    bool rtc_latch_written{false};
    u8 rtc_registers[5]{0, 0, 0, 0, 0}; // S, M, H, DL, DH
};

} // namespace gb
