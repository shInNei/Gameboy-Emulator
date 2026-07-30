#pragma once

#include "types.hpp"
#include "quirks.hpp"
#include "cpu/registers.hpp"
#include "cpu/cpu.hpp"
#include "bus/interrupt_controller.hpp"
#include "bus/mmu.hpp"
#include "cartridge/cartridge.hpp"
#include "ppu/ppu.hpp"
#include "apu/apu.hpp"
#include "timer/timer.hpp"
#include "joypad/joypad.hpp"
#include "serial/serial.hpp"

namespace gb {

class System {
public:
    System();

    void reset();
    bool load_rom(const std::string& filepath);
    bool load_rom(const std::vector<u8>& rom_bytes);

    void step_instruction();
    void step_frame();

    [[nodiscard]] const Framebuffer& get_framebuffer() const { return ppu.framebuffer(); }
    [[nodiscard]] bool is_frame_ready() const { return ppu.is_frame_ready(); }
    void clear_frame_ready() { ppu.clear_frame_ready(); }

    // Subsystem Getters
    HardwareQuirks& get_quirks() { return quirks; }
    const HardwareQuirks& get_quirks() const { return quirks; }

    Registers& get_registers() { return regs; }
    const Registers& get_registers() const { return regs; }

    CPU& get_cpu() { return cpu; }
    MMU& get_mmu() { return mmu; }
    Cartridge& get_cartridge() { return cartridge; }
    PPU& get_ppu() { return ppu; }
    APU& get_apu() { return apu; }
    Timer& get_timer() { return timer; }
    Joypad& get_joypad() { return joypad; }
    Serial& get_serial() { return serial; }
    InterruptController& get_interrupts() { return interrupts; }

    [[nodiscard]] bool is_paused() const { return paused; }
    void set_paused(bool val) { paused = val; }

private:
    HardwareQuirks quirks;
    Registers regs;
    InterruptController interrupts;
    Cartridge cartridge;
    PPU ppu;
    APU apu;
    Timer timer;
    Joypad joypad;
    Serial serial;
    MMU mmu;
    CPU cpu;

    bool paused{false};
};

} // namespace gb
