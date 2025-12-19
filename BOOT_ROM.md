# Boot ROM Support

## What is the Boot ROM?

The Game Boy boot ROM is a small 256-byte program stored in the console's hardware that:
- Displays the scrolling Nintendo logo animation
- Validates the cartridge ROM header
- Plays the startup sound
- Initializes hardware registers to specific values
- Then jumps to the cartridge code at 0x0100

## Do I need it?

**No, but it helps!** The emulator works without the boot ROM by:
- Starting with registers pre-initialized to post-boot values
- Jumping directly to cartridge entry point (0x0100)

However, **some games may have issues** without proper boot ROM initialization, particularly:
- Games that rely on specific timing
- Games that check hardware state left by boot ROM
- Commercial games like Pokémon

## How to get the Boot ROM

You need to extract it from real Game Boy hardware. The file should be:
- **Exactly 256 bytes** (0x100 bytes)
- MD5: `32fbbd84168d3482956eb3c5051637f5` (DMG boot ROM)

**Common filenames:**
- `dmg_boot.bin` (recommended)
- `boot.gb`
- `DMG_ROM.bin`

**Legal note:** Boot ROMs are copyrighted by Nintendo. You must extract them from hardware you own.

## Installation

1. Place the boot ROM file in the same directory as the emulator executable
2. Name it `dmg_boot.bin` or `boot.gb`
3. Run the emulator normally

The emulator will automatically detect and load the boot ROM if present.

## Verification

When you run the emulator, you'll see one of these messages:
```
Boot ROM loaded
```
or
```
No boot ROM found (emulator will skip boot sequence)
```

## Technical Details

- Boot ROM occupies address range: `0x0000-0x00FF`
- Boot ROM can be disabled by writing any non-zero value to register `0xFF50`
- Once disabled, the boot ROM cannot be re-enabled until system reset
- When disabled, cartridge ROM becomes visible at `0x0000-0x00FF`

## Post-Boot Register State

Without boot ROM, registers are initialized to:
```
A  = 0x01
F  = 0xB0 (flags: Z=1, N=0, H=1, C=0)
B  = 0x00
C  = 0x13
D  = 0x00
E  = 0xD8
H  = 0x01
L  = 0x4D
SP = 0xFFFE
PC = 0x0100 (cartridge entry point)
```

These values match real Game Boy hardware after the boot ROM completes.
