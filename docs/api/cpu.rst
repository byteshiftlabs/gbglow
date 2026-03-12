CPU Class
=========

The Sharp LR35902 CPU emulation — executes instructions and handles interrupts.

File: ``src/core/cpu.h``

Class Declaration
-----------------

.. code-block:: cpp

   class CPU {
   public:
       explicit CPU(Memory& memory);

       Cycles step();
       void handle_interrupts();
       void reset();

       const Registers& registers() const;
       Registers& registers();

       bool ime() const;
       void set_ime(bool value);
       bool is_halted() const;
       void set_halted(bool value);
       void request_interrupt(u8 interrupt_bit);

       void serialize(std::vector<u8>& data) const;
       void deserialize(const u8* data, size_t data_size, size_t& offset);
   };

Execution
---------

step()
~~~~~~

.. code-block:: cpp

   Cycles step();

Fetches, decodes, and executes the next instruction at ``PC``.
Returns the number of M-cycles consumed. Handles the HALT bug and
EI delayed-enable semantics internally.

handle_interrupts()
~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   void handle_interrupts();

Checks ``IF`` and ``IE`` registers. If an enabled interrupt is pending
and ``IME`` is set, pushes ``PC`` onto the stack and jumps to the
corresponding interrupt vector. Clears the serviced bit in ``IF`` and
disables ``IME``.

Register Access
---------------

.. code-block:: cpp

   const Registers& registers() const;
   Registers& registers();

Returns a reference to the CPU register file.

Serialization
-------------

.. code-block:: cpp

   void serialize(std::vector<u8>& data) const;
   void deserialize(const u8* data, size_t data_size, size_t& offset);

Writes/reads all CPU state (registers, IME, halt flag) to/from a
byte vector for save-state support.
