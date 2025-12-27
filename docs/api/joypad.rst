Joypad
======

Joypad input handler for button and D-pad input.

Manages the P1/JOYP register (0xFF00) which provides button state
to the Game Boy. Buttons are active-low (0 = pressed).

**Header:** ``input/joypad.h``

**Namespace:** ``emugbc``


P1/JOYP Register (0xFF00)
-------------------------

+------+---------------------+----------------------------------------+
| Bit  | Name                | Description                            |
+======+=====================+========================================+
| 7-6  | Unused              | Always 1                               |
+------+---------------------+----------------------------------------+
| 5    | Select Buttons      | 0 = Select action buttons              |
+------+---------------------+----------------------------------------+
| 4    | Select D-Pad        | 0 = Select direction buttons           |
+------+---------------------+----------------------------------------+
| 3    | Down / Start        | 0 = Pressed                            |
+------+---------------------+----------------------------------------+
| 2    | Up / Select         | 0 = Pressed                            |
+------+---------------------+----------------------------------------+
| 1    | Left / B            | 0 = Pressed                            |
+------+---------------------+----------------------------------------+
| 0    | Right / A           | 0 = Pressed                            |
+------+---------------------+----------------------------------------+

The lower 4 bits return different values depending on bits 4-5:

- Bit 5 = 0: Returns A, B, Select, Start
- Bit 4 = 0: Returns Right, Left, Up, Down


Constructor
-----------

.. cpp:function:: explicit Joypad::Joypad(Memory& memory)

   Creates a joypad handler connected to the specified memory system.
   
   :param memory: Reference to Memory instance


Action Buttons
--------------

.. cpp:function:: void Joypad::press_a()

   Presses the A button.

.. cpp:function:: void Joypad::release_a()

   Releases the A button.

.. cpp:function:: void Joypad::press_b()

   Presses the B button.

.. cpp:function:: void Joypad::release_b()

   Releases the B button.

.. cpp:function:: void Joypad::press_start()

   Presses the Start button.

.. cpp:function:: void Joypad::release_start()

   Releases the Start button.

.. cpp:function:: void Joypad::press_select()

   Presses the Select button.

.. cpp:function:: void Joypad::release_select()

   Releases the Select button.


Direction Buttons (D-Pad)
-------------------------

.. cpp:function:: void Joypad::press_up()

   Presses Up on the D-pad.

.. cpp:function:: void Joypad::release_up()

   Releases Up on the D-pad.

.. cpp:function:: void Joypad::press_down()

   Presses Down on the D-pad.

.. cpp:function:: void Joypad::release_down()

   Releases Down on the D-pad.

.. cpp:function:: void Joypad::press_left()

   Presses Left on the D-pad.

.. cpp:function:: void Joypad::release_left()

   Releases Left on the D-pad.

.. cpp:function:: void Joypad::press_right()

   Presses Right on the D-pad.

.. cpp:function:: void Joypad::release_right()

   Releases Right on the D-pad.


Register Access
---------------

.. cpp:function:: u8 Joypad::read_register() const

   Reads the P1/JOYP register.
   
   :return: Register value with button states
   
   Returns button states based on current selection bits (4-5).

.. cpp:function:: void Joypad::write_register(u8 value)

   Writes to P1/JOYP register.
   
   :param value: Value to write (only bits 4-5 are writable)
   
   Sets which button group to read (action buttons vs D-pad).


Interrupt Behavior
------------------

When any button transitions from released to pressed while that button
group is selected, the joypad interrupt (IF bit 4) is requested.
