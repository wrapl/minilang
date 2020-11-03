types
=====

.. include:: <isonum.txt>

:mini:`any`
   Base type for all values.

   *Defined at line 47 in src/ml_types.c*

:mini:`meth *(Arg₁: type, Arg₂: type)`
   *Defined at line 283 in src/ml_types.c*

:mini:`meth <(Arg₁: type, Arg₂: type)`
   *Defined at line 287 in src/ml_types.c*

:mini:`meth <=(Arg₁: type, Arg₂: type)`
   *Defined at line 296 in src/ml_types.c*

:mini:`meth >(Arg₁: type, Arg₂: type)`
   *Defined at line 305 in src/ml_types.c*

:mini:`meth >=(Arg₁: type, Arg₂: type)`
   *Defined at line 314 in src/ml_types.c*

:mini:`meth [](Arg₁: type)`
   *Defined at line 324 in src/ml_types.c*

:mini:`meth :in(Value: any, Type: type)` |rarr| :mini:`Value` or :mini:`nil`
   Returns :mini:`Value` if it is an instance of :mini:`Type` or a type that inherits from :mini:`Type`.

   Returns :mini:`nil` otherwise.

   *Defined at line 336 in src/ml_types.c*

:mini:`meth <>(Value₁: any, Value₂: any)` |rarr| :mini:`integer`
   Compares :mini:`Value₁` and :mini:`Value₂` and returns :mini:`-1`, :mini:`0` or :mini:`1`.

   This comparison is based on the internal addresses of :mini:`Value₁` and :mini:`Value₂` and thus only has no persistent meaning.

   *Defined at line 382 in src/ml_types.c*

:mini:`meth #(Value: any)` |rarr| :mini:`integer`
   Returns a hash for :mini:`Value` for use in lookup tables, etc.

   *Defined at line 393 in src/ml_types.c*

:mini:`meth =(Value₁: any, Value₂: any)` |rarr| :mini:`Value₂` or :mini:`nil`
   Returns :mini:`Value2` if :mini:`Value1` and :mini:`Value2` are exactly the same instance.

   Returns :mini:`nil` otherwise.

   *Defined at line 401 in src/ml_types.c*

:mini:`meth !=(Value₁: any, Value₂: any)` |rarr| :mini:`Value₂` or :mini:`nil`
   Returns :mini:`Value2` if :mini:`Value1` and :mini:`Value2` are not exactly the same instance.

   Returns :mini:`nil` otherwise.

   *Defined at line 410 in src/ml_types.c*

:mini:`meth string(Value: any)` |rarr| :mini:`string`
   Returns a general (type name only) representation of :mini:`Value` as a string.

   *Defined at line 419 in src/ml_types.c*

:mini:`fun mltuplegeneric(Arg₁: integer)`
   *Defined at line 840 in src/ml_types.c*

:mini:`meth string(Arg₁: nil)`
   *Defined at line 2107 in src/ml_types.c*

:mini:`meth string(Arg₁: some)`
   *Defined at line 2115 in src/ml_types.c*

:mini:`fun mllistgeneric(Arg₁: type)`
   *Defined at line 3480 in src/ml_types.c*

:mini:`meth list()`
   *Defined at line 3516 in src/ml_types.c*

:mini:`meth list(Arg₁: tuple)`
   *Defined at line 3520 in src/ml_types.c*

:mini:`fun mlmapgeneric(Arg₁: type, Arg₂: type)`
   *Defined at line 4146 in src/ml_types.c*

:mini:`meth map()`
   *Defined at line 4169 in src/ml_types.c*

