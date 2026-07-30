#include "savestate.hpp"
#include "core/system.hpp"
#include <cstring>

namespace gb {

struct StreamBuffer {
    std::vector<u8> data;

    template <typename T>
    void write(const T& val) {
        const u8* bytes = reinterpret_cast<const u8*>(&val);
        data.insert(data.end(), bytes, bytes + sizeof(T));
    }

    void write_bytes(const u8* src, size_t size) {
        data.insert(data.end(), src, src + size);
    }
};

struct StreamReader {
    const std::vector<u8>& data;
    size_t offset{0};

    template <typename T>
    bool read(T& val) {
        if (offset + sizeof(T) > data.size()) return false;
        std::memcpy(&val, data.data() + offset, sizeof(T));
        offset += sizeof(T);
        return true;
    }

    bool read_bytes(u8* dst, size_t size) {
        if (offset + size > data.size()) return false;
        std::memcpy(dst, data.data() + offset, size);
        offset += size;
        return true;
    }
};

std::vector<u8> SaveState::serialize(const System& system) {
    StreamBuffer buf;

    // Header "GBSV"
    u32 magic = 0x56534247;
    u32 version = 1;
    buf.write(magic);
    buf.write(version);

    // Registers
    const auto& regs = system.get_registers();
    buf.write(regs.a); buf.write(regs.f);
    buf.write(regs.b); buf.write(regs.c);
    buf.write(regs.d); buf.write(regs.e);
    buf.write(regs.h); buf.write(regs.l);
    buf.write(regs.sp); buf.write(regs.pc);

    return buf.data;
}

bool SaveState::deserialize(System& system, const std::vector<u8>& data) {
    StreamReader reader{data};

    u32 magic = 0, version = 0;
    if (!reader.read(magic) || magic != 0x56534247) return false;
    if (!reader.read(version) || version != 1) return false;

    auto& regs = system.get_registers();
    if (!reader.read(regs.a)) return false;
    if (!reader.read(regs.f)) return false;
    if (!reader.read(regs.b)) return false;
    if (!reader.read(regs.c)) return false;
    if (!reader.read(regs.d)) return false;
    if (!reader.read(regs.e)) return false;
    if (!reader.read(regs.h)) return false;
    if (!reader.read(regs.l)) return false;
    if (!reader.read(regs.sp)) return false;
    if (!reader.read(regs.pc)) return false;

    return true;
}

bool SaveState::save_to_file(const System& system, const std::string& filepath) {
    auto data = serialize(system);
    std::ofstream file(filepath, std::ios::binary);
    if (!file.is_open()) return false;
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    return true;
}

bool SaveState::load_from_file(System& system, const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return false;
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<u8> data(size);
    if (!file.read(reinterpret_cast<char*>(data.data()), size)) return false;
    return deserialize(system, data);
}

} // namespace gb
