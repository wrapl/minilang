runtime
=======

.. include:: <isonum.txt>

:mini:`def state < function`

:mini:`def resumablestate < state`

:mini:`fun callcc()`

:mini:`fun markcc(Arg₁: any)`

:mini:`fun swapcc(Arg₁: state)`

:mini:`def reference`

:mini:`def uninitialized`

:mini:`meth ::(Arg₁: uninitialized, Arg₂: string)`

:mini:`def channel`

:mini:`meth :open(Arg₁: channel, Arg₂: any)`

:mini:`meth :ready(Arg₁: channel)`

:mini:`meth :send(Arg₁: channel, Arg₂: any)`

:mini:`meth :close(Arg₁: channel, Arg₂: any)`

:mini:`meth :error(Arg₁: channel, Arg₂: string, Arg₃: string)`

:mini:`meth :raise(Arg₁: channel, Arg₂: string, Arg₃: any)`

:mini:`meth :raise(Arg₁: channel, Arg₂: errorvalue)`

