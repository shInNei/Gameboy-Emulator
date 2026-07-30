#include "mmu.hpp"
#include <iostream>

namespace gb {

void MMU::reset() {
    for (auto& bank : wram) bank.fill(0);
    hram.fill(0);
    wram_bank_sel = 1;

    dma_transferring = false;
    dma_source_addr = 0;
    dma_byte_index = 0;
}

u8 MMU::read(Address addr) const {
    if (addr <= 0x7FFF) {
        return cartridge.read_rom(addr);
    } else if (addr >= 0x8000 && addr <= 0x9FFF) {
        return ppu.read_vram(addr);
    } else if (addr >= 0xA000 && addr <= 0xBFFF) {
        return cartridge.read_ram(addr);
    } else if (addr >= 0xC000 && addr <= 0xCFFF) {
        return wram[0][addr - 0xC000];
    } else if (addr >= 0xD000 && addr <= 0xDFFF) {
        return wram[wram_bank_sel][addr - 0xD000];
    } else if (addr >= 0xE000 && addr <= 0xFDFF) {
        return read(addr - 0x2000); // Echo RAM
    } else if (addr >= 0xFE00 && addr <= 0xFE9F) {
        return ppu.read_oam(addr);
    } else if (addr >= 0xFEA0 && addr <= 0xFEFF) {
        return 0xFF; // Unusable
    } else if (addr >= 0xFF00 && addr <= 0xFF7F) {
        // IO Registers
        if (addr == 0xFF00) return joypad.read();
        if (addr == 0xFF01) return serial.read_sb();
        if (addr == 0xFF02) return serial.read_sc();
        if (addr == 0xFF04) return timer.read_div();
        if (addr == 0xFF05) return timer.read_tima();
        if (addr == 0xFF06) return timer.read_tma();
        if (addr == 0xFF07) return timer.read_tac();
        if (addr == 0xFF0F) return interrupts.read_if();
        if (addr >= 0xFF10 && addr <= 0xFF3F) {
            if (addr >= 0xFF30) return apu.read_wave_ram(addr);
            return apu.read_register(addr);
        }
        if (addr == 0xFF46) return 0xFF; // DMA write only, or 0xFF
        if (addr == 0xFF4D) return key1_reg | 0x7E;
        if (addr >= 0xFF40 && addr <= 0xFF4B) return ppu.read_register(addr);
        if (addr == 0xFF4F) return ppu.read_register(addr);
        if (addr == 0xFF70) return wram_bank_sel | 0xF8;
        return 0xFF;
    } else if (addr >= 0xFF80 && addr <= 0xFFFE) {
        return hram[addr - 0xFF80];
    } else if (addr == 0xFFFF) {
        return interrupts.read_ie();
    }
    return 0xFF;
}

void MMU::write(Address addr, u8 val) {
    if (addr <= 0x7FFF) {
        cartridge.write_rom(addr, val);
    } else if (addr >= 0x8000 && addr <= 0x9FFF) {
        ppu.write_vram(addr, val);
    } else if (addr >= 0xA000 && addr <= 0xBFFF) {
        cartridge.write_ram(addr, val);
    } else if (addr >= 0xC000 && addr <= 0xCFFF) {
                wram[0][addr - 0xC000] = val;
    } else if (addr >= 0xD000 && addr <= 0xDFFF) {
        wram[wram_bank_sel][addr - 0xD000] = val;
    } else if (addr >= 0xE000 && addr <= 0xFDFF) {
        write(addr - 0x2000, val); // Echo RAM
    } else if (addr >= 0xFE00 && addr <= 0xFE9F) {
        ppu.write_oam(addr, val);
    } else if (addr >= 0xFEA0 && addr <= 0xFEFF) {
        // Unusable RAM writes ignored
    } else if (addr >= 0xFF00 && addr <= 0xFF7F) {
        // IO Registers
        if (addr == 0xFF00) joypad.write(val);
        else if (addr == 0xFF01) serial.write_sb(val);
        else if (addr == 0xFF02) serial.write_sc(val);
        else if (addr == 0xFF04) timer.write_div(val);
        else if (addr == 0xFF05) timer.write_tima(val);
        else if (addr == 0xFF06) timer.write_tma(val);
        else if (addr == 0xFF07) timer.write_tac(val);
        else if (addr == 0xFF0F) interrupts.write_if(val);
        else if (addr >= 0xFF10 && addr <= 0xFF3F) {
            if (addr >= 0xFF30) apu.write_wave_ram(addr, val);
            else apu.write_register(addr, val);
        }
        else if (addr == 0xFF46) start_oam_dma(val);
        else if (addr == 0xFF4D) key1_reg = (key1_reg & 0x80) | (val & 0x01) | 0x7E;
        else if (addr >= 0xFF40 && addr <= 0xFF4B) ppu.write_register(addr, val);
        else if (addr == 0xFF4F) ppu.write_register(addr, val);
        else if (addr == 0xFF70) {
            u8 b = val & 0x07;
            wram_bank_sel = (b == 0) ? 1 : b;
        }
    } else if (addr >= 0xFF80 && addr <= 0xFFFE) {
        hram[addr - 0xFF80] = val;
    } else if (addr == 0xFFFF) {
        interrupts.write_ie(val);
    }
}

void MMU::start_oam_dma(u8 base_addr_high) {
    dma_transferring = true;
    dma_source_addr = static_cast<u16>(base_addr_high) << 8;
    dma_byte_index = 0;
    dma_cycle_accumulator = 0;
}

void MMU::tick_dma(u16 t_cycles) {
    if (!dma_transferring) return;

    // OAM DMA transfers 1 byte per 4 T-cycles (1 normal M-cycle)
    dma_cycle_accumulator += t_cycles;

    while (dma_cycle_accumulator >= 4 && dma_transferring) {
        dma_cycle_accumulator -= 4;
        
        u16 src = dma_source_addr + dma_byte_index;
        u8 val = read(src);
        ppu.write_oam(0xFE00 + dma_byte_index, val);

        dma_byte_index++;
        if (dma_byte_index >= 160) {
            dma_transferring = false;
        }
    }
}

} // namespace gb
