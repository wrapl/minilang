error
=====

.. include:: <isonum.txt>

:mini:`fun error(Type: string, Message: string)` |rarr| :mini:`error`
   Creates an error exception with type :mini:`Type` and message :mini:`Message`. Since this creates an exception, it will trigger the current exception handler.


:mini:`fun raise(Type: string, Value: any)` |rarr| :mini:`error`
   Creates an exception with type :mini:`Type` and value :mini:`Value`. Since this creates an exception, it will trigger the current exception handler.


:mini:`def error`

:mini:`meth :type(Arg₁: errorvalue)`

:mini:`meth :message(Arg₁: errorvalue)`

:mini:`meth :trace(Arg₁: errorvalue)`

