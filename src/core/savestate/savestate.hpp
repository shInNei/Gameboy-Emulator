#pragma once

#include "core/types.hpp"
#include <vector>
#include <string>
#include <iostream>
#include <fstream>

namespace gb {

class System; // Forward declaration

class SaveState {
public:
    static bool save_to_file(const System& system, const std::string& filepath);
    static bool load_from_file(System& system, const std::string& filepath);

    static std::vector<u8> serialize(const System& system);
    static bool deserialize(System& system, const std::vector<u8>& data);
};

} // namespace gb
