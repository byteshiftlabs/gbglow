Interrupt System
================

The Game Boy has 5 hardware interrupts with a priority system.

Interrupt Sources
-----------------

.. list-table::
   :header-rows: 1
   :widths: 10 15 15 60

   * - Priority
     - Name
     - Vector
     - Description
   * - 1
     - VBlank
     - 0x0040
     - Triggered at start of VBlank period (~59.7 Hz)
   * - 2
     - LCD STAT
     - 0x0048
     - Various LCD controller conditions
   * - 3
     - Timer
     - 0x0050
     - Timer overflow (configurable frequency)
   * - 4
     - Serial
     - 0x0058
     - Serial transfer complete
   * - 5
     - Joypad
     - 0x0060
     - Button press (can wake from STOP)

Interrupt Registers
-------------------

IE - Interrupt Enable (0xFFFF)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Controls which interrupts are enabled:

.. code-block:: text

   Bit 4: Joypad
   Bit 3: Serial
   Bit 2: Timer
   Bit 1: LCD STAT
   Bit 0: VBlank

IF - Interrupt Flag (0xFF0F)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Indicates pending interrupts:

.. code-block:: text

   Bit 4: Joypad interrupt requested
   Bit 3: Serial interrupt requested
   Bit 2: Timer interrupt requested
   Bit 1: LCD STAT interrupt requested
   Bit 0: VBlank interrupt requested

IME - Interrupt Master Enable
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Global interrupt enable flag (CPU internal):

* Set by ``EI`` instruction
* Cleared by ``DI`` instruction
* Automatically cleared when interrupt is serviced
* Restored by ``RETI`` instruction

Interrupt Handling
------------------

Interrupt Service Routine
~~~~~~~~~~~~~~~~~~~~~~~~~~

When an interrupt is triggered:

1. Check if IME is enabled
2. Check if interrupt is enabled in IE
3. Check if interrupt is pending in IF
4. Select highest priority pending interrupt
5. Disable IME
6. Clear interrupt flag in IF
7. Push PC to stack
8. Jump to interrupt vector

Interrupt Sequence
~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   void CPU::handle_interrupts() {
       if (!ime_) return;
       
       u8 ie = memory_.read(0xFFFF);
       u8 if_reg = memory_.read(0xFF0F);
       u8 pending = ie & if_reg;
       
       if (pending == 0) return;
       
       // Check priority order
       if (pending & 0x01) {
           service_interrupt(0x01, 0x40);  // VBlank
       } else if (pending & 0x02) {
           service_interrupt(0x02, 0x48);  // LCD STAT
       } else if (pending & 0x04) {
           service_interrupt(0x04, 0x50);  // Timer
       } else if (pending & 0x08) {
           service_interrupt(0x08, 0x58);  // Serial
       } else if (pending & 0x10) {
           service_interrupt(0x10, 0x60);  // Joypad
       }
   }
   
   void CPU::service_interrupt(u8 bit, u16 vector) {
       halted_ = false;  // Wake from HALT
       ime_ = false;     // Disable interrupts
       
       // Clear interrupt flag
       u8 if_reg = memory_.read(0xFF0F);
       memory_.write(0xFF0F, if_reg & ~bit);
       
       // Push PC and jump
       push_stack(regs_.pc);
       regs_.pc = vector;
   }

VBlank Interrupt
----------------

Most commonly used interrupt, triggered once per frame.

**Trigger Condition**
   When PPU enters Mode 1 (VBlank) at scanline 144

**Typical Use**
   * Update game logic
   * Upload graphics data to VRAM
   * Synchronize frame timing

**Example Handler**

.. code-block:: asm

   ; VBlank handler at 0x0040
   VBlankHandler:
       push af
       push bc
       push de
       push hl
       
       ; Do VBlank work here
       call UpdateGameLogic
       call TransferTileData
       
       pop hl
       pop de
       pop bc
       pop af
       reti

LCD STAT Interrupt
-------------------

Flexible interrupt for various LCD conditions.

**Trigger Conditions** (STAT register bits 3-6)
   * Mode 0 (HBlank)
   * Mode 1 (VBlank)
   * Mode 2 (OAM)
   * LYC=LY coincidence

**Typical Use**
   * Scanline effects (raster effects)
   * Precise timing for graphics updates
   * Window/sprite manipulation mid-frame

Timer Interrupt
---------------

Programmable timer for game timing.

**Control Registers**
   * DIV (0xFF04): Divider register (16384 Hz, read-only)
   * TIMA (0xFF05): Timer counter (increments at TAC frequency)
   * TMA (0xFF06): Timer modulo (reload value)
   * TAC (0xFF07): Timer control

**TAC Register**

.. code-block:: text

   Bit 2: Timer Enable
   Bits 1-0: Clock Select
     00: 4096 Hz
     01: 262144 Hz
     10: 65536 Hz
     11: 16384 Hz

**Operation**
   * TIMA increments at selected frequency
   * When TIMA overflows (0xFF → 0x00), interrupt triggers
   * TIMA reloads from TMA

Serial Interrupt
----------------

Triggered when serial transfer completes.

**Control Registers**
   * SB (0xFF01): Serial data
   * SC (0xFF02): Serial control

**Typical Use**
   * Link cable communication
   * Two-player games
   * Data transfer between Game Boys

Joypad Interrupt
----------------

Triggered on button press (any button).

**Typical Use**
   * Wake from STOP mode
   * Low-power button detection

Implementation
--------------

Interrupt Priority
~~~~~~~~~~~~~~~~~~

Multiple pending interrupts are serviced in priority order:

.. code-block:: cpp

   u8 pending = ie & if_reg;
   
   // Check in priority order
   if (pending & 0x01) {
       // VBlank - highest priority
       service_vblank();
   } else if (pending & 0x02) {
       // LCD STAT
       service_lcd_stat();
   } else if (pending & 0x04) {
       // Timer
       service_timer();
   } else if (pending & 0x08) {
       // Serial
       service_serial();
   } else if (pending & 0x10) {
       // Joypad - lowest priority
       service_joypad();
   }

HALT and Interrupts
~~~~~~~~~~~~~~~~~~~

The ``HALT`` instruction suspends CPU until interrupt:

.. code-block:: cpp

   case 0x76:  // HALT
       halted_ = true;
       return 1;

Interrupt handling wakes CPU from HALT:

.. code-block:: cpp

   void CPU::handle_interrupts() {
       if (!ime_) return;
       
       if (has_pending_interrupt()) {
           halted_ = false;  // Wake up
           service_interrupt();
       }
   }

HALT Bug
~~~~~~~~

On DMG, there's a HALT bug when IE & IF are both non-zero but IME is 0:

* CPU does not halt
* Next byte is executed twice

This is not yet implemented in GBCrush.

Nested Interrupts
-----------------

By default, interrupts cannot nest (IME cleared when serviced).

To allow nesting, interrupt handler can re-enable IME:

.. code-block:: asm

   InterruptHandler:
       push af
       ei              ; Re-enable interrupts
       ; ... handler code ...
       di              ; Disable before return
       pop af
       reti

**Caution**: Must carefully manage stack and registers.

Testing
-------

Interrupt tests verify:

* Correct priority handling
* IE and IF register behavior
* IME flag state changes
* Interrupt timing accuracy
* HALT wake-up behavior

Common Patterns
---------------

VBlank Synchronization
~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: asm

   ; Wait for VBlank
   WaitVBlank:
       ld a, [rLY]     ; Read scanline
       cp 144          ; Check if in VBlank
       jr c, WaitVBlank
       ret

Frame-Rate Control
~~~~~~~~~~~~~~~~~~

.. code-block:: asm

   ; Set VBlank flag in handler
   VBlankISR:
       ld a, 1
       ld [vblank_flag], a
       reti
   
   ; Main loop waits for flag
   MainLoop:
       ; Wait for VBlank
   .wait:
       ld a, [vblank_flag]
       or a
       jr z, .wait
       
       xor a
       ld [vblank_flag], a
       
       ; Do frame work
       call UpdateGame
       jr MainLoop

Future Enhancements
-------------------

Not Yet Implemented
~~~~~~~~~~~~~~~~~~~

* LCD STAT interrupt conditions
* Timer overflow interrupt
* Serial transfer interrupt
* Joypad interrupt
* HALT bug emulation (DMG)
