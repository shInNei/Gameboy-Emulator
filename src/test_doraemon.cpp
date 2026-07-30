#include "core/system.hpp"
#include <iostream>
#include <iomanip>

int main() {
    gb::System system;
    if (!system.load_rom("src/rom/doraemon/Doraemon (J).gb")) {
        std::cerr << "Failed to load ROM!" << std::endl;
        return 1;
    }

    std::cout << "Cartridge Header:" << std::endl;
    auto& h = system.get_cartridge().header();
    std::cout << "Title: " << h.title << std::endl;
    std::cout << "Type: 0x" << std::hex << (int)h.type << std::endl;
    std::cout << "ROM size: " << std::dec << h.rom_size << std::endl;
    std::cout << "RAM size: " << std::dec << h.ram_size << std::endl;

    for (int frame = 0; frame < 100; ++frame) {
        system.step_frame();
    }

    std::cout << "\nDisassembly around 0xF0A3 (or 0xD0A3):" << std::endl;
    for (uint16_t addr = 0xF095; addr <= 0xF0B5; ++addr) {
        std::cout << "0x" << std::hex << std::setw(4) << std::setfill('0') << addr 
                  << ": 0x" << std::setw(2) << (int)system.get_mmu().read(addr) << std::endl;
    }

    return 0;
}
