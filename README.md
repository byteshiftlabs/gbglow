# GBCrush - A Game Boy Color Emulator

**GBCrush** is a Game Boy Color emulator written in C++17, focusing on code clarity, comprehensive documentation, and educational value.

## Philosophy

GBCrush emphasizes:

- **Clear code**: Easy to understand and learn from
- **Comprehensive documentation**: Every component fully documented
- **Educational value**: Reference implementation for learning GB emulation

## Current Status

✅ **Phase 1 Complete: CPU Core**
- ~180 standard opcodes implemented
- All 256 CB prefix instructions
- Complete flag handling

✅ **Phase 2 Complete: Basic PPU**
- Background tile rendering
- LCD timing (4 modes)
- VBlank interrupt
- 160x144 framebuffer

✅ **Phase 3 Complete: Audio**
- 4 sound channels (2 square, 1 wave, 1 noise)
- APU implementation
- Real-time SDL2 audio output
- Sweep, envelope, and length support

✅ **Phase 4 Complete: Graphics**
- Sprite rendering (8x8 and 8x16 modes)
- OAM DMA transfer for fast sprite updates
- Window layer with position control
- Sprite priority and transparency
- 10 sprites per scanline limit

⏳ **Phase 5: Optional Enhancements**
- Game Boy Color palette support
- Additional polish and optimization

## Building

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

## Running

```bash
./gbcrush <rom_file.gb> [num_frames]
```

## Documentation

Complete Sphinx documentation available in `docs/`:

- **Architecture**: Detailed component documentation (CPU, Memory, PPU, etc.)
- **API Reference**: Complete API with code examples
- **Implementation Notes**: Design decisions and rationale
- **Development Guide**: Building, testing, contributing

To build docs:
```bash
cd docs
pip install -r requirements.txt
make html
```

View at `docs/_build/html/index.html`

## Testing

```bash
cd build
./test_basic
```

## Features

- Sharp LR35902 CPU emulation (~180 opcodes)
- Complete memory system (64KB address space)
- Background and window tile rendering
- **Sprite rendering (8x8 and 8x16 modes)**
- **OAM DMA transfer for sprite updates**
- **Full audio emulation (4 channels)**
- MBC1, MBC3, and MBC5 cartridge support
- Battery-backed save file support
- Complete interrupt handling (VBlank, Timer, Joypad, etc.)
- Cycle-accurate timing
- SDL2 video and audio output
- Joypad input with keyboard mapping

## License

GPL 3.0 - see LICENSE file

## References

- [Pan Docs](https://gbdev.io/pandocs/) - Complete GB hardware reference
- [Game Boy CPU Manual](http://marc.rawer.de/Gameboy/Docs/GBCPUman.pdf)
- [GB Opcodes](https://gbdev.io/gb-opcodes/)
