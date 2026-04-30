cmake_minimum_required(VERSION 3.10)
project(gbglow VERSION 0.1.0 LANGUAGES CXX)

# C++17 required for clean modern syntax
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler optimizations - O3 for maximum performance
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(MSVC)
        add_compile_options(/O2)
    else()
        # Core optimizations for maximum performance
        add_compile_options(
            -O3                      # Maximum optimization level
            -march=native            # Optimize for your specific CPU architecture
            -mtune=native            # Fine-tune for your CPU microarchitecture
            -ffast-math              # Fast floating-point math operations
            -funroll-loops           # Unroll small loops (excellent for emulation)
            -finline-functions       # Aggressive function inlining
            -fomit-frame-pointer     # Free up register for computations
        )
        # Strip debug symbols in release
        add_link_options(-s)
        # Enable Link Time Optimization
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()
elseif(NOT CMAKE_BUILD_TYPE)
    message(WARNING "No build type specified, defaulting to Release")
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler warnings
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# Source files
set(CORE_SOURCES
    src/core/cpu.cpp
    src/core/memory.cpp
    src/core/registers.cpp
    src/core/instructions.cpp
    src/core/emulator.cpp
    src/core/timer.cpp
    src/core/savestate.cpp
)

set(VIDEO_SOURCES
    src/video/ppu.cpp
    src/video/lcd.cpp
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

# ImGui sources
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdlrenderer2.cpp
)

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
    ${IMGUI_SOURCES}
)

target_include_directories(gbglow PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

# Find and link SDL2 for graphics output
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
target_include_directories(gbglow PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(gbglow ${SDL2_LIBRARIES})

# Testing support
enable_testing()
add_subdirectory(tests)
