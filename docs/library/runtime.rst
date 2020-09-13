runtime
=======

.. include:: <isonum.txt>

:mini:`contextkey`
   :Parents: :mini:`cfunction`

   *Defined at line 95 in src/ml_runtime.c*

:mini:`fun mlcontextkey()`
   *Defined at line 105 in src/ml_runtime.c*

:mini:`state`
   :Parents: :mini:`function`

   *Defined at line 113 in src/ml_runtime.c*

:mini:`resumablestate`
   :Parents: :mini:`state`

   *Defined at line 182 in src/ml_runtime.c*

:mini:`fun mlcallcc()`
   *Defined at line 190 in src/ml_runtime.c*

:mini:`fun mlmark(Arg₁: any)`
:mini:`reference`
   *Defined at line 251 in src/ml_runtime.c*

:mini:`uninitialized`
   *Defined at line 287 in src/ml_runtime.c*

:mini:`meth ::(Arg₁: uninitialized, Arg₂: string)`
   *Defined at line 321 in src/ml_runtime.c*

:mini:`fun error(Type: string, Message: string)` |rarr| :mini:`error`
   Creates an error exception with type :mini:`Type` and message :mini:`Message`. Since this creates an exception, it will trigger the current exception handler.

:mini:`fun raise(Type: string, Value: any)` |rarr| :mini:`error`
   Creates an exception with type :mini:`Type` and value :mini:`Value`. Since this creates an exception, it will trigger the current exception handler.

:mini:`errorvalue`
   *Defined at line 395 in src/ml_runtime.c*

