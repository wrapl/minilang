.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

socket
======

.. rst-class:: mini-api

:mini:`type socket < (MLStreamFd`
   *TBD*


:mini:`meth (Arg₁: socket):listen(Arg₂: integer)`
   *TBD*


:mini:`type socket::inet < (MLSocket`
   *TBD*


:mini:`meth (Arg₁: socket::inet):accept`
   *TBD*


:mini:`meth (Arg₁: socket::inet):bind(Arg₂: integer)`
   *TBD*


:mini:`meth (Arg₁: socket::inet):bind(Arg₂: string, Arg₃: integer)`
   *TBD*


:mini:`meth (Arg₁: socket::inet):connect(Arg₂: string, Arg₃: integer)`
   *TBD*


:mini:`type socket::local < (MLSocket`
   *TBD*


:mini:`meth (Arg₁: socket::local):accept`
   *TBD*


:mini:`meth (Arg₁: socket::local):bind(Arg₂: string)`
   *TBD*


:mini:`meth (Arg₁: socket::local):connect(Arg₂: string)`
   *TBD*


:mini:`type socket::type < enum`
   * :mini:`::Stream`
   * :mini:`::DGram`
   * :mini:`::Raw`


:mini:`fun mlsocketinet(Arg₁: socket::type)`
   *TBD*


:mini:`fun mlsocketlocal(Arg₁: socket::type)`
   *TBD*


