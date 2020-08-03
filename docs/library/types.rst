types
=====

.. include:: <isonum.txt>

**type** :mini:`any`
   Base type for all values.

   *Defined at line 40 in src/ml_types.c*

**method** :mini:`? any Value` |rarr| :mini:`type`
   Returns the type of :mini:`Value`.

   *Defined at line 220 in src/ml_types.c*

**method** :mini:`any Value:isa(type Type)` |rarr| :mini:`Value` or :mini:`nil`
   Returns :mini:`Value` if it is an instance of :mini:`Type` or a type that inherits from :mini:`Type`.

   Returns :mini:`nil` otherwise.

   *Defined at line 227 in src/ml_types.c*

**method** :mini:`any Value₁ <> any Value₂` |rarr| :mini:`integer`
   Compares :mini:`Value₁` and :mini:`Value₂` and returns :mini:`-1`, :mini:`0` or :mini:`1`.

   This comparison is based on the internal addresses of :mini:`Value₁` and :mini:`Value₂` and thus only has no persistent meaning.

   *Defined at line 273 in src/ml_types.c*

**method** :mini:`# any Value` |rarr| :mini:`integer`
   Returns a hash for :mini:`Value` for use in lookup tables, etc.

   *Defined at line 284 in src/ml_types.c*

**method** :mini:`any Value₁ = any Value₂` |rarr| :mini:`Value₂` or :mini:`nil`
   Returns :mini:`Value2` if :mini:`Value1` and :mini:`Value2` are exactly the same instance.

   Returns :mini:`nil` otherwise.

   *Defined at line 292 in src/ml_types.c*

**method** :mini:`any Value₁ != any Value₂` |rarr| :mini:`Value₂` or :mini:`nil`
   Returns :mini:`Value2` if :mini:`Value1` and :mini:`Value2` are not exactly the same instance.

   Returns :mini:`nil` otherwise.

   *Defined at line 301 in src/ml_types.c*

**method** :mini:`string(any Value)` |rarr| :mini:`string`
   Returns a general (type name only) representation of :mini:`Value` as a string.

   *Defined at line 310 in src/ml_types.c*

**method** :mini:`string(nil Arg₁)`
   *Defined at line 1878 in src/ml_types.c*

**method** :mini:`string(some Arg₁)`
   *Defined at line 1886 in src/ml_types.c*

