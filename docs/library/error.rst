.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

error
=====

.. _type-error:

:mini:`type error`
   An error. Values of this type are not accessible from Minilang code since they are caught by the runtime. Each error contains an *error value* which contains the details of the error.



.. _type-error-value:

:mini:`type error::value`
   An error value. Error values contain the details of an error but are not themselves errors (since errors are caught by the runtime).



:mini:`meth (Error: error::value):message: string`
   Returns the message of :mini:`Error`.



:mini:`meth (Error: error::value):raise: error`
   Returns :mini:`Error` as an error (i.e. rethrows the error).



:mini:`meth (Error: error::value):trace: list`
   Returns the stack trace of :mini:`Error` as a list of tuples.



:mini:`meth (Error: error::value):type: string`
   Returns the type of :mini:`Error`.



.. _fun-error:

:mini:`fun error(Type: string, Message: string): error`
   Creates an error exception with type :mini:`Type` and message :mini:`Message`. Since this creates an exception,  it will trigger the current exception handler.



.. _fun-raise:

:mini:`fun raise(Type: string, Value: any): error`
   Creates a general exception with type :mini:`Type` and value :mini:`Value`. Since this creates an exception,  it will trigger the current exception handler.



