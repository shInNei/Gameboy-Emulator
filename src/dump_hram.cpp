#include "core/system.hpp"
#include <iostream>

int main() {
    gb::System system;
    system.load_rom("src/rom/capcom_quiz/Capcom Quiz (J).gb");

    for (int i = 0; i < 200; ++i) {
        system.step_frame();
    }

    std::cout << "HRAM 0xFF80 to 0xFF8F contents:" << std::endl;
    for (uint16_t a = 0xFF80; a <= 0xFF8F; ++a) {
        std::cout << "0x" << std::hex << a << ": 0x" << (int)system.get_mmu().read(a) << std::endl;
    }

    return 0;
}
