gobject
=======

.. include:: <isonum.txt>

:mini:`TypelibT`
   A gobject-introspection typelib.

   :Parents: :mini:`iteratable`

   *Defined at line 17 in src/ml_gir.c*

:mini:`ObjectT`
   A gobject-introspection object type.

   :Parents: :mini:`type`

   *Defined at line 71 in src/ml_gir.c*

:mini:`ObjectInstanceT`
   A gobject-introspection object instance.

   *Defined at line 74 in src/ml_gir.c*

:mini:`meth string(Object: ObjectInstanceT)` |rarr| :mini:`string`
   *Defined at line 165 in src/ml_gir.c*

:mini:`StructT`
   A gobject-introspection struct type.

   :Parents: :mini:`type`

   *Defined at line 182 in src/ml_gir.c*

:mini:`StructInstanceT`
   A gobject-introspection struct instance.

   *Defined at line 185 in src/ml_gir.c*

:mini:`meth string(Struct: StructInstanceT)` |rarr| :mini:`string`
   *Defined at line 195 in src/ml_gir.c*

:mini:`EnumT`
   A gobject-instrospection enum type.

   :Parents: :mini:`type`

   *Defined at line 306 in src/ml_gir.c*

:mini:`EnumValueT`
   A gobject-instrospection enum value.

   *Defined at line 309 in src/ml_gir.c*

:mini:`meth string(Value: EnumValueT)` |rarr| :mini:`string`
   *Defined at line 312 in src/ml_gir.c*

:mini:`meth integer(Value: EnumValueT)` |rarr| :mini:`integer`
   *Defined at line 319 in src/ml_gir.c*

:mini:`meth |(Value₁: EnumValueT, Value₂: EnumValueT)` |rarr| :mini:`EnumValueT`
   *Defined at line 326 in src/ml_gir.c*

:mini:`meth ::(Typelib: TypelibT, Name: string)` |rarr| :mini:`any` or :mini:`error`
   *Defined at line 1530 in src/ml_gir.c*

:mini:`meth :connect(Object: ObjectInstanceT, Signal: string, Handler: function)` |rarr| :mini:`Object`
   *Defined at line 1627 in src/ml_gir.c*

:mini:`ObjectPropertyT`
   *Defined at line 1660 in src/ml_gir.c*

:mini:`meth ::(Object: ObjectInstanceT, Property: string)` |rarr| :mini:`any`
   *Defined at line 1665 in src/ml_gir.c*

