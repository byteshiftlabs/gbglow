Timer
=====

Hardware timer system with configurable frequencies.

Provides the DIV (divider) register that increments at a fixed rate,
and the TIMA (timer counter) register with configurable frequency
that triggers interrupts on overflow.

**Header:** ``core/timer.h``

**Namespace:** ``emugbc``


Timer Registers
---------------

+----------+----------+------------------------------------------------+
| Register | Address  | Description                                    |
+==========+==========+================================================+
| DIV      | 0xFF04   | Divider - increments at 16,384 Hz              |
+----------+----------+------------------------------------------------+
| TIMA     | 0xFF05   | Timer counter - configurable frequency         |
+----------+----------+------------------------------------------------+
| TMA      | 0xFF06   | Timer modulo - reload value after overflow     |
+----------+----------+------------------------------------------------+
| TAC      | 0xFF07   | Timer control - enable and clock select        |
+----------+----------+------------------------------------------------+

TAC Register Bits
~~~~~~~~~~~~~~~~~

+-------+------------------+-------------------------------------------+
| Bits  | Name             | Description                               |
+=======+==================+===========================================+
| 7-3   | Unused           | Always 1                                  |
+-------+------------------+-------------------------------------------+
| 2     | Enable           | 1 = Timer enabled, 0 = disabled           |
+-------+------------------+-------------------------------------------+
| 1-0   | Clock Select     | Timer frequency selection                 |
+-------+------------------+-------------------------------------------+

Clock Select Values
~~~~~~~~~~~~~~~~~~~

+-------+------------+-------------------+
| Value | Frequency  | Cycles/Increment  |
+=======+============+===================+
| 00    | 4,096 Hz   | 1,024             |
+-------+------------+-------------------+
| 01    | 262,144 Hz | 16                |
+-------+------------+-------------------+
| 10    | 65,536 Hz  | 64                |
+-------+------------+-------------------+
| 11    | 16,384 Hz  | 256               |
+-------+------------+-------------------+


Constructor
-----------

.. cpp:function:: explicit Timer::Timer(Memory& memory)

   Creates a timer connected to the specified memory system.
   
   :param memory: Reference to Memory instance


Execution
---------

.. cpp:function:: void Timer::step(Cycles cycles)

   Advances timer state by the specified cycles.
   
   :param cycles: Number of M-cycles elapsed
   
   When TIMA overflows:
   
   1. Timer interrupt is requested (IF bit 2)
   2. TIMA is reloaded with TMA value


Register Read
-------------

.. cpp:function:: u8 Timer::read_div() const

   :return: DIV register value

.. cpp:function:: u8 Timer::read_tima() const

   :return: TIMA register value

.. cpp:function:: u8 Timer::read_tma() const

   :return: TMA register value

.. cpp:function:: u8 Timer::read_tac() const

   :return: TAC register value


Register Write
--------------

.. cpp:function:: void Timer::write_div(u8 value)

   Writing any value resets DIV to 0.
   
   :param value: Ignored (DIV always resets to 0)

.. cpp:function:: void Timer::write_tima(u8 value)

   Sets TIMA counter value.
   
   :param value: New counter value

.. cpp:function:: void Timer::write_tma(u8 value)

   Sets TMA modulo value.
   
   :param value: Reload value for TIMA after overflow

.. cpp:function:: void Timer::write_tac(u8 value)

   Sets TAC control register.
   
   :param value: Control value (bit 2 = enable, bits 1-0 = clock select)
