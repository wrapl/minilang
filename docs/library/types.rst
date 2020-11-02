types
=====

.. include:: <isonum.txt>

:mini:`any`
   Base type for all values.

   *Defined at line 47 in src/ml_types.c*

:mini:`meth *(Arg₁: type, Arg₂: type)`
   *Defined at line 270 in src/ml_types.c*

:mini:`meth <(Arg₁: type, Arg₂: type)`
   *Defined at line 274 in src/ml_types.c*

:mini:`meth <=(Arg₁: type, Arg₂: type)`
   *Defined at line 283 in src/ml_types.c*

:mini:`meth >(Arg₁: type, Arg₂: type)`
   *Defined at line 292 in src/ml_types.c*

:mini:`meth >=(Arg₁: type, Arg₂: type)`
   *Defined at line 301 in src/ml_types.c*

:mini:`meth [](Arg₁: type)`
   *Defined at line 311 in src/ml_types.c*

:mini:`meth :in(Value: any, Type: type)` |rarr| :mini:`Value` or :mini:`nil`
   Returns :mini:`Value` if it is an instance of :mini:`Type` or a type that inherits from :mini:`Type`.

   Returns :mini:`nil` otherwise.

   *Defined at line 323 in src/ml_types.c*

:mini:`meth <>(Value₁: any, Value₂: any)` |rarr| :mini:`integer`
   Compares :mini:`Value₁` and :mini:`Value₂` and returns :mini:`-1`, :mini:`0` or :mini:`1`.

   This comparison is based on the internal addresses of :mini:`Value₁` and :mini:`Value₂` and thus only has no persistent meaning.

   *Defined at line 369 in src/ml_types.c*

:mini:`meth #(Value: any)` |rarr| :mini:`integer`
   Returns a hash for :mini:`Value` for use in lookup tables, etc.

   *Defined at line 380 in src/ml_types.c*

:mini:`meth =(Value₁: any, Value₂: any)` |rarr| :mini:`Value₂` or :mini:`nil`
   Returns :mini:`Value2` if :mini:`Value1` and :mini:`Value2` are exactly the same instance.

   Returns :mini:`nil` otherwise.

   *Defined at line 388 in src/ml_types.c*

:mini:`meth !=(Value₁: any, Value₂: any)` |rarr| :mini:`Value₂` or :mini:`nil`
   Returns :mini:`Value2` if :mini:`Value1` and :mini:`Value2` are not exactly the same instance.

   Returns :mini:`nil` otherwise.

   *Defined at line 397 in src/ml_types.c*

:mini:`meth string(Value: any)` |rarr| :mini:`string`
   Returns a general (type name only) representation of :mini:`Value` as a string.

   *Defined at line 406 in src/ml_types.c*

:mini:`fun mltuplegeneric(Arg₁: integer)`
   *Defined at line 827 in src/ml_types.c*

:mini:`meth string(Arg₁: nil)`
   *Defined at line 2094 in src/ml_types.c*

:mini:`meth string(Arg₁: some)`
   *Defined at line 2102 in src/ml_types.c*

:mini:`fun mllistgeneric(Arg₁: type)`
   *Defined at line 3466 in src/ml_types.c*

:mini:`meth list()`
   *Defined at line 3502 in src/ml_types.c*

:mini:`meth list(Arg₁: tuple)`
   *Defined at line 3506 in src/ml_types.c*

:mini:`fun mlmapgeneric(Arg₁: type, Arg₂: type)`
   *Defined at line 4131 in src/ml_types.c*

:mini:`meth map()`
   *Defined at line 4154 in src/ml_types.c*

