# GameBoy Emulator

A complete GameBoy emulator written in C++ with SDL2, OpenGL, and ImGui.
*(Note: This emulator is specifically designed and built for Windows)*

## Features
- Full CPU instruction set emulation
- PPU (Graphics) with flexible screen scaling and optional palette forcing
- APU (Audio) with audio synchronization and volume control
- Memory Bank Controller support (MBC1, MBC2, MBC3, MBC5)
- Powerful Debugger mode with Memory Editor (MMU) and CPU Registers tracking
- Save state / Battery save support
- ImGui user interface, allowing drag-and-drop of ROM files directly into the window to play immediately
- Slow-motion support

## Build & Run Instructions
This emulator is built specifically for **Windows 11**.

### Requirements
- **OS**: Windows 11
- **CMake**: Version 3.20 or higher
- **Compiler**: MSVC (Visual Studio 2019/2022) or any compatible C++20 compiler on Windows.

### Build from source
Open your terminal (Command Prompt / PowerShell) and run the following commands:
```bat
# 1. Generate the build files
cmake -B build

# 2. Compile the executable
cmake --build build
```
*(Note: By default on Visual Studio/MSVC, the executable will be located at `build\Debug\gb_emulator.exe`. If you want an optimized version, run `cmake --build build --config Release` and find it in `build\Release\gb_emulator.exe`.)*

### Running the Emulator
You can launch the emulator in two ways:
1. **Direct Launch**: Simply double-click `gb_emulator.exe`. Once the window opens, you can drag and drop any `.gb` ROM file into the emulator window to start playing.
2. **Command Line**: Run the executable and pass the ROM file path as an argument.
   ```bat
   .\build\Debug\gb_emulator.exe "path/to/your/game.gb"
   ```

## ROMs
You can download test ROMs and game ROMs to play from the links below:
- **Test ROMs:** [c-sp/game-boy-test-roms](https://github.com/c-sp/game-boy-test-roms)
- **Game ROMs:** [Google Drive Collection](https://drive.google.com/drive/folders/1JgnLjNZpI0l10GN4OGHr47ceT9yhh4UT)

## Tech Stack
- **C++20**
- **SDL2** (Windowing, Input, Audio)
- **OpenGL 3.0** (Rendering)
- **Dear ImGui** (UI & Layout)

## Contact Info
- **Created by**: Nguyen Huu Huy Thinh (shinnei1509)
- **Email**: dst15092004@gmail.com
- **Website**: [https://shinnei.github.io/](https://shinnei.github.io/)
- **GitHub**: [shInNei](https://github.com/shInNei)
