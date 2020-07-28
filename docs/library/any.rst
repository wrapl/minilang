any
===

.. include:: <isonum.txt>

**type** :mini:`any`
   Base type for all values.


**method** :mini:`? any Value` |rarr| :mini:`type`
   Returns the type of :mini:`Value`.


**method** :mini:`any Value:isa(type Type)` |rarr| :mini:`Value` or :mini:`nil`
   Returns :mini:`Value` if it is an instance of :mini:`Type` or a type that inherits from :mini:`Type`.

   Returns :mini:`nil` otherwise.


**method** :mini:`any Value₁ <> any Value₂` |rarr| :mini:`integer`
   Compares :mini:`Value₁` and :mini:`Value₂` and returns :mini:`-1`, :mini:`0` or :mini:`1`.

   This comparison is based on the internal addresses of :mini:`Value₁` and :mini:`Value₂` and thus only has no persistent meaning.


**method** :mini:`# any Value` |rarr| :mini:`integer`
   Returns a hash for :mini:`Value` for use in lookup tables, etc.


**method** :mini:`any Value₁ = any Value₂` |rarr| :mini:`Value₂` or :mini:`nil`
   Returns :mini:`Value2` if :mini:`Value1` and :mini:`Value2` are exactly the same instance.

   Returns :mini:`nil` otherwise.


**method** :mini:`any Value₁ != any Value₂` |rarr| :mini:`Value₂` or :mini:`nil`
   Returns :mini:`Value2` if :mini:`Value1` and :mini:`Value2` are not exactly the same instance.

   Returns :mini:`nil` otherwise.


**method** :mini:`string(any Value)` |rarr| :mini:`string`
   Returns a general (type name only) representation of :mini:`Value` as a string.


