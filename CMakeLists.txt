cmake_minimum_required(VERSION 3.14)
project(gbglow VERSION 0.1.0 LANGUAGES CXX)

include(FetchContent)

# C++17 required for clean modern syntax
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler optimizations for the current supported target: Ubuntu 24.04 with GCC/Clang.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    # Core optimizations for the supported Ubuntu 24.04 toolchain.
    add_compile_options(
        -O3
        -march=native
        -mtune=native
        -ffast-math
        -funroll-loops
        -finline-functions
        -fomit-frame-pointer
    )
    add_link_options(-s)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

# Compiler warnings for the current supported GCC/Clang toolchain.
add_compile_options(-Wall -Wextra -Wpedantic -Werror)

# Source files
set(CORE_SOURCES
    src/core/constants.cpp
    src/core/cpu.cpp
    src/core/memory.cpp
    src/core/registers.cpp
    src/core/instructions.cpp
    src/core/instructions_cb.cpp
    src/core/emulator.cpp
    src/core/game_loop.cpp
    src/core/savestate_io.cpp
    src/core/timer.cpp
)

set(VIDEO_SOURCES
    src/video/ppu.cpp
    src/video/display.cpp
)

set(CARTRIDGE_SOURCES
    src/cartridge/cartridge.cpp
    src/cartridge/mbc1.cpp
    src/cartridge/mbc3.cpp
    src/cartridge/mbc5.cpp
)

set(INPUT_SOURCES
    src/input/joypad.cpp
    src/input/gamepad.cpp
)

set(AUDIO_SOURCES
    src/audio/apu.cpp
)

set(DEBUG_SOURCES
    src/debug/debugger.cpp
    src/debug/debugger_gui.cpp
)

set(UI_SOURCES
    src/ui/recent_roms.cpp
    src/ui/screenshot.cpp
)

# Find and link SDL2 for graphics output
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

# Dear ImGui dependency
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.8
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(imgui)

set(IMGUI_DIR ${imgui_SOURCE_DIR})
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdlrenderer2.cpp
)

add_library(imgui_backend STATIC ${IMGUI_SOURCES})
target_include_directories(imgui_backend PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${SDL2_INCLUDE_DIRS}
)
target_link_libraries(imgui_backend PUBLIC ${SDL2_LIBRARIES})

# Main executable
add_executable(gbglow
    src/main.cpp
    ${CORE_SOURCES}
    ${VIDEO_SOURCES}
    ${CARTRIDGE_SOURCES}
    ${INPUT_SOURCES}
    ${AUDIO_SOURCES}
    ${DEBUG_SOURCES}
    ${UI_SOURCES}
)

target_include_directories(gbglow PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(gbglow PRIVATE imgui_backend ${SDL2_LIBRARIES})

# Testing support
enable_testing()
add_subdirectory(tests)
