.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

bytecode
========

This is a mostly internal module,  subject to change.

.. _type-variable:

:mini:`type variable`
   A variable,  which can hold another value (returned when dereferenced) and assigned a new value.

   Variables may optionally be typed,  assigning a value that is not an instance of the specified type (or a subtype) will raise an error.


:mini:`meth variable(): variable`
   Return a new untyped variable with current value :mini:`nil`.


:mini:`meth variable(Value: any): variable`
   Return a new untyped variable with current value :mini:`Value`.


:mini:`meth variable(Value: any, Type: type): variable`
   Return a new typed variable with type :mini:`Type` and current value :mini:`Value`.


.. _fun-closure:

:mini:`fun closure(Original: closure): closure`
   Returns a copy of :mini:`Closure`.


.. _type-closure:

:mini:`type closure < function, sequence`
   A Minilang function.


.. _type-closure-info:

:mini:`type closure::info`
   Information about a closure.


:mini:`meth (Buffer: string::buffer):append(Closure: closure)`
   Appends a representation of :mini:`Closure` to :mini:`Buffer`.


:mini:`meth (Closure: closure):parameters: list`
   Returns the list of parameter names of :mini:`Closure`.


:mini:`meth (Closure: closure):info: map`
   Returns some information about :mini:`Closure`.


:mini:`meth (Closure: closure):list: string`
   Returns a listing of the bytecode of :mini:`Closure`.


