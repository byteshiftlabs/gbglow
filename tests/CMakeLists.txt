# Basic component tests
add_executable(test_basic 
    test_basic.cpp
    ../src/core/cpu.cpp
    ../src/core/instructions.cpp
    ../src/core/memory.cpp
    ../src/core/registers.cpp
    ../src/cartridge/cartridge.cpp
    ../src/cartridge/mbc1.cpp
)

target_include_directories(test_basic PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_test(NAME basic_tests COMMAND test_basic)
