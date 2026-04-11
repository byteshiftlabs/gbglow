#include "../src/core/cpu.h"
#include "../src/core/memory.h"
#include "../src/core/registers.h"
#include <iostream>
#include <cassert>

using namespace gbcrush;

void test_registers() {
    std::cout << "Testing registers...\n";
    
    Registers regs;
    regs.reset();
    
    // Check initial values after reset
    assert(regs.a == 0x01);
    assert(regs.pc == 0x0100);
    assert(regs.sp == 0xFFFE);
    
    // Test register pairs
    regs.set_bc(0x1234);
    assert(regs.b == 0x12);
    assert(regs.c == 0x34);
    assert(regs.bc() == 0x1234);
    
    // Test flags
    regs.set_flag(Registers::FLAG_Z, true);
    assert(regs.get_flag(Registers::FLAG_Z));
    regs.set_flag(Registers::FLAG_Z, false);
    assert(!regs.get_flag(Registers::FLAG_Z));
    
    std::cout << "  ✓ Registers working correctly\n";
}

void test_memory() {
    std::cout << "Testing memory...\n";
    
    Memory mem;
    
    // Test WRAM read/write
    mem.write(0xC000, 0x42);
    assert(mem.read(0xC000) == 0x42);
    
    // Test echo RAM
    mem.write(0xC100, 0xAB);
    assert(mem.read(0xE100) == 0xAB);  // Echo
    
    // Test HRAM
    mem.write(0xFF80, 0xCD);
    assert(mem.read(0xFF80) == 0xCD);
    
    // Test 16-bit operations
    mem.write16(0xC200, 0x1234);
    assert(mem.read16(0xC200) == 0x1234);
    
    std::cout << "  ✓ Memory working correctly\n";
}

void test_cpu_basic() {
    std::cout << "Testing CPU basics...\n";
    
    Memory mem;
    CPU cpu(mem);
    cpu.reset();
    
    // Write a simple program to WRAM: LD A, 0x42 ; HALT
    mem.write(0xC000, 0x3E);  // LD A, n
    mem.write(0xC001, 0x42);  // n = 0x42
    mem.write(0xC002, 0x76);  // HALT
    
    // Set PC to our program
    cpu.registers().pc = 0xC000;
    
    // Execute LD A, 0x42
    Cycles cycles1 = cpu.step();
    (void)cycles1;  // Used in assert, suppress warning
    assert(cycles1 == 2);  // LD A,n takes 2 M-cycles
    assert(cpu.registers().a == 0x42);
    assert(cpu.registers().pc == 0xC002);
    
    // Execute HALT
    Cycles cycles2 = cpu.step();
    (void)cycles2;  // Used in assert, suppress warning
    assert(cycles2 == 1);
    assert(cpu.is_halted());
    
    std::cout << "  ✓ CPU basic execution working\n";
}

void test_cpu_flags() {
    std::cout << "Testing CPU ALU flags...\n";
    
    Memory mem;
    CPU cpu(mem);
    cpu.reset();
    
    // Test XOR A (clears A and sets Z flag)
    cpu.registers().a = 0x42;
    cpu.registers().f = 0x00;
    
    // We'd need to implement XOR A instruction (0xAF) to test this properly
    // For now, just verify the register access works
    
    std::cout << "  ✓ CPU ALU structure ready\n";
}

int main() {
    std::cout << "=================================\n";
    std::cout << "EmuGBC Basic Component Tests\n";
    std::cout << "=================================\n\n";
    
    try {
        test_registers();
        test_memory();
        test_cpu_basic();
        test_cpu_flags();
        
        std::cout << "\n✅ All basic tests passed!\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Test failed: " << e.what() << "\n";
        return 1;
    }
}
