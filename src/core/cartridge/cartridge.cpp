#include "cartridge.hpp"
#include "mbc0.hpp"
#include "mbc1.hpp"
#include "mbc3.hpp"
#include "mbc5.hpp"
#include <fstream>
#include <iostream>

namespace gb {

bool Cartridge::load_rom(const std::vector<u8>& rom_bytes) {
    if (rom_bytes.size() < 0x0150) return false;
    rom = rom_bytes;
    parse_header();
    create_mbc();
    loaded = true;
    return true;
}

bool Cartridge::load_rom_file(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return false;

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<u8> buffer(size);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        return false;
    }
    return load_rom(buffer);
}

void Cartridge::parse_header() {
    // Title 0x0134 - 0x0143
    char title_buf[17] = {0};
    for (int i = 0; i < 16; ++i) {
        u8 c = rom[0x0134 + i];
        if (c == 0) break;
        title_buf[i] = static_cast<char>(c);
    }
    header_info.title = title_buf;

    // CGB Flag 0x0143
    u8 cgb_flag = rom[0x0143];
    header_info.is_cgb = (cgb_flag == 0x80 || cgb_flag == 0xC0);

    // Cartridge Type 0x0147
    header_info.type = static_cast<CartridgeType>(rom[0x0147]);

    // ROM Size 0x0148
    u8 rom_code = rom[0x0148];
    header_info.rom_size = (32 * 1024) << rom_code;

    // RAM Size 0x0149
    u8 ram_code = rom[0x0149];
    switch (ram_code) {
        case 0x00: header_info.ram_size = 0; break;
        case 0x01: header_info.ram_size = 2 * 1024; break;
        case 0x02: header_info.ram_size = 8 * 1024; break;
        case 0x03: header_info.ram_size = 32 * 1024; break;
        case 0x04: header_info.ram_size = 128 * 1024; break;
        case 0x05: header_info.ram_size = 64 * 1024; break;
        default:   header_info.ram_size = 0; break;
    }

    // Checksum 0x014D
    u8 checksum = 0;
    for (Address addr = 0x0134; addr <= 0x014C; ++addr) {
        checksum = checksum - rom[addr] - 1;
    }
    header_info.checksum = checksum;
    header_info.valid_checksum = (checksum == rom[0x014D]);
}

void Cartridge::create_mbc() {
    switch (header_info.type) {
        case CartridgeType::ROM_ONLY:
            mbc = std::make_unique<MBC0>(rom, header_info.ram_size);
            break;
        case CartridgeType::MBC1:
        case CartridgeType::MBC1_RAM:
        case CartridgeType::MBC1_RAM_BATTERY:
            mbc = std::make_unique<MBC1>(rom, header_info.ram_size);
            break;
        case CartridgeType::MBC3:
        case CartridgeType::MBC3_RAM:
        case CartridgeType::MBC3_RAM_BATTERY:
        case CartridgeType::MBC3_TIMER_BATTERY:
        case CartridgeType::MBC3_TIMER_RAM_BATTERY:
            mbc = std::make_unique<MBC3>(rom, header_info.ram_size);
            break;
        case CartridgeType::MBC5:
        case CartridgeType::MBC5_RAM:
        case CartridgeType::MBC5_RAM_BATTERY:
        case CartridgeType::MBC5_RUMBLE:
        case CartridgeType::MBC5_RUMBLE_RAM:
        case CartridgeType::MBC5_RUMBLE_RAM_BATTERY:
            mbc = std::make_unique<MBC5>(rom, header_info.ram_size);
            break;
        default:
            // Fallback to MBC1 for unsupported or general types
            mbc = std::make_unique<MBC1>(rom, header_info.ram_size);
            break;
    }
}

u8 Cartridge::read_rom(Address addr) const {
    if (mbc) return mbc->read_rom(addr);
    return 0xFF;
}

void Cartridge::write_rom(Address addr, u8 val) {
    if (mbc) mbc->write_rom(addr, val);
}

u8 Cartridge::read_ram(Address addr) const {
    if (mbc) return mbc->read_ram(addr);
    return 0xFF;
}

void Cartridge::write_ram(Address addr, u8 val) {
    if (mbc) mbc->write_ram(addr, val);
}

void Cartridge::save_battery(const std::string& save_path) {
    if (!mbc) return;
    auto& ram = mbc->get_ram();
    if (ram.empty()) return;

    std::ofstream file(save_path, std::ios::binary);
    if (file.is_open()) {
        file.write(reinterpret_cast<const char*>(ram.data()), ram.size());
    }
}

void Cartridge::load_battery(const std::string& save_path) {
    if (!mbc) return;
    std::ifstream file(save_path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return;

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<u8> buffer(size);
    if (file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        mbc->set_ram(buffer);
    }
}

} // namespace gb
