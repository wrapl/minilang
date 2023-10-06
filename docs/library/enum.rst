.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

enum
====

.. rst-class:: mini-api

:mini:`type enum < type, sequence`
   The base type of enumeration types.


:mini:`meth enum(Name₁ is Value₁, ...): enum`
   Returns a new enumeration type.

   .. code-block:: mini

      let colour := enum(Red is 10, Green is 20, Blue is 30)
      :> <<colour>>
      colour::Red :> Red
      list(colour, _ + 0) :> [Red, Green, Blue]


:mini:`meth enum(Names: string, ...): enum`
   Returns a new enumeration type.

   .. code-block:: mini

      let day := enum("Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun")
      :> <<day>>
      day::Wed :> Wed
      day::Fri + 0 :> Fri


:mini:`meth (Enum: enum):count: integer`
   Returns the size of the enumeration :mini:`Enum`.

   .. code-block:: mini

      let day := enum("Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun")
      :> <<day>>
      day:count :> 7


:mini:`meth (Enum: enum):random: enum::value`
   *TBD*


:mini:`type enum::cyclic < enum`
   *TBD*


:mini:`type enum::range < sequence`
   A range of enum values.


:mini:`type enum::value < integer`
   An instance of an enumeration type.


:mini:`meth (Arg₁: enum::value) + (Arg₂: integer)`
   *TBD*


:mini:`meth (Arg₁: enum::value) - (Arg₂: integer)`
   *TBD*


:mini:`meth (Min: enum::value) .. (Max: enum::value): enum::range`
   Returns a range of enum values. :mini:`Min` and :mini:`Max` must belong to the same enumeration.

   .. code-block:: mini

      let day := enum("Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun")
      :> <<day>>
      day::Mon .. day::Fri :> <enum-range[day]>


:mini:`meth (Arg₁: enum::value) <> (Arg₂: integer)`
   *TBD*


:mini:`meth integer(Arg₁: enum::value)`
   *TBD*


:mini:`meth (Arg₁: enum::value):next`
   *TBD*


:mini:`meth (Arg₁: enum::value):prev`
   *TBD*


:mini:`meth (Arg₁: string::buffer):append(Arg₂: enum::value)`
   *TBD*


:mini:`meth (Arg₁: integer) + (Arg₂: enum::value)`
   *TBD*


:mini:`meth (Arg₁: integer) <> (Arg₂: enum::value)`
   *TBD*


:mini:`meth enum::cyclic(Name₁ is Value₁, ...): enum`
   Returns a new enumeration type.

   .. code-block:: mini

      let colour := enum::cyclic(Red is 10, Green is 20, Blue is 30)
      :> <<colour>>
      colour::Red :> Red
      list(colour, _ + 0) :> [Red, Green, Blue]


:mini:`meth enum::cyclic(Names: string, ...): enum`
   Returns a new enumeration type.

   .. code-block:: mini

      let day := enum::cyclic("Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun")
      :> <<day>>
      day::Wed :> Wed
      day::Fri + 0 :> Fri


