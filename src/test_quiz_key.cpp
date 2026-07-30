#include "core/system.hpp"
#include <iostream>
#include <iomanip>

int main() {
    gb::System system;
    if (!system.load_rom("src/rom/capcom_quiz/Capcom Quiz (J).gb")) {
        std::cerr << "Failed to load ROM!" << std::endl;
        return 1;
    }

    std::cout << "Disassembly at 0x1BBB to 0x1BE0 in Capcom Quiz:" << std::endl;
    for (uint16_t addr = 0x1BBB; addr <= 0x1BE0; ++addr) {
        std::cout << "0x" << std::hex << std::setw(4) << std::setfill('0') << addr 
                  << ": 0x" << std::setw(2) << (int)system.get_mmu().read(addr) << std::endl;
    }

    return 0;
}
