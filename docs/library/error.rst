error
=====

.. include:: <isonum.txt>

:mini:`fun error(Type: string, Message: string)` |rarr| :mini:`error`
   Creates an error exception with type :mini:`Type` and message :mini:`Message`. Since this creates an exception, it will trigger the current exception handler.

   *Defined at line 391 in src/ml_runtime.c*

:mini:`fun raise(Type: string, Value: any)` |rarr| :mini:`error`
   Creates an exception with type :mini:`Type` and value :mini:`Value`. Since this creates an exception, it will trigger the current exception handler.

   *Defined at line 410 in src/ml_runtime.c*

:mini:`error`
   *Defined at line 428 in src/ml_runtime.c*

:mini:`meth :type(Arg₁: errorvalue)`
   *Defined at line 504 in src/ml_runtime.c*

:mini:`meth :message(Arg₁: errorvalue)`
   *Defined at line 509 in src/ml_runtime.c*

:mini:`meth :trace(Arg₁: errorvalue)`
   *Defined at line 514 in src/ml_runtime.c*

