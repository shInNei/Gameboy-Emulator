cmake_minimum_required(VERSION 3.20)
project(GameBoyEmulator C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# 1. Fetch SDL2 if not installed
find_package(SDL2 QUIET)
if (NOT SDL2_FOUND)
    message(STATUS "SDL2 not found locally. Fetching SDL2 via FetchContent...")
    FetchContent_Declare(
        SDL2
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-2.28.5
    )
    FetchContent_MakeAvailable(SDL2)
    set(SDL2_LIBRARIES SDL2::SDL2main SDL2::SDL2-static)
    set(SDL2_INCLUDE_DIRS ${sdl2_SOURCE_DIR}/include)
else()
    include_directories(${SDL2_INCLUDE_DIRS})
endif()

# Core sources
file(GLOB_RECURSE CORE_SOURCES
    "src/core/*.cpp"
    "src/debug/*.cpp"
    "src/utils/*.cpp"
)

# ImGui sources
set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/vendor/imgui")
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

include_directories(
    src
    src/vendor
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

# Core Library
add_library(gb_core STATIC ${CORE_SOURCES})

# Headless Test Runner Executable
add_executable(gb_test_runner src/test_runner.cpp)
target_link_libraries(gb_test_runner PRIVATE gb_core)

add_executable(test_quiz_key src/test_quiz_key.cpp)
target_link_libraries(test_quiz_key PRIVATE gb_core)

add_executable(test_doraemon src/test_doraemon.cpp)
target_link_libraries(test_doraemon PRIVATE gb_core)

# Main GUI Application Executable
add_executable(gb_emulator
    src/main.cpp
    src/ui/renderer.cpp
    src/ui/audio_player.cpp
    ${IMGUI_SOURCES}
)

if (TARGET SDL2::SDL2main)
    target_link_libraries(gb_emulator PRIVATE gb_core SDL2::SDL2main SDL2::SDL2-static opengl32)
else()
    target_link_libraries(gb_emulator PRIVATE gb_core ${SDL2_LIBRARIES} opengl32)
endif()
