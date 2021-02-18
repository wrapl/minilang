runtime
=======

.. include:: <isonum.txt>

:mini:`type state < function`

:mini:`type resumablestate < state`

:mini:`fun callcc()`

:mini:`fun markcc(Arg₁: any)`

:mini:`fun calldc()`

:mini:`fun swapcc(Arg₁: state)`

:mini:`type reference`

:mini:`type uninitialized`

:mini:`meth ::(Arg₁: uninitialized, Arg₂: string)`

:mini:`fun mlbreak(Condition?: any)`
   If a debugger present and :mini:`Condition` is omitted or not :mini:`nil` then triggers a breakpoint.


:mini:`type channel`

:mini:`meth :open(Arg₁: channel, Arg₂: any)`

:mini:`meth :ready(Arg₁: channel)`

:mini:`meth :next(Arg₁: channel)`

:mini:`meth :send(Arg₁: channel, Arg₂: any)`

:mini:`meth :close(Arg₁: channel, Arg₂: any)`

:mini:`meth :error(Arg₁: channel, Arg₂: string, Arg₃: string)`

:mini:`meth :raise(Arg₁: channel, Arg₂: string, Arg₃: any)`

:mini:`meth :raise(Arg₁: channel, Arg₂: errorvalue)`

