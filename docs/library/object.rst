object
======

.. include:: <isonum.txt>

:mini:`type object`
   Parent type of all object classes.


:mini:`type class < type`
   Type of all object classes.


:mini:`meth string(Arg₁: object)`

:mini:`fun class(Parents...: class, Fields...: method, Exports...: names)` |rarr| :mini:`class`
   Returns a new class inheriting from :mini:`Parents`, with fields :mini:`Fields` and exports :mini:`Exports`. The special exports :mini:`"of"` and :mini:`"init"` can be set to override the default conversion and initialization behaviour. The :mini:`"new"` export will *always* be set to the original constructor for this class.


:mini:`type property`

:mini:`fun property(Arg₁: function, Arg₂: function)`

:mini:`meth string(Arg₁: enumvalue)`

:mini:`fun enum(Values...: string)` |rarr| :mini:`enum`

:mini:`type enum < type, iteratable`

:mini:`meth :count(Enum: enum)` |rarr| :mini:`integer`

:mini:`meth string(Arg₁: flagsvalue)`

:mini:`fun flags(Values...: string)` |rarr| :mini:`flags`

:mini:`type flags < type`

:mini:`meth +(Arg₁: flagsvalue, Arg₂: flagsvalue)`

:mini:`meth -(Arg₁: flagsvalue, Arg₂: flagsvalue)`

:mini:`meth <(Arg₁: flagsvalue, Arg₂: flagsvalue)`

:mini:`meth <=(Arg₁: flagsvalue, Arg₂: flagsvalue)`

:mini:`meth >(Arg₁: flagsvalue, Arg₂: flagsvalue)`

:mini:`meth >=(Arg₁: flagsvalue, Arg₂: flagsvalue)`

