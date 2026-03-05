# Memory Map Documentation

## Game Boy Memory Map

| Address Range | Size  | Description                    | Notes                          |
|---------------|-------|--------------------------------|--------------------------------|
| 0x0000-0x00FF | 256B  | Boot ROM                       | Unmapped after boot            |
| 0x0000-0x3FFF | 16KB  | ROM Bank 0                     | Fixed, from cartridge          |
| 0x4000-0x7FFF | 16KB  | ROM Bank 1-N                   | Switchable via MBC             |
| 0x8000-0x9FFF | 8KB   | Video RAM (VRAM)               | Tiles and maps                 |
| 0xA000-0xBFFF | 8KB   | External RAM                   | Cartridge RAM (if present)     |
| 0xC000-0xCFFF | 4KB   | Work RAM Bank 0 (WRAM)         | Internal RAM                   |
| 0xD000-0xDFFF | 4KB   | Work RAM Bank 1                | Switchable in CGB mode         |
| 0xE000-0xFDFF | ~8KB  | Echo RAM                       | Mirror of 0xC000-0xDDFF        |
| 0xFE00-0xFE9F | 160B  | Object Attribute Memory (OAM)  | Sprite data (40 sprites)       |
| 0xFEA0-0xFEFF | 96B   | Unusable                       | Do not use                     |
| 0xFF00-0xFF7F | 128B  | I/O Registers                  | Hardware control               |
| 0xFF80-0xFFFE | 127B  | High RAM (HRAM)                | Fast internal RAM              |
| 0xFFFF        | 1B    | Interrupt Enable (IE)          | Interrupt control              |

## Important I/O Registers

### Joypad (0xFF00)
| Bit | Name | Description        |
|-----|------|--------------------|
| 5   | P15  | Select button keys |
| 4   | P14  | Select direction   |
| 3-0 | P13-P10 | Input lines     |

### Serial Transfer
- **SB (0xFF01)**: Serial transfer data
- **SC (0xFF02)**: Serial transfer control

### Timer
- **DIV (0xFF04)**: Divider register (incremented at 16384 Hz)
- **TIMA (0xFF05)**: Timer counter
- **TMA (0xFF06)**: Timer modulo
- **TAC (0xFF07)**: Timer control

### Interrupts
- **IF (0xFF0F)**: Interrupt flags
- **IE (0xFFFF)**: Interrupt enable

### LCD
- **LCDC (0xFF40)**: LCD control
- **STAT (0xFF41)**: LCD status
- **SCY (0xFF42)**: Scroll Y
- **SCX (0xFF43)**: Scroll X
- **LY (0xFF44)**: Current scanline
- **LYC (0xFF45)**: Scanline compare
- **WY (0xFF4A)**: Window Y
- **WX (0xFF4B)**: Window X

## References
- [Pan Docs - Memory Map](https://gbdev.io/pandocs/Memory_Map.html)
- [Pan Docs - I/O Registers](https://gbdev.io/pandocs/Hardware_Reg_List.html)
