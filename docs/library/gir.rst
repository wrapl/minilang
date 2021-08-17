gir
===

.. include:: <isonum.txt>

:mini:`type gir-typelib < sequence`
   A gobject-introspection typelib.


:mini:`type BaseInfoT < type`

:mini:`fun gir(Name: string)` |rarr| :mini:`gir-typelib`

:mini:`type ObjectT < BaseInfoT`
   A gobject-introspection object type.


:mini:`type ObjectInstanceT`
   A gobject-introspection object instance.


:mini:`meth string(Object: ObjectInstanceT)` |rarr| :mini:`string`

:mini:`type StructT < BaseInfoT`
   A gobject-introspection struct type.


:mini:`type StructInstanceT`
   A gobject-introspection struct instance.


:mini:`meth string(Struct: StructInstanceT)` |rarr| :mini:`string`

:mini:`type EnumT < BaseInfoT`
   A gobject-instrospection enum type.


:mini:`type EnumValueT`
   A gobject-instrospection enum value.


:mini:`meth string(Value: EnumValueT)` |rarr| :mini:`string`

:mini:`meth integer(Value: EnumValueT)` |rarr| :mini:`integer`

:mini:`meth |(Value₁: EnumValueT, Value₂: nil)` |rarr| :mini:`EnumValueT`

:mini:`meth |(Value₁: nil, Value₂: EnumValueT)` |rarr| :mini:`EnumValueT`

:mini:`meth |(Value₁: EnumValueT, Value₂: EnumValueT)` |rarr| :mini:`EnumValueT`

:mini:`meth ::(Typelib: TypelibT, Name: string)` |rarr| :mini:`any` or :mini:`error`

:mini:`meth :connect(Object: ObjectInstanceT, Signal: string, Handler: function)` |rarr| :mini:`Object`

:mini:`type ObjectPropertyT`

:mini:`meth ::(Object: ObjectInstanceT, Property: string)` |rarr| :mini:`any`

