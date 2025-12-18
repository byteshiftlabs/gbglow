cmake_minimum_required(VERSION 3.15)
project(emugbc VERSION 0.1.0 LANGUAGES CXX)

# C++17 required for clean modern syntax
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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
)

set(CARTRIDGE_SOURCES
    src/cartridge/cartridge.cpp
    src/cartridge/mbc1.cpp
    src/cartridge/mbc3.cpp
)

set(INPUT_SOURCES
    src/input/joypad.cpp
)

# Main executable
add_executable(emugbc
    src/main.cpp
    ${CORE_SOURCES}
    ${VIDEO_SOURCES}
    ${CARTRIDGE_SOURCES}
    ${INPUT_SOURCES}
)

target_include_directories(emugbc PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Optional: Add SDL2 for graphics later
# find_package(SDL2 REQUIRED)
# target_link_libraries(emugbc SDL2::SDL2)

# Testing support
enable_testing()
add_subdirectory(tests)
