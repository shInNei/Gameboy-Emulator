#include "core/system.hpp"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: gb_test_runner <path_to_rom.gb> [max_frames]" << std::endl;
        return 1;
    }

    std::string rom_path = argv[1];
    uint32_t max_frames = (argc >= 3) ? std::stoul(argv[2]) : 600; // Default 10 seconds of emulation

    if (!fs::exists(rom_path)) {
        std::cerr << "Error: ROM file not found: " << rom_path << std::endl;
        return 1;
    }

    gb::System system;

    // Capture serial port output
    std::string serial_log;
    system.get_serial().set_output_callback([&serial_log](char c) {
        serial_log += c;
        std::cout << c << std::flush;
    });

    if (!system.load_rom(rom_path)) {
        std::cerr << "Error: Failed to load ROM: " << rom_path << std::endl;
        return 1;
    }

    std::cout << "\n========================================================" << std::endl;
    std::cout << "Running Test ROM: " << fs::path(rom_path).filename().string() << std::endl;
    std::cout << "========================================================\n" << std::endl;

    const auto& ppu = system.get_ppu();
    const auto& cpu = system.get_cpu();
    const auto& regs = cpu.get_registers();
    const auto& mmu = system.get_mmu();
    const auto& ints = system.get_interrupts();

    for (uint32_t frame = 0; frame < max_frames; ++frame) {
        system.step_frame();
    }
    std::cout << "\n=== END OF RUN EMULATOR STATE ===" << std::endl;
    std::cout << "CPU PC: 0x" << std::hex << regs.pc << " SP: 0x" << regs.sp << " A: 0x" << (int)regs.a << " F: 0x" << (int)regs.f << std::endl;
    std::cout << "IME: " << (int)ints.ime() << " IE: 0x" << (int)mmu.read(0xFFFF) << " IF: 0x" << (int)mmu.read(0xFF0F) << std::endl;
    std::cout << "C301: 0x" << std::hex << (int)mmu.read(0xC301) << " C203: 0x" << (int)mmu.read(0xC203) << " C204: 0x" << (int)mmu.read(0xC204) << std::endl;
    std::cout << "PPU LCDC (0xFF40): 0x" << std::hex << (int)ppu.read_register(0xFF40) << std::endl;
    std::cout << "PPU STAT (0xFF41): 0x" << std::hex << (int)ppu.read_register(0xFF41) << std::endl;
    std::cout << "PPU SCY  (0xFF42): 0x" << std::hex << (int)ppu.read_register(0xFF42) << std::endl;
    std::cout << "PPU SCX  (0xFF43): 0x" << std::hex << (int)ppu.read_register(0xFF43) << std::endl;
    std::cout << "PPU BGP  (0xFF47): 0x" << std::hex << (int)ppu.read_register(0xFF47) << std::endl;
    std::cout << "PPU OBP0 (0xFF48): 0x" << std::hex << (int)ppu.read_register(0xFF48) << std::endl;
    std::cout << "PPU OBP1 (0xFF49): 0x" << std::hex << (int)ppu.read_register(0xFF49) << std::endl;
    std::cout << "PPU WY   (0xFF4A): 0x" << std::hex << (int)ppu.read_register(0xFF4A) << std::endl;
    std::cout << "PPU WX   (0xFF4B): 0x" << std::hex << (int)ppu.read_register(0xFF4B) << std::endl;

    size_t b0 = 0, b1 = 0, b2 = 0, m0 = 0, m1 = 0;
    for (uint16_t a = 0x8000; a < 0x8800; ++a) if (ppu.read_vram(a, 0) != 0) b0++;
    for (uint16_t a = 0x8800; a < 0x9000; ++a) if (ppu.read_vram(a, 0) != 0) b1++;
    for (uint16_t a = 0x9000; a < 0x9800; ++a) if (ppu.read_vram(a, 0) != 0) b2++;
    for (uint16_t a = 0x9800; a < 0x9C00; ++a) if (ppu.read_vram(a, 0) != 0) m0++;
    for (uint16_t a = 0x9C00; a < 0xA000; ++a) if (ppu.read_vram(a, 0) != 0) m1++;

    std::cout << "VRAM Block 0x8000-0x87FF: " << std::dec << b0 << " / 2048" << std::endl;
    std::cout << "VRAM Block 0x8800-0x8FFF: " << std::dec << b1 << " / 2048" << std::endl;
    std::cout << "VRAM Block 0x9000-0x97FF: " << std::dec << b2 << " / 2048" << std::endl;
    std::cout << "VRAM Map   0x9800-0x9BFF: " << std::dec << m0 << " / 1024" << std::endl;
    std::cout << "VRAM Map   0x9C00-0x9FFF: " << std::dec << m1 << " / 1024" << std::endl;

    std::cout << "Tile Map 0x9800 (BG):" << std::endl;
    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 16; ++x) {
            std::cout << std::hex << (int)ppu.read_vram(0x9800 + y * 32 + x, 0) << " ";
        }
        std::cout << std::endl;
    }

    std::cout << "Tile Data 0x9000 (Tile 0, 1, 2):" << std::endl;
    for (int t = 0; t < 4; ++t) {
        std::cout << "Tile " << t << ": ";
        for (int b = 0; b < 16; ++b) {
            std::cout << std::hex << (int)ppu.read_vram(0x9000 + t * 16 + b, 0) << " ";
        }
        std::cout << std::endl;
    }

    return 0;
}
