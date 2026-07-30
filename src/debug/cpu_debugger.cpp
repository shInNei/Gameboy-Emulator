#include "cpu_debugger.hpp"
#include <format>

namespace gb {

DisassembledInstruction CPUDebugger::disassemble_at(System& system, Address addr) {
    DisassembledInstruction inst;
    inst.address = addr;

    auto& mmu = system.get_mmu();
    u8 op = mmu.read(addr);

    if (op == 0xCB) {
        u8 cb_op = mmu.read(addr + 1);
        inst.length = 2;
        inst.bytes_hex = std::format("CB {:02X}", cb_op);
        inst.mnemonic = std::format("CB Opcode {:02X}", cb_op);
        return inst;
    }

    // Simplistic mnemonic generator for debugger view
    inst.length = 1;
    inst.bytes_hex = std::format("{:02X}", op);

    switch (op) {
        case 0x00: inst.mnemonic = "NOP"; break;
        case 0x01: { u16 n16 = mmu.read16(addr + 1); inst.length = 3; inst.bytes_hex = std::format("01 {:04X}", n16); inst.mnemonic = std::format("LD BC, ${:04X}", n16); break; }
        case 0x02: inst.mnemonic = "LD (BC), A"; break;
        case 0x03: inst.mnemonic = "INC BC"; break;
        case 0x04: inst.mnemonic = "INC B"; break;
        case 0x05: inst.mnemonic = "DEC B"; break;
        case 0x06: { u8 n8 = mmu.read(addr + 1); inst.length = 2; inst.bytes_hex = std::format("06 {:02X}", n8); inst.mnemonic = std::format("LD B, ${:02X}", n8); break; }
        case 0x18: { i8 e8 = static_cast<i8>(mmu.read(addr + 1)); inst.length = 2; inst.bytes_hex = std::format("18 {:02X}", static_cast<u8>(e8)); inst.mnemonic = std::format("JR ${:04X}", addr + 2 + e8); break; }
        case 0x20: { i8 e8 = static_cast<i8>(mmu.read(addr + 1)); inst.length = 2; inst.bytes_hex = std::format("20 {:02X}", static_cast<u8>(e8)); inst.mnemonic = std::format("JR NZ, ${:04X}", addr + 2 + e8); break; }
        case 0x21: { u16 n16 = mmu.read16(addr + 1); inst.length = 3; inst.bytes_hex = std::format("21 {:04X}", n16); inst.mnemonic = std::format("LD HL, ${:04X}", n16); break; }
        case 0x28: { i8 e8 = static_cast<i8>(mmu.read(addr + 1)); inst.length = 2; inst.bytes_hex = std::format("28 {:02X}", static_cast<u8>(e8)); inst.mnemonic = std::format("JR Z, ${:04X}", addr + 2 + e8); break; }
        case 0x31: { u16 n16 = mmu.read16(addr + 1); inst.length = 3; inst.bytes_hex = std::format("31 {:04X}", n16); inst.mnemonic = std::format("LD SP, ${:04X}", n16); break; }
        case 0x3E: { u8 n8 = mmu.read(addr + 1); inst.length = 2; inst.bytes_hex = std::format("3E {:02X}", n8); inst.mnemonic = std::format("LD A, ${:02X}", n8); break; }
        case 0xC3: { u16 n16 = mmu.read16(addr + 1); inst.length = 3; inst.bytes_hex = std::format("C3 {:04X}", n16); inst.mnemonic = std::format("JP ${:04X}", n16); break; }
        case 0xCD: { u16 n16 = mmu.read16(addr + 1); inst.length = 3; inst.bytes_hex = std::format("CD {:04X}", n16); inst.mnemonic = std::format("CALL ${:04X}", n16); break; }
        case 0xE0: { u8 n8 = mmu.read(addr + 1); inst.length = 2; inst.bytes_hex = std::format("E0 {:02X}", n8); inst.mnemonic = std::format("LDH (${:04X}), A", 0xFF00 + n8); break; }
        case 0xEA: { u16 n16 = mmu.read16(addr + 1); inst.length = 3; inst.bytes_hex = std::format("EA {:04X}", n16); inst.mnemonic = std::format("LD (${:04X}), A", n16); break; }
        case 0xF0: { u8 n8 = mmu.read(addr + 1); inst.length = 2; inst.bytes_hex = std::format("F0 {:02X}", n8); inst.mnemonic = std::format("LDH A, (${:04X})", 0xFF00 + n8); break; }
        case 0xFA: { u16 n16 = mmu.read16(addr + 1); inst.length = 3; inst.bytes_hex = std::format("FA {:04X}", n16); inst.mnemonic = std::format("LD A, (${:04X})", n16); break; }
        default: inst.mnemonic = std::format("OP {:02X}", op); break;
    }

    return inst;
}

std::vector<DisassembledInstruction> CPUDebugger::disassemble_range(System& system, Address start_addr, int count) {
    std::vector<DisassembledInstruction> result;
    Address curr = start_addr;
    for (int i = 0; i < count; ++i) {
        auto inst = disassemble_at(system, curr);
        result.push_back(inst);
        curr += inst.length;
    }
    return result;
}

} // namespace gb
