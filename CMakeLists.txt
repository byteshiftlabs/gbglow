cmake_minimum_required(VERSION 3.15)
project(emugbc VERSION 0.1.0 LANGUAGES CXX)

# C++17 required for clean modern syntax
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler optimizations
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(MSVC)
        add_compile_options(/O2)
    else()
        add_compile_options(-O2)
        # Enable Link Time Optimization for better cross-module inlining
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()
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
)

set(AUDIO_SOURCES
    src/audio/apu.cpp
)

# Main executable
add_executable(emugbc
    src/main.cpp
    ${CORE_SOURCES}
    ${VIDEO_SOURCES}
    ${CARTRIDGE_SOURCES}
    ${INPUT_SOURCES}
    ${AUDIO_SOURCES}
)

target_include_directories(emugbc PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Find and link SDL2 for graphics output
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
target_include_directories(emugbc PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(emugbc ${SDL2_LIBRARIES})

# Testing support
enable_testing()
add_subdirectory(tests)
