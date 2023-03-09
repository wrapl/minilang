.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

runtime
=======

.. rst-class:: mini-api

.. _fun-break:

:mini:`fun break(Condition?: any)`
   If a debugger is present and :mini:`Condition` is omitted or not :mini:`nil` then triggers a breakpoint.


.. _fun-callcc:

:mini:`fun callcc()`
   *TBD*


.. _fun-calldc:

:mini:`fun calldc()`
   *TBD*


.. _fun-source:

:mini:`fun source(): tuple[string, integer]`
   Returns the caller source location. Evaluated at compile time if possible.


.. _fun-trace:

:mini:`fun trace(): list[tuple[string, integer]]`
   Returns the call stack trace (source locations).


.. _fun-atomic:

:mini:`fun atomic(Args: any, ..., Fn: function): any`
   Calls :mini:`Fn(Args)` in a new context without a scheduler and returns the result.


.. _fun-markcc:

:mini:`fun markcc(Arg₁: any)`
   *TBD*


:mini:`meth (Arg₁: channel):error(Arg₂: string, Arg₃: string)`
   *TBD*


:mini:`meth (Arg₁: channel):raise(Arg₂: error::value)`
   *TBD*


:mini:`meth (Arg₁: channel):raise(Arg₂: string, Arg₃: any)`
   *TBD*


.. _type-debugger:

:mini:`type debugger`
   *TBD*


:mini:`meth (Arg₁: error::value):value`
   *TBD*


:mini:`meth (Arg₁: string::buffer):append(Arg₂: error::value)`
   *TBD*


.. _type-resumable-state:

:mini:`type resumable::state < state`
   *TBD*


.. _type-state:

:mini:`type state < function`
   *TBD*


.. _fun-swapcc:

:mini:`fun swapcc(Arg₁: state)`
   *TBD*


.. _type-uninitialized:

:mini:`type uninitialized`
   An uninitialized value. Used for forward declarations.


:mini:`meth (Arg₁: uninitialized) :: (Arg₂: string)`
   *TBD*


