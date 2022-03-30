.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

runtime
=======

.. _fun-break:

:mini:`fun break(Condition?: any)`
   If a debugger is present and :mini:`Condition` is omitted or not :mini:`nil` then triggers a breakpoint.


.. _fun-callcc:

:mini:`fun callcc()`
   *TBD*


.. _fun-calldc:

:mini:`fun calldc()`
   *TBD*


.. _fun-markcc:

:mini:`fun markcc(Arg₁: any)`
   *TBD*


.. _fun-mldebugger:

:mini:`fun mldebugger(Arg₁: any)`
   *TBD*


:mini:`meth (Arg₁: channel):error(Arg₂: string, Arg₃: string)`
   *TBD*


:mini:`meth (Arg₁: channel):raise(Arg₂: error::value)`
   *TBD*


:mini:`meth (Arg₁: channel):raise(Arg₂: string, Arg₃: any)`
   *TBD*


:mini:`meth (Arg₁: string::buffer):append(Arg₂: error::value)`
   *TBD*


.. _type-mini-debugger:

:mini:`type mini::debugger`
   *TBD*


:mini:`meth (Arg₁: mini::debugger):breakpoint_clear(Arg₂: string, Arg₃: integer)`
   *TBD*


:mini:`meth (Arg₁: mini::debugger):breakpoint_set(Arg₂: string, Arg₃: integer)`
   *TBD*


:mini:`meth (Arg₁: mini::debugger):continue(Arg₂: state, Arg₃: any)`
   *TBD*


:mini:`meth (Arg₁: mini::debugger):error_mode(Arg₂: any)`
   *TBD*


:mini:`meth (Arg₁: mini::debugger):step_in(Arg₂: state, Arg₃: any)`
   *TBD*


:mini:`meth (Arg₁: mini::debugger):step_mode(Arg₂: any)`
   *TBD*


:mini:`meth (Arg₁: mini::debugger):step_out(Arg₂: state, Arg₃: any)`
   *TBD*


:mini:`meth (Arg₁: mini::debugger):step_over(Arg₂: state, Arg₃: any)`
   *TBD*


.. _type-reference:

:mini:`type reference`
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


:mini:`meth (Arg₁: state):locals`
   *TBD*


:mini:`meth (Arg₁: state):source`
   *TBD*


:mini:`meth (Arg₁: state):trace`
   *TBD*


.. _type-uninitialized:

:mini:`type uninitialized`
   *TBD*


:mini:`meth (Arg₁: uninitialized) :: (Arg₂: string)`
   *TBD*


