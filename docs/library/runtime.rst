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


.. _fun-mlatomic:

:mini:`fun mlatomic(Arg₁: any)`
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


