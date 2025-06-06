.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

runtime
=======

.. rst-class:: mini-api

:mini:`fun backtrace()`
   *TBD*


:mini:`fun break(Condition?: any)`
   If a debugger is present and :mini:`Condition` is omitted or not :mini:`nil` then triggers a breakpoint.


:mini:`fun callcc()`
   *TBD*


:mini:`fun calldc()`
   *TBD*


:mini:`fun trace(): list[tuple[string, integer]]`
   Returns the call stack trace (source locations).


:mini:`fun atomic(Args: any, ..., Fn: function): any`
   Calls :mini:`Fn(Args)` in a new context without a scheduler and returns the result.


:mini:`fun finalize(Value: any, Fn: function)`
   Registers :mini:`Fn` as the finalizer for :mini:`Value`.


:mini:`fun markcc(Arg₁: any)`
   *TBD*


:mini:`meth (Arg₁: channel):error(Arg₂: string, Arg₃: string)`
   *TBD*


:mini:`meth (Arg₁: channel):raise(Arg₂: error::value)`
   *TBD*


:mini:`meth (Arg₁: channel):raise(Arg₂: string, Arg₃: any)`
   *TBD*


:mini:`type debugger`
   *TBD*


:mini:`meth (Arg₁: error::value):value`
   *TBD*


:mini:`meth (Arg₁: string::buffer):append(Arg₂: error::value)`
   *TBD*


:mini:`fun mlsleep(Arg₁: real)`
   *TBD*


:mini:`type resumable::state < state`
   *TBD*


:mini:`type state < function`
   *TBD*


:mini:`fun swapcc(Arg₁: state)`
   *TBD*


:mini:`type uninitialized`
   An uninitialized value. Used for forward declarations.


:mini:`meth (Arg₁: uninitialized) :: (Arg₂: string)`
   *TBD*


