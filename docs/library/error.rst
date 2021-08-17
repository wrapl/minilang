error
=====

:mini:`fun error(Type: string, Message: string): error`
   Creates an error exception with type :mini:`Type` and message :mini:`Message`. Since this creates an exception, it will trigger the current exception handler.


:mini:`fun raise(Type: string, Value: any): error`
   Creates an exception with type :mini:`Type` and value :mini:`Value`. Since this creates an exception, it will trigger the current exception handler.


:mini:`type error`
   *TBD*

:mini:`meth :type(Arg₁: errorvalue)`
   *TBD*

:mini:`meth :message(Arg₁: errorvalue)`
   *TBD*

:mini:`meth :trace(Arg₁: errorvalue)`
   *TBD*

