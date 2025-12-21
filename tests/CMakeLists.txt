# Basic component tests
add_executable(test_basic 
    test_basic.cpp
    ../src/core/cpu.cpp
    ../src/core/instructions.cpp
    ../src/core/memory.cpp
    ../src/core/registers.cpp
    ../src/core/timer.cpp
    ../src/cartridge/cartridge.cpp
    ../src/cartridge/mbc1.cpp
    ../src/cartridge/mbc3.cpp
    ../src/cartridge/mbc5.cpp
    ../src/input/joypad.cpp
    ../src/audio/apu.cpp
)

target_include_directories(test_basic PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_executable(sound_capture
    sound_capture.cpp
    ../src/core/emulator.cpp
    ../src/core/cpu.cpp
    ../src/core/instructions.cpp
    ../src/core/memory.cpp
    ../src/core/registers.cpp
    ../src/core/timer.cpp
    ../src/cartridge/cartridge.cpp
    ../src/cartridge/mbc1.cpp
    ../src/cartridge/mbc3.cpp
    ../src/cartridge/mbc5.cpp
    ../src/input/joypad.cpp
    ../src/audio/apu.cpp
    ../src/video/ppu.cpp
    ../src/video/display.cpp
)

target_include_directories(sound_capture PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

find_package(SDL2 REQUIRED)
target_link_libraries(sound_capture PRIVATE SDL2::SDL2)

add_test(NAME basic_tests COMMAND test_basic)
