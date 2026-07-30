# GameBoy Emulator

A complete GameBoy emulator written in C++ with SDL2, OpenGL, and ImGui.

## Các chức năng
- Full CPU instruction set emulation (Mô phỏng đầy đủ tập lệnh CPU)
- PPU (Graphics) hỗ trợ scale màn hình linh hoạt và palette màu tùy chọn
- APU (Audio) có đồng bộ hóa âm thanh và tùy chỉnh âm lượng
- Hỗ trợ các thẻ băng MBC1, MBC2, MBC3, MBC5
- Chế độ Debugger mạnh mẽ với Memory Editor (MMU) và theo dõi trạng thái CPU Registers
- Hỗ trợ Save state / Battery save
- Giao diện người dùng ImGui, cho phép kéo thả trực tiếp file ROM vào cửa sổ để chơi ngay
- Hỗ trợ chế độ chạy chậm (Slow-motion)

## Link tải file test (Github test)
Nếu bạn muốn tìm các file ROM test để kiểm tra độ chính xác của emulator, bạn có thể tải tại đây:
- [Blargg's GameBoy Hardware Tests](https://github.com/retrio/gb-test-roms)
- [Mooneye GB Test Suite](https://github.com/Gekkio/mooneye-test-suite)

## Tech Stack
- **C++20**
- **SDL2** (Windowing, Input, Audio)
- **OpenGL 3.0** (Rendering)
- **Dear ImGui** (UI & Layout)

## Contact Info
- **Created by**: shinnei1509
- **GitHub**: [shInNei](https://github.com/shInNei)
