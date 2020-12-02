types
=====

.. include:: <isonum.txt>

:mini:`def any`
   Base type for all values.


:mini:`meth :append(Arg₁: stringbuffer, Arg₂: type)`

:mini:`def typerule`

:mini:`meth *(Arg₁: type, Arg₂: type)`

:mini:`meth <(Arg₁: type, Arg₂: type)`

:mini:`meth <=(Arg₁: type, Arg₂: type)`

:mini:`meth >(Arg₁: type, Arg₂: type)`

:mini:`meth >=(Arg₁: type, Arg₂: type)`

:mini:`meth (Arg₁: type)[]`

:mini:`meth :in(Value: any, Type: type)` |rarr| :mini:`Value` or :mini:`nil`
   Returns :mini:`Value` if it is an instance of :mini:`Type` or a type that inherits from :mini:`Type`.

   Returns :mini:`nil` otherwise.


:mini:`meth <>(Value₁: any, Value₂: any)` |rarr| :mini:`integer`
   Compares :mini:`Value₁` and :mini:`Value₂` and returns :mini:`-1`, :mini:`0` or :mini:`1`.

   This comparison is based on the internal addresses of :mini:`Value₁` and :mini:`Value₂` and thus only has no persistent meaning.


:mini:`meth #(Value: any)` |rarr| :mini:`integer`
   Returns a hash for :mini:`Value` for use in lookup tables, etc.


:mini:`meth =(Value₁: any, Value₂: any)` |rarr| :mini:`Value₂` or :mini:`nil`
   Returns :mini:`Value2` if :mini:`Value1` and :mini:`Value2` are exactly the same instance.

   Returns :mini:`nil` otherwise.


:mini:`meth !=(Value₁: any, Value₂: any)` |rarr| :mini:`Value₂` or :mini:`nil`
   Returns :mini:`Value2` if :mini:`Value1` and :mini:`Value2` are not exactly the same instance.

   Returns :mini:`nil` otherwise.


:mini:`meth string(Value: any)` |rarr| :mini:`string`
   Returns a general (type name only) representation of :mini:`Value` as a string.


:mini:`meth :append(Arg₁: stringbuffer, Arg₂: tuple)`

