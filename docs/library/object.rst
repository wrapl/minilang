object
======

:mini:`type object`
   Parent type of all object classes.


:mini:`type class < type`
   Type of all object classes.


:mini:`meth string(Arg₁: object)`
   *TBD*

:mini:`fun class(Parents...: class, Fields...: method, Exports...: names): class`
   Returns a new class inheriting from :mini:`Parents`, with fields :mini:`Fields` and exports :mini:`Exports`. The special exports :mini:`"of"` and :mini:`"init"` can be set to override the default conversion and initialization behaviour. The :mini:`"new"` export will *always* be set to the original constructor for this class.


:mini:`type property`
   *TBD*

:mini:`fun property(Arg₁: function, Arg₂: function)`
   *TBD*

:mini:`meth string(Arg₁: enumvalue)`
   *TBD*

:mini:`fun enum(Values...: string): enum`
   *TBD*

:mini:`type enum < type, sequence`
   *TBD*

:mini:`meth :count(Enum: enum): integer`
   *TBD*

:mini:`meth string(Arg₁: flagsvalue)`
   *TBD*

:mini:`fun flags(Values...: string): flags`
   *TBD*

:mini:`type flags < type`
   *TBD*

:mini:`meth +(Arg₁: flagsvalue, Arg₂: flagsvalue)`
   *TBD*

:mini:`meth -(Arg₁: flagsvalue, Arg₂: flagsvalue)`
   *TBD*

:mini:`meth <(Arg₁: flagsvalue, Arg₂: flagsvalue)`
   *TBD*

:mini:`meth <=(Arg₁: flagsvalue, Arg₂: flagsvalue)`
   *TBD*

:mini:`meth >(Arg₁: flagsvalue, Arg₂: flagsvalue)`
   *TBD*

:mini:`meth >=(Arg₁: flagsvalue, Arg₂: flagsvalue)`
   *TBD*

