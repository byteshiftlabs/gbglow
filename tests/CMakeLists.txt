# Basic component tests
add_executable(test_basic 
    test_basic.cpp
    ../src/core/constants.cpp
    ../src/core/emulator.cpp
    ../src/core/cpu.cpp
    ../src/core/instructions.cpp
    ../src/core/instructions_cb.cpp
    ../src/core/memory.cpp
    ../src/core/registers.cpp
    ../src/core/savestate_io.cpp
    ../src/core/timer.cpp
    ../src/cartridge/cartridge.cpp
    ../src/cartridge/mbc1.cpp
    ../src/cartridge/mbc3.cpp
    ../src/cartridge/mbc5.cpp
    ../src/debug/debugger.cpp
    ../src/input/joypad.cpp
    ../src/audio/apu.cpp
    ../src/video/ppu.cpp
    ../src/ui/recent_roms.cpp
)

target_include_directories(test_basic PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_test(NAME basic_tests COMMAND test_basic)
