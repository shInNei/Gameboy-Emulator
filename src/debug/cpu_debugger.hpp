#pragma once

#include "core/system.hpp"
#include <string>
#include <vector>
#include <set>

namespace gb {

struct DisassembledInstruction {
    Address address{0};
    std::string bytes_hex;
    std::string mnemonic;
    u8 length{1};
};

struct Breakpoint {
    Address address{0};
    bool enabled{true};
    bool is_write{false};
    bool is_read{false};
};

class CPUDebugger {
public:
    DisassembledInstruction disassemble_at(System& system, Address addr);
    std::vector<DisassembledInstruction> disassemble_range(System& system, Address start_addr, int count);

    void add_breakpoint(Address addr) { breakpoints.insert(addr); }
    void remove_breakpoint(Address addr) { breakpoints.erase(addr); }
    [[nodiscard]] bool has_breakpoint(Address addr) const { return breakpoints.contains(addr); }

    const std::set<Address>& get_breakpoints() const { return breakpoints; }

private:
    std::set<Address> breakpoints;
};

} // namespace gb
