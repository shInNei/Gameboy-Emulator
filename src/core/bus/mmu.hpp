#pragma once

#include "core/types.hpp"
#include "core/quirks.hpp"
#include "core/bus/interrupt_controller.hpp"
#include "core/cartridge/cartridge.hpp"
#include "core/ppu/ppu.hpp"
#include "core/apu/apu.hpp"
#include "core/timer/timer.hpp"
#include "core/joypad/joypad.hpp"
#include "core/serial/serial.hpp"
#include <array>

namespace gb {

class MMU {
public:
    MMU(InterruptController& interrupts,
        Cartridge& cartridge,
        PPU& ppu,
        APU& apu,
        Timer& timer,
        Joypad& joypad,
        Serial& serial,
        const HardwareQuirks& quirks)
        : interrupts(interrupts), cartridge(cartridge), ppu(ppu),
          apu(apu), timer(timer), joypad(joypad), serial(serial), quirks(quirks) {}

    void reset();

    [[nodiscard]] u8 read(Address addr) const;
    void write(Address addr, u8 val);

    [[nodiscard]] u16 read16(Address addr) const {
        return read(addr) | (static_cast<u16>(read(addr + 1)) << 8);
    }

    void write16(Address addr, u16 val) {
        write(addr, static_cast<u8>(val & 0xFF));
        write(addr + 1, static_cast<u8>((val >> 8) & 0xFF));
    }

    void tick_dma(u16 t_cycles);

    [[nodiscard]] u8 get_key1() const { return key1_reg; }
    void set_key1(u8 val) { key1_reg = val; }

    [[nodiscard]] const std::array<std::array<u8, 0x1000>, 8>& wram_banks() const { return wram; }
    [[nodiscard]] const std::array<u8, 0x7F>& hram_bytes() const { return hram; }

private:
    void start_oam_dma(u8 base_addr_high);

    InterruptController& interrupts;
    Cartridge& cartridge;
    PPU& ppu;
    APU& apu;
    Timer& timer;
    Joypad& joypad;
    Serial& serial;
    const HardwareQuirks& quirks;

    std::array<std::array<u8, 0x1000>, 8> wram{}; // WRAM Bank 0..7
    std::array<u8, 0x7F> hram{};                  // HRAM 0xFF80..0xFFFE
    u8 wram_bank_sel{1};

    // OAM DMA State
    bool dma_transferring{false};
    u16 dma_source_addr{0};
    u8 dma_byte_index{0};
    u16 dma_cycle_accumulator{0};
    u8 key1_reg{0x7E};
};

} // namespace gb
