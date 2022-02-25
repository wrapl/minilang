.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

object
======

.. _type-object:

:mini:`type object`
   Parent type of all object classes.


:mini:`meth (Object: object) :: (Field: string): field`
   Retrieves the field :mini:`Field` from :mini:`Object`. Mainly intended for unpacking objects.


.. _type-class:

:mini:`type class < type`
   Type of all object classes.


:mini:`meth (Arg₁: string::buffer):append(Arg₂: object)`
   *TBD*

.. _fun-class:

:mini:`fun class(Parents: class, ..., Fields: method, ..., Exports: names, ...): class`
   Returns a new class inheriting from :mini:`Parents`,  with fields :mini:`Fields` and exports :mini:`Exports`. The special exports :mini:`::of` and :mini:`::init` can be set to override the default conversion and initialization behaviour. The :mini:`::new` export will *always* be set to the original constructor for this class.


.. _type-property:

:mini:`type property`
   A value with an associated setter function.


.. _fun-property:

:mini:`fun property(Value: any, set: any): property`
   Returns a new property which dereferences to :mini:`Value`. Assigning to the property will call :mini:`set(NewValue)`.


:mini:`meth (Arg₁: string::buffer):append(Arg₂: enum::value)`
   *TBD*

.. _fun-enum:

:mini:`fun enum(Values: string, ...): enum`
   *TBD*

.. _type-enum:

:mini:`type enum < type, sequence`
   *TBD*

:mini:`meth (Enum: enum):count: integer`
   *TBD*

.. _type-enum-range:

:mini:`type enum::range`
   *TBD*

.. _type-enum-range:

:mini:`type enum::range < sequence`
   *TBD*

:mini:`meth (Arg₁: enum::value) .. (Arg₂: enum::value)`
   *TBD*

.. _type-flags:

:mini:`type flags < type`
   *TBD*

:mini:`meth (Arg₁: string::buffer):append(Arg₂: flags::value)`
   *TBD*

:mini:`meth flags(Name₁: string, ...): flags`
   *TBD*

:mini:`meth flags(Name₁ is  Value₁, ...): flags`
   *TBD*

:mini:`meth (Arg₁: flags::value) + (Arg₂: flags::value)`
   *TBD*

:mini:`meth (Arg₁: flags::value) - (Arg₂: flags::value)`
   *TBD*

:mini:`meth (Arg₁: flags::value) < (Arg₂: flags::value)`
   *TBD*

:mini:`meth (Arg₁: flags::value) <= (Arg₂: flags::value)`
   *TBD*

:mini:`meth (Arg₁: flags::value) > (Arg₂: flags::value)`
   *TBD*

:mini:`meth (Arg₁: flags::value) >= (Arg₂: flags::value)`
   *TBD*

:mini:`meth list(Arg₁: flags::value)`
   *TBD*

