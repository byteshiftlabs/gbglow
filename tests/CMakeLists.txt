set(GBGLOW_TEST_RUNTIME_SOURCES
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
    ../src/debug/debugger_gui.cpp
    ../src/input/gamepad.cpp
    ../src/input/joypad.cpp
    ../src/audio/apu.cpp
    ../src/video/ppu.cpp
    ../src/ui/recent_roms.cpp
)

function(add_gbglow_test target test_source)
    add_executable(${target}
        ${test_source}
        ${GBGLOW_TEST_RUNTIME_SOURCES}
    )

    target_include_directories(${target} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
    )

    target_link_libraries(${target} PRIVATE imgui_backend ${SDL2_LIBRARIES})

    add_test(NAME ${target} COMMAND ${target})
endfunction()

add_gbglow_test(test_core test_core.cpp)
add_gbglow_test(test_persistence test_persistence.cpp)
add_gbglow_test(test_ppu test_ppu.cpp)
