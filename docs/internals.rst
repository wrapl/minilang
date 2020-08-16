Internals
=========

.. include:: <isonum.txt>
.. include:: <isopub.txt>

Call States and Contexts
------------------------

All code in Minilang is executed within a *state*. Each state has the following properties:

:Type: Every state is also a valid Minilang value (so they can be passed as first class values).
:Caller: The state that called this state.
:Context: Every state has an associated context (which may be shared across states).
:run: The function to call in order to run this state.

See :doc:`/api/types` for precise C structure definition of states.

Closure Implementation
----------------------

Minilang code is run using a simple stack based virtual machine. There is a single register called **RESULT** which holds the most recent result.

.. list-table:: Frame Layout
   :widths: auto
   :header-rows: 1

   * - Field
     - Notes
   * - Common state fields
     - Every closure frame is a valid state
   * - Source name
     - For error reporting and debugging
   * - Next instruction
     - For resuming after a call returns
   * - Saved stack top
     - For resuming after a call returns
   * - Current error handler
     - 
   * - Upvalues
     - For nested closures
   * - Flags
     - 
   * - Schedule
     - For preemptive task switching
   * - Debug state
     - Only present if frame is being debugged
   * - Stack\ :subscript:`0`
     - Bottom of stack
   * - |vellip|
     - |larr| Top of stack points to somewhere in this
   * - Stack\ :subscript:`n`
     - Maximum stack

.. list-table:: Instruction Set
   :widths: auto
   :header-rows: 1

   * - Instruction
     - Parameters
     - Description
   * - RETURN
     - *none*
     - Switches to the calling state, passing the current value of **RESULT**.
   * - SUSPEND
     - 
     - 
   * - RESUME
     - 
     - 
   * - NIL
     - 
     - 
   * - SOME
     - 
     - 
   * - IF
     -
     - 
   * - ELSE
     -
     - 
   * - PUSH
     -
     - 
   * - WITH
     -
     - 
   * - WITH_VAR
     -
     - 
   * - WITHX
     -
     - 
   * - POP
     -
     - 
   * - ENTER
     -
     - 
   * - EXIT
     -
     - 
   * - LOOP
     -
     - 
   * - TRY
     -
     - 
   * - CATCH
     -
     - 
   * - LOAD
     -
     - 
   * - LOAD_PUSH
     -
     - 
   * - VAR
     -
     - 
   * - VARX
     -
     - 
   * - LET
     -
     - 
   * - LETI
     -
     - 
   * - LETX
     -
     - 
   * - FOR
     -
     - 
   * - NEXT
     -
     - 
   * - VALUE
     -
     - 
   * - KEY
     -
     - 
   * - CALL
     -
     - 
   * - CONST_CALL
     -
     - 
   * - ASSIGN
     -
     - 
   * - LOCAL
     -
     - 
   * - LOCAL_PUSH
     -
     - 
   * - UPVALUE
     -
     - 
   * - LOCALX
     -
     - 
   * - TUPLE_NEW
     -
     - 
   * - TUPLE_SET
     -
     - 
   * - LIST_NEW
     -
     - 
   * - LIST_APPEND
     -
     - 
   * - MAP_NEW
     -
     - 
   * - MAP_INSERT
     -
     - 
   * - CLOSURE
     -
     - 
   * - PARTIAL_NEW
     -
     - 
   * - PARTIAL_SET
     - 
     -

