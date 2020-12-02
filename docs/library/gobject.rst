gobject
=======

.. include:: <isonum.txt>

:mini:`def TypelibT < iteratable`
   A gobject-introspection typelib.


:mini:`def ObjectT < type`
   A gobject-introspection object type.


:mini:`def ObjectInstanceT`
   A gobject-introspection object instance.


:mini:`meth string(Object: ObjectInstanceT)` |rarr| :mini:`string`

:mini:`def StructT < type`
   A gobject-introspection struct type.


:mini:`def StructInstanceT`
   A gobject-introspection struct instance.


:mini:`meth string(Struct: StructInstanceT)` |rarr| :mini:`string`

:mini:`def EnumT < type`
   A gobject-instrospection enum type.


:mini:`def EnumValueT`
   A gobject-instrospection enum value.


:mini:`meth string(Value: EnumValueT)` |rarr| :mini:`string`

:mini:`meth integer(Value: EnumValueT)` |rarr| :mini:`integer`

:mini:`meth |(Value₁: EnumValueT, Value₂: EnumValueT)` |rarr| :mini:`EnumValueT`

:mini:`meth ::(Typelib: TypelibT, Name: string)` |rarr| :mini:`any` or :mini:`error`

:mini:`meth :connect(Object: ObjectInstanceT, Signal: string, Handler: function)` |rarr| :mini:`Object`

:mini:`def ObjectPropertyT`

:mini:`meth ::(Object: ObjectInstanceT, Property: string)` |rarr| :mini:`any`

