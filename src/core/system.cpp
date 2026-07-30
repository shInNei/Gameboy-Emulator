#include <iostream>
#include "system.hpp"

namespace gb {

System::System()
    : ppu(interrupts, quirks),
      apu(quirks),
      timer(interrupts, quirks),
      joypad(interrupts),
      serial(interrupts),
      mmu(interrupts, cartridge, ppu, apu, timer, joypad, serial, quirks),
      cpu(regs, interrupts, mmu, timer, ppu, apu, serial, quirks) {
    reset();
}

void System::reset() {
    if (cartridge.is_loaded() && cartridge.header().is_cgb) {
        quirks.set_preset(HardwareModel::CGB_RevC);
    } else {
        quirks.set_preset(HardwareModel::DMG_RevB);
    }
    regs = Registers{};
    interrupts.reset();
    ppu.reset();
    apu.reset();
    timer.reset();
    joypad.reset();
    serial.reset();
    mmu.reset();
    cpu.reset();
    paused = false;
}

bool System::load_rom(const std::string& filepath) {
    if (cartridge.load_rom_file(filepath)) {
        reset();
        return true;
    }
    return false;
}

bool System::load_rom(const std::vector<u8>& rom_bytes) {
    if (cartridge.load_rom(rom_bytes)) {
        reset();
        return true;
    }
    return false;
}

void System::step_instruction() {
    cpu.step();
}

void System::step_frame() {
    if (paused) return;

    clear_frame_ready();
    Cycles start_cycles = cpu.cycle_count();
    while (cpu.cycle_count() - start_cycles < CYCLES_PER_FRAME) {
        cpu.step();
        if (is_frame_ready()) break;
    }
}

} // namespace gb
