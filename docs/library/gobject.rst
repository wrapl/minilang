gobject
=======

.. include:: <isonum.txt>

:mini:`TypelibT`
   A gobject-introspection typelib.

   :Parents: :mini:`iteratable`


:mini:`ObjectT`
   A gobject-introspection object type.

   :Parents: :mini:`type`


:mini:`ObjectInstanceT`
   A gobject-introspection object instance.


:mini:`meth string(Object: ObjectInstanceT)` |rarr| :mini:`string`

:mini:`StructT`
   A gobject-introspection struct type.

   :Parents: :mini:`type`


:mini:`StructInstanceT`
   A gobject-introspection struct instance.


:mini:`meth string(Struct: StructInstanceT)` |rarr| :mini:`string`

:mini:`EnumT`
   A gobject-instrospection enum type.

   :Parents: :mini:`type`


:mini:`EnumValueT`
   A gobject-instrospection enum value.


:mini:`meth string(Value: EnumValueT)` |rarr| :mini:`string`

:mini:`meth integer(Value: EnumValueT)` |rarr| :mini:`integer`

:mini:`meth |(Value₁: EnumValueT, Value₂: EnumValueT)` |rarr| :mini:`EnumValueT`

:mini:`meth ::(Typelib: TypelibT, Name: string)` |rarr| :mini:`any` or :mini:`error`

:mini:`meth :connect(Object: ObjectInstanceT, Signal: string, Handler: function)` |rarr| :mini:`Object`

:mini:`ObjectPropertyT`

:mini:`meth ::(Object: ObjectInstanceT, Property: string)` |rarr| :mini:`any`

