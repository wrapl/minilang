types
=====

.. include:: <isonum.txt>

:mini:`any`
   Base type for all values.

   *Defined at line 52 in src/ml_types.c*

:mini:`typerule`
   *Defined at line 319 in src/ml_types.c*

:mini:`meth *(Arg₁: type, Arg₂: type)`
   *Defined at line 404 in src/ml_types.c*

:mini:`meth <(Arg₁: type, Arg₂: type)`
   *Defined at line 408 in src/ml_types.c*

:mini:`meth <=(Arg₁: type, Arg₂: type)`
   *Defined at line 416 in src/ml_types.c*

:mini:`meth >(Arg₁: type, Arg₂: type)`
   *Defined at line 424 in src/ml_types.c*

:mini:`meth >=(Arg₁: type, Arg₂: type)`
   *Defined at line 432 in src/ml_types.c*

:mini:`meth [](Arg₁: type)`
   *Defined at line 441 in src/ml_types.c*

:mini:`meth :in(Value: any, Type: type)` |rarr| :mini:`Value` or :mini:`nil`
   Returns :mini:`Value` if it is an instance of :mini:`Type` or a type that inherits from :mini:`Type`.

   Returns :mini:`nil` otherwise.

   *Defined at line 453 in src/ml_types.c*

:mini:`meth <>(Value₁: any, Value₂: any)` |rarr| :mini:`integer`
   Compares :mini:`Value₁` and :mini:`Value₂` and returns :mini:`-1`, :mini:`0` or :mini:`1`.

   This comparison is based on the internal addresses of :mini:`Value₁` and :mini:`Value₂` and thus only has no persistent meaning.

   *Defined at line 499 in src/ml_types.c*

:mini:`meth #(Value: any)` |rarr| :mini:`integer`
   Returns a hash for :mini:`Value` for use in lookup tables, etc.

   *Defined at line 510 in src/ml_types.c*

:mini:`meth =(Value₁: any, Value₂: any)` |rarr| :mini:`Value₂` or :mini:`nil`
   Returns :mini:`Value2` if :mini:`Value1` and :mini:`Value2` are exactly the same instance.

   Returns :mini:`nil` otherwise.

   *Defined at line 518 in src/ml_types.c*

:mini:`meth !=(Value₁: any, Value₂: any)` |rarr| :mini:`Value₂` or :mini:`nil`
   Returns :mini:`Value2` if :mini:`Value1` and :mini:`Value2` are not exactly the same instance.

   Returns :mini:`nil` otherwise.

   *Defined at line 527 in src/ml_types.c*

:mini:`meth string(Value: any)` |rarr| :mini:`string`
   Returns a general (type name only) representation of :mini:`Value` as a string.

   *Defined at line 536 in src/ml_types.c*

:mini:`meth string(Arg₁: nil)`
   *Defined at line 2200 in src/ml_types.c*

:mini:`meth string(Arg₁: some)`
   *Defined at line 2208 in src/ml_types.c*

:mini:`meth list()`
   *Defined at line 3565 in src/ml_types.c*

:mini:`meth list(Arg₁: tuple)`
   *Defined at line 3569 in src/ml_types.c*

:mini:`meth map()`
   *Defined at line 4164 in src/ml_types.c*

