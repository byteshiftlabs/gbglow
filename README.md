# EmuGBC - A Crystal-Clear Game Boy Color Emulator

**EmuGBC** is a Game Boy Color emulator written in C++17, focusing on code clarity, comprehensive documentation, and educational value.

## Philosophy

Unlike existing emulators (gnuboy, etc.) that prioritize performance or features, EmuGBC emphasizes:

- **Crystal-clear code**: Easy to understand and learn from
- **Comprehensive documentation**: Every component fully documented
- **Educational value**: Reference implementation for learning GB emulation
- **Cycle accuracy**: Matches hardware timing behavior

## Current Status

✅ **Phase 1 Complete: CPU Core**
- ~180 standard opcodes implemented
- All 256 CB prefix instructions
- Complete flag handling
- Cycle-accurate timing

✅ **Phase 2 Complete: Basic PPU**
- Background tile rendering
- LCD timing (4 modes)
- VBlank interrupt
- 160x144 framebuffer

⏳ **Phase 3: In Progress**
- Sprite rendering (OAM, priority)
- Window layer
- Additional interrupts

## Building

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

## Running

```bash
./emugbc <rom_file.gb> [num_frames]
```

## Documentation

Complete Sphinx documentation available in `docs/sphinx/`:

- **Architecture**: Detailed component documentation (CPU, Memory, PPU, etc.)
- **API Reference**: Complete API with code examples
- **Implementation Notes**: Design decisions and rationale
- **Development Guide**: Building, testing, contributing

To build docs:
```bash
cd docs/sphinx
pip install -r requirements.txt
make html
```

View at `docs/sphinx/_build/html/index.html`

## Testing

```bash
cd build
./test_basic
```

## Features

- Sharp LR35902 CPU emulation (~180 opcodes)
- Complete memory system (64KB address space)
- Background tile rendering
- ROM-only and MBC1 cartridge support
- VBlank interrupt handling
- Cycle-accurate timing

## License

GPL 3.0 - see LICENSE file

## References

- [Pan Docs](https://gbdev.io/pandocs/) - Complete GB hardware reference
- [Game Boy CPU Manual](http://marc.rawer.de/Gameboy/Docs/GBCPUman.pdf)
- [GB Opcodes](https://gbdev.io/gb-opcodes/)
