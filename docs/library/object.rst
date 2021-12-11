object
======

:mini:`type object`
   Parent type of all object classes.


:mini:`meth (Object: object) :: (Field: string): field`
   Retrieves the field :mini:`Field` from :mini:`Object`. Mainly intended for unpacking objects.


:mini:`type class < type`
   Type of all object classes.


:mini:`meth (Arg₁: string::buffer):append(Arg₂: object)`
   *TBD*

:mini:`fun class(Parents...: class, Fields...: method, Exports...: names): class`
   Returns a new class inheriting from :mini:`Parents`,  with fields :mini:`Fields` and exports :mini:`Exports`. The special exports :mini:`"of"` and :mini:`"init"` can be set to override the default conversion and initialization behaviour. The :mini:`"new"` export will *always* be set to the original constructor for this class.


:mini:`type property`
   A value with an associated setter function.


:mini:`fun property(Value: any, set: any): property`
   Returns a new property which dereferences to :mini:`Value`. Assigning to the property will call :mini:`set(NewValue)`.


:mini:`meth (Arg₁: string::buffer):append(Arg₂: enum::value)`
   *TBD*

:mini:`fun enum(Values...: string): enum`
   *TBD*

:mini:`type enum < type, sequence`
   *TBD*

:mini:`meth (Enum: enum):count: integer`
   *TBD*

:mini:`type enum::range`
   *TBD*

:mini:`type enum::range < sequence`
   *TBD*

:mini:`meth (Arg₁: enum::value) .. (Arg₂: enum::value)`
   *TBD*

:mini:`type flags < type`
   *TBD*

:mini:`meth (Arg₁: string::buffer):append(Arg₂: flags::value)`
   *TBD*

:mini:`meth (Values...: string):flags(): flags`
   *TBD*

:mini:`meth (Values...: names):flags(): flags`
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

