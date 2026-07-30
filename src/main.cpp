#include <SDL.h>
#include <SDL_opengl.h>

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>
#include "vendor/imgui_memory_editor.h"

#include "core/system.hpp"
#include "ui/renderer.hpp"
#include "ui/audio_player.hpp"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <string>
#include <algorithm>

namespace fs = std::filesystem;

inline fs::path utf8_to_path(const std::string& s) {
    return fs::path(reinterpret_cast<const char8_t*>(s.c_str()));
}
inline std::string path_to_utf8(const fs::path& p) {
    auto u8 = p.u8string();
    return std::string(reinterpret_cast<const char*>(u8.c_str()), u8.size());
}

// Default Key Mapping
struct KeyConfig {
    SDL_Keycode up{SDLK_UP};
    SDL_Keycode down{SDLK_DOWN};
    SDL_Keycode left{SDLK_LEFT};
    SDL_Keycode right{SDLK_RIGHT};
    SDL_Keycode a{SDLK_z};
    SDL_Keycode b{SDLK_x};
    SDL_Keycode select{SDLK_BACKSPACE};
    SDL_Keycode start{SDLK_RETURN};
};

static gb::Key keycode_to_gb_key(SDL_Keycode code, const KeyConfig& config) {
    if (code == config.right || code == SDLK_RIGHT || code == SDLK_d)  return gb::Key::Right;
    if (code == config.left  || code == SDLK_LEFT  || code == SDLK_a)  return gb::Key::Left;
    if (code == config.up    || code == SDLK_UP    || code == SDLK_w)  return gb::Key::Up;
    if (code == config.down  || code == SDLK_DOWN  || code == SDLK_s)  return gb::Key::Down;
    if (code == config.a     || code == SDLK_z     || code == SDLK_j)  return gb::Key::A;
    if (code == config.b     || code == SDLK_x     || code == SDLK_k)  return gb::Key::B;
    if (code == config.select|| code == SDLK_BACKSPACE || code == SDLK_RSHIFT) return gb::Key::Select;
    if (code == config.start || code == SDLK_RETURN|| code == SDLK_KP_ENTER || code == SDLK_SPACE) return gb::Key::Start;
    return static_cast<gb::Key>(255);
}

void save_config(const std::string& path, const KeyConfig& kc, float vol, bool mute, bool a_sync, bool v_sync, float speed, const gb::HardwareQuirks& quirks) {
    std::ofstream out(path);
    if (!out) return;
    out << "up=" << kc.up << "\n";
    out << "down=" << kc.down << "\n";
    out << "left=" << kc.left << "\n";
    out << "right=" << kc.right << "\n";
    out << "a=" << kc.a << "\n";
    out << "b=" << kc.b << "\n";
    out << "select=" << kc.select << "\n";
    out << "start=" << kc.start << "\n";
    out << "volume=" << vol << "\n";
    out << "mute=" << mute << "\n";
    out << "audio_sync=" << a_sync << "\n";
    out << "vsync=" << v_sync << "\n";
    out << "speed=" << speed << "\n";
    out << "model=" << static_cast<int>(quirks.model) << "\n";
    out << "halt_bug=" << quirks.halt_bug << "\n";
    out << "oam_bug=" << quirks.oam_corruption_bug << "\n";
    out << "stat_glitch=" << quirks.stat_interrupt_glitch << "\n";
    out << "vblank_bug=" << quirks.vblank_stat_bug << "\n";
    out << "tima_delay=" << quirks.tima_reload_delay << "\n";
}

void load_config(const std::string& path, KeyConfig& kc, float& vol, bool& mute, bool& a_sync, bool& v_sync, float& speed, gb::HardwareQuirks& quirks) {
    std::ifstream in(path);
    if (!in) return;
    std::string line;
    while (std::getline(in, line)) {
        size_t eq = line.find('=');
        if (eq != std::string::npos) {
            std::string k = line.substr(0, eq);
            std::string v = line.substr(eq + 1);
            try {
                if (k == "up") kc.up = std::stoi(v);
                else if (k == "down") kc.down = std::stoi(v);
                else if (k == "left") kc.left = std::stoi(v);
                else if (k == "right") kc.right = std::stoi(v);
                else if (k == "a") kc.a = std::stoi(v);
                else if (k == "b") kc.b = std::stoi(v);
                else if (k == "select") kc.select = std::stoi(v);
                else if (k == "start") kc.start = std::stoi(v);
                else if (k == "volume") vol = std::stof(v);
                else if (k == "mute") mute = std::stoi(v) != 0;
                else if (k == "audio_sync") a_sync = std::stoi(v) != 0;
                else if (k == "vsync") v_sync = std::stoi(v) != 0;
                else if (k == "speed") speed = std::stof(v);
                else if (k == "model") quirks.set_preset(static_cast<gb::HardwareModel>(std::stoi(v)));
                else if (k == "halt_bug") quirks.halt_bug = std::stoi(v) != 0;
                else if (k == "oam_bug") quirks.oam_corruption_bug = std::stoi(v) != 0;
                else if (k == "stat_glitch") quirks.stat_interrupt_glitch = std::stoi(v) != 0;
                else if (k == "vblank_bug") quirks.vblank_stat_bug = std::stoi(v) != 0;
                else if (k == "tima_delay") quirks.tima_reload_delay = std::stoi(v) != 0;
            } catch(...) {}
        }
    }
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    SDL_Window* window = SDL_CreateWindow(
        "GameBoy Emulator - created by shinnei1509",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1420, 800,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
    );

    if (!window) {
        std::cerr << "Failed to create SDL Window: " << SDL_GetError() << std::endl;
        return -1;
    }

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Disabled so arrows/space don't mess with UI during gameplay

    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Initialize Core System, Renderer, and Audio
    gb::System system;
    gb::OpenGLRenderer renderer;
    renderer.init();

    gb::AudioPlayer audio_player(system.get_apu());
    audio_player.init();

    // Serial Log buffer
    std::string serial_log_buffer;
    system.get_serial().set_output_callback([&serial_log_buffer](char c) {
        serial_log_buffer += c;
    });

    // Keybindings & Audio Settings
    KeyConfig key_config;
    int key_to_rebind = -1; // -1: none, 0:Up, 1:Down, 2:Left, 3:Right, 4:A, 5:B, 6:Select, 7:Start
    float master_volume = 1.0f;
    bool audio_mute = false;
    bool audio_sync = true;
    bool vsync_enabled = false;

    SDL_GL_SetSwapInterval(vsync_enabled ? 1 : 0);

    // ROM Browser State - Default to src/rom folder
    std::string current_rom_dir = "src/rom";
    if (!fs::exists(utf8_to_path(current_rom_dir))) {
        current_rom_dir = ".";
    }
    current_rom_dir = path_to_utf8(fs::absolute(utf8_to_path(current_rom_dir)));
    std::string selected_rom_info = "No ROM Loaded";

    // Debugger & Memory Editor
    static MemoryEditor mem_edit;
    bool show_memory_editor = false;
    bool debug_mode = true;
    float emulation_speed = 1.0f; // 1.0x

    // Load config from settings.ini
    load_config("settings.ini", key_config, master_volume, audio_mute, audio_sync, vsync_enabled, emulation_speed, system.get_quirks());

    // Attempt to load command line ROM if provided
    if (argc > 1) {
        if (system.load_rom(argv[1])) {
            selected_rom_info = fs::path(argv[1]).filename().string();
        }
    }

    bool running = true;
    Uint32 previous_time = SDL_GetTicks();
    while (running) {
        bool is_audio_syncing = (emulation_speed == 1.0f) && audio_sync;
        if (!is_audio_syncing && !vsync_enabled) {
            Uint32 current_time = SDL_GetTicks();
            Uint32 elapsed = current_time - previous_time;
            if (elapsed < 16) {
                SDL_Delay(16 - elapsed);
            }
        }
        previous_time = SDL_GetTicks();

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) {
                running = false;
            }
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window)) {
                running = false;
            }

            // Drag and Drop File (ROM)
            if (event.type == SDL_DROPFILE) {
                std::string dropped_file = event.drop.file;
                try {
                    fs::path src = utf8_to_path(dropped_file);
                    fs::path dest = utf8_to_path(current_rom_dir) / src.filename();
                    if (src != dest) {
                        fs::copy_file(src, dest, fs::copy_options::overwrite_existing);
                    }
                    if (system.load_rom(path_to_utf8(dest))) {
                        selected_rom_info = path_to_utf8(dest.filename());
                        serial_log_buffer.clear();
                    }
                } catch(const std::exception& e) {
                    std::cerr << "Error handling dropped file: " << e.what() << std::endl;
                }
                SDL_free(event.drop.file);
            }

            // Keyboard Rebinding
            if (key_to_rebind != -1 && event.type == SDL_KEYDOWN) {
                SDL_Keycode k = event.key.keysym.sym;
                switch (key_to_rebind) {
                    case 0: key_config.up = k; break;
                    case 1: key_config.down = k; break;
                    case 2: key_config.left = k; break;
                    case 3: key_config.right = k; break;
                    case 4: key_config.a = k; break;
                    case 5: key_config.b = k; break;
                    case 6: key_config.select = k; break;
                    case 7: key_config.start = k; break;
                }
                key_to_rebind = -1;
                continue;
            }

            // Joypad Inputs
            if (key_to_rebind == -1) {
                if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
                    gb::Key gb_k = keycode_to_gb_key(event.key.keysym.sym, key_config);
                    if (static_cast<gb::u8>(gb_k) != 255) {
                        if (event.type == SDL_KEYDOWN) {
                            system.get_joypad().key_down(gb_k);
                        } else {
                            system.get_joypad().key_up(gb_k);
                        }
                    }
                }
            }
        }

        // Emulation step
        static float frame_accumulator = 0.0f;
        if (!system.is_paused() && system.get_cartridge().is_loaded()) {
            system.get_apu().sync_to_audio = (emulation_speed == 1.0f) && audio_sync;
            
            frame_accumulator += emulation_speed;
            int frames_to_run = static_cast<int>(frame_accumulator);
            frame_accumulator -= frames_to_run;
            
            if (frames_to_run > 0) {
                for (int s = 0; s < frames_to_run; ++s) {
                    system.step_frame();
                }
                renderer.update_texture(system.get_framebuffer());
            }
            audio_player.set_volume(audio_mute ? 0.0f : master_volume);
        }

        // ImGui Frame Init
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // 1. Menu Bar
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Reset System")) system.reset();
                if (ImGui::MenuItem("Exit")) running = false;
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View")) {
                ImGui::MenuItem("Memory Viewer", nullptr, &show_memory_editor, debug_mode);
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        // 2. ROM Browser Window
        ImGui::SetNextWindowPos(ImVec2(10, 30), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(320, 400), ImGuiCond_FirstUseEver);
        ImGui::Begin("ROM Browser", nullptr, ImGuiWindowFlags_MenuBar);
        {
            ImGui::Text("Current Directory:");
            ImGui::SameLine();
            if (ImGui::SmallButton("Up (..)")) {
                fs::path p = utf8_to_path(current_rom_dir);
                if (p.has_parent_path()) {
                    current_rom_dir = path_to_utf8(p.parent_path());
                }
            }
            ImGui::TextWrapped("%s", current_rom_dir.c_str());
            ImGui::Separator();

            ImGui::BeginChild("ROMList", ImVec2(0, -55.0f), true);
            try {
                fs::path current_p = utf8_to_path(current_rom_dir);
                if (fs::exists(current_p) && fs::is_directory(current_p)) {
                    for (const auto& entry : fs::directory_iterator(current_p)) {
                        std::string filename = path_to_utf8(entry.path().filename());
                        if (entry.is_directory()) {
                            std::string label = "[DIR] " + filename;
                            if (ImGui::Selectable(label.c_str())) {
                                current_rom_dir = path_to_utf8(entry.path());
                            }
                        } else if (filename.ends_with(".gb")) {
                            std::string label = "[ROM] " + filename;
                            if (ImGui::Selectable(label.c_str())) {
                                std::string full_path = path_to_utf8(entry.path());
                                if (system.load_rom(full_path)) {
                                    selected_rom_info = filename;
                                    serial_log_buffer.clear();
                                }
                            }
                        } else if (filename == "manifest.txt") {
                            std::string label = "[DOC] " + filename;
                            if (ImGui::Selectable(label.c_str())) {
                                // Select manifest
                            }
                        }
                    }
                }
            } catch (const std::exception& e) {
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "Error reading directory: %s", e.what());
            }
            ImGui::EndChild();

            ImGui::Separator();
            ImGui::TextWrapped("Tip: You can drag and drop a .gb file directly into the emulator to copy and load it instantly.");
        }
        ImGui::End();

        // 3. GameBoy Display Window
        ImGui::SetNextWindowPos(ImVec2(340, 30), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(600, 550), ImGuiCond_FirstUseEver);
        ImGui::Begin("Display", nullptr, ImGuiWindowFlags_NoScrollbar);
        {
            ImGui::Text("Loaded: %s", selected_rom_info.c_str());
            
            if (debug_mode) {
                ImGui::SameLine();
                const auto& r = system.get_cpu().get_registers();
                gb::u8 current_bgp = system.get_ppu().read_register(0xFF47);
                ImGui::Text(" | PC: 0x%04X | BGP: 0x%02X | SCX: %d | SCY: %d", r.pc, current_bgp, system.get_ppu().read_register(0xFF43), system.get_ppu().read_register(0xFF42));
            }

            static bool force_palette = false;
            ImGui::SameLine();
            if (ImGui::Checkbox("Force Contrast Palette (0xFC)", &force_palette)) {
                if (force_palette) {
                    system.get_ppu().write_register(0xFF47, 0xFC);
                }
            }

            ImVec2 avail = ImGui::GetContentRegionAvail();
            if (avail.x > 0 && avail.y > 0) {
                float aspect = 160.0f / 144.0f;
                float w = avail.x;
                float h = w / aspect;
                if (h > avail.y) {
                    h = avail.y;
                    w = h * aspect;
                }
                ImVec2 pos = ImGui::GetCursorPos();
                ImGui::SetCursorPos(ImVec2(pos.x + (avail.x - w) * 0.5f, pos.y + (avail.y - h) * 0.5f));
                ImGui::Image((ImTextureID)(uintptr_t)renderer.get_texture_id(), ImVec2(w, h));
            }
        }
        ImGui::End();

        // 4. Settings Panel (Audio & Controller & Hardware Quirks)
        ImGui::SetNextWindowPos(ImVec2(950, 30), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(450, 750), ImGuiCond_FirstUseEver);
        ImGui::Begin("Settings & Controls");
        {
            if (ImGui::CollapsingHeader("Emulation Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
                if (system.is_paused()) {
                    if (ImGui::Button("Play")) system.set_paused(false);
                } else {
                    if (ImGui::Button("Pause")) system.set_paused(true);
                }
                ImGui::SameLine();
                if (ImGui::Button("Reset")) system.reset();
                ImGui::SameLine();
                if (ImGui::Button("Single Step")) system.step_instruction();
                ImGui::SameLine();
                ImGui::Checkbox("Debug Mode", &debug_mode);

                ImGui::SliderFloat("Speed", &emulation_speed, 0.1f, 5.0f, "%.1fx");
            }

            if (ImGui::CollapsingHeader("Audio & Video Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::SliderFloat("Master Volume", &master_volume, 0.0f, 1.0f, "%.2f");
                ImGui::Checkbox("Mute Audio", &audio_mute);
                ImGui::Checkbox("Sync to Audio", &audio_sync);
                if (ImGui::Checkbox("Enable VSync", &vsync_enabled)) {
                    SDL_GL_SetSwapInterval(vsync_enabled ? 1 : 0);
                }
            }

            if (ImGui::CollapsingHeader("Keyboard Controller Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
                auto rebind_button = [&](const char* label, SDL_Keycode key, int idx) {
                    std::string key_name = SDL_GetKeyName(key);
                    if (key_name == "Return") key_name = "Enter";
                    std::string btn_label = (key_to_rebind == idx) ? "Press any key..." : (label + std::string(": ") + key_name);
                    if (ImGui::Button(btn_label.c_str(), ImVec2(200, 0))) {
                        key_to_rebind = idx;
                    }
                };

                rebind_button("Up", key_config.up, 0); ImGui::SameLine();
                rebind_button("Down", key_config.down, 1);
                rebind_button("Left", key_config.left, 2); ImGui::SameLine();
                rebind_button("Right", key_config.right, 3);
                rebind_button("A Button", key_config.a, 4); ImGui::SameLine();
                rebind_button("B Button", key_config.b, 5);
                rebind_button("Select", key_config.select, 6); ImGui::SameLine();
                rebind_button("Start", key_config.start, 7);
            }

            if (ImGui::CollapsingHeader("Hardware Quirks & Preset", ImGuiTreeNodeFlags_DefaultOpen)) {
                auto& quirks = system.get_quirks();
                int current_model = static_cast<int>(quirks.model);
                const char* models[] = { "Auto", "DMG Rev B", "MGB", "CGB Rev C", "AGB" };
                if (ImGui::Combo("Hardware Model", &current_model, models, IM_ARRAYSIZE(models))) {
                    quirks.set_preset(static_cast<gb::HardwareModel>(current_model));
                }

                ImGui::Checkbox("HALT Bug", &quirks.halt_bug);
                ImGui::Checkbox("OAM Corruption Bug", &quirks.oam_corruption_bug);
                ImGui::Checkbox("STAT Interrupt Glitch", &quirks.stat_interrupt_glitch);
                ImGui::Checkbox("VBlank STAT Bug", &quirks.vblank_stat_bug);
                ImGui::Checkbox("TIMA Reload Delay", &quirks.tima_reload_delay);
            }
        }
        ImGui::End();

        // 5. Serial Log / Test Console
        if (debug_mode) {
            ImGui::SetNextWindowPos(ImVec2(340, 590), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(600, 190), ImGuiCond_FirstUseEver);
            ImGui::Begin("Serial Output Console");
            {
                if (ImGui::Button("Clear Log")) serial_log_buffer.clear();
                ImGui::SameLine();
                if (ImGui::Button("Copy to Clipboard")) ImGui::SetClipboardText(serial_log_buffer.c_str());
                ImGui::Separator();

                ImGui::BeginChild("SerialTextScroll", ImVec2(0, 0), true);
                ImGui::TextUnformatted(serial_log_buffer.c_str());
                if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
                    ImGui::SetScrollHereY(1.0f);
                }
                ImGui::EndChild();
            }
            ImGui::End();
        }

        // 6. CPU Debugger Panel
        if (debug_mode) {
            ImGui::SetNextWindowPos(ImVec2(10, 440), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(320, 340), ImGuiCond_FirstUseEver);
            ImGui::Begin("CPU Registers & Flags");
            {
                const auto& r = system.get_cpu().get_registers();
                ImGui::Text("A: 0x%02X   F: 0x%02X (AF: 0x%04X)", r.a, r.f, r.af());
                ImGui::Text("B: 0x%02X   C: 0x%02X (BC: 0x%04X)", r.b, r.c, r.bc());
                ImGui::Text("D: 0x%02X   E: 0x%02X (DE: 0x%04X)", r.d, r.e, r.de());
                ImGui::Text("H: 0x%02X   L: 0x%02X (HL: 0x%04X)", r.h, r.l, r.hl());
                ImGui::Text("SP: 0x%04X  PC: 0x%04X", r.sp, r.pc);
                ImGui::Separator();
                ImGui::Text("Flags: Z[%c] N[%c] H[%c] C[%c]",
                    r.get_z() ? '1' : '0',
                    r.get_n() ? '1' : '0',
                    r.get_h() ? '1' : '0',
                    r.get_c() ? '1' : '0');
                ImGui::Text("Total Cycles: %llu", static_cast<unsigned long long>(system.get_cpu().cycle_count()));
            }
            ImGui::End();
        }

        // 7. Memory Editor
        if (debug_mode && show_memory_editor) {
            mem_edit.ReadFn = [](const ImU8* mem, size_t off, void* user_data) -> ImU8 {
                auto* sys = static_cast<gb::System*>(user_data);
                return sys ? sys->get_mmu().read(static_cast<gb::u16>(off)) : 0;
            };
            mem_edit.UserData = &system;
            ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
            if (ImGui::Begin("Memory Editor (MMU)", &show_memory_editor, ImGuiWindowFlags_NoScrollbar)) {
                mem_edit.DrawContents(nullptr, 0x10000);
            }
            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    save_config("settings.ini", key_config, master_volume, audio_mute, audio_sync, vsync_enabled, emulation_speed, system.get_quirks());

    audio_player.shutdown();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
