error
=====

:mini:`fun error(Type: string, Message: string): error`
   Creates an error exception with type :mini:`Type` and message :mini:`Message`. Since this creates an exception, it will trigger the current exception handler.


:mini:`fun raise(Type: string, Value: any): error`
   Creates a general exception with type :mini:`Type` and value :mini:`Value`. Since this creates an exception, it will trigger the current exception handler.


:mini:`type error`
   An error. Values of this type are not accessible from Minilang code since they are caught by the runtime. Each error contains an *error value* which contains the details of the error.


:mini:`type error::value`
   An error value. Error values contain the details of an error but are not themselves errors (since errors are caught by the runtime).


:mini:`meth :type(Error: error::value): string`
   Returns the type of :mini:`Error`.


:mini:`meth :message(Error: error::value): string`
   Returns the message of :mini:`Error`.


:mini:`meth :trace(Error: error::value): list`
   Returns the stack trace of :mini:`Error` as a list of tuples.


:mini:`meth :raise(Error: error::value): error`
   Returns :mini:`Error` as an error (i.e. rethrows the error).


