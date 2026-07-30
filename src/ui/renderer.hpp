#pragma once

#include "core/types.hpp"

namespace gb {

class OpenGLRenderer {
public:
    OpenGLRenderer() = default;
    ~OpenGLRenderer();

    bool init();
    void update_texture(const Framebuffer& framebuffer);
    void render();

    [[nodiscard]] uint32_t get_texture_id() const { return texture_id; }

private:
    uint32_t texture_id{0};
};

} // namespace gb
