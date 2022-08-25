.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

enum
====

.. _type-enum:

:mini:`type enum < type, sequence`
   The base type of enumeration types.


:mini:`meth enum(Values: string, ..., Arg₂: string): enum`
   Returns a new enumeration type.

   .. code-block:: mini

      let day := enum("Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun")
      :> <<day>>
      day::Wed :> Wed
      day::Fri + 0 :> 5


:mini:`meth enum(Name₁ is Value₁, ...): enum`
   Returns a new enumeration type.

   .. code-block:: mini

      let colour := enum(Red is 10, Green is 20, Blue is 30)
      :> <<colour>>
      colour::Red :> Red
      list(colour, _ + 0) :> [10, 20, 30]


:mini:`meth (Enum: enum):count: integer`
   Returns the size of the enumeration :mini:`Enum`.

   .. code-block:: mini

      let day := enum("Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun")
      :> <<day>>
      day:count :> 7


.. _type-enum-range:

:mini:`type enum::range < sequence`
   A range of enum values.


.. _type-enum-value:

:mini:`type enum::value < integer`
   An instance of an enumeration type.


:mini:`meth (Min: enum::value) .. (Max: enum::value): enum::range`
   Returns a range of enum values. :mini:`Min` and :mini:`Max` must belong to the same enumeration.

   .. code-block:: mini

      let day := enum("Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun")
      :> <<day>>
      day::Mon .. day::Fri :> <enum-range[day]>


:mini:`meth (Arg₁: string::buffer):append(Arg₂: enum::value)`
   *TBD*

