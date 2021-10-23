gir
===

:mini:`type gir-typelib < sequence`
   A gobject-introspection typelib.


:mini:`type baseinfo < type`
   *TBD*

:mini:`fun gir(Name: string): gir-typelib`
   *TBD*

:mini:`type object < baseinfo`
   A gobject-introspection object type.


:mini:`type objectinstance`
   A gobject-introspection object instance.


:mini:`meth string(Object: objectinstance): string`
   *TBD*

:mini:`type struct < baseinfo`
   A gobject-introspection struct type.


:mini:`type structinstance`
   A gobject-introspection struct instance.


:mini:`meth string(Struct: structinstance): string`
   *TBD*

:mini:`type fieldref::boolean`
   *TBD*

:mini:`type fieldref::int8`
   *TBD*

:mini:`type fieldref::uint8`
   *TBD*

:mini:`type fieldref::int16`
   *TBD*

:mini:`type fieldref::uint16`
   *TBD*

:mini:`type fieldref::int32`
   *TBD*

:mini:`type fieldref::uint32`
   *TBD*

:mini:`type fieldref::int64`
   *TBD*

:mini:`type fieldref::uint64`
   *TBD*

:mini:`type fieldref::float`
   *TBD*

:mini:`type fieldref::double`
   *TBD*

:mini:`type fieldref::utf8`
   *TBD*

:mini:`type enum < baseinfo`
   A gobject-instrospection enum type.


:mini:`type enumvalue`
   A gobject-instrospection enum value.


:mini:`meth string(Value: enumvalue): string`
   *TBD*

:mini:`meth integer(Value: enumvalue): integer`
   *TBD*

:mini:`meth (Value₁: enumvalue) | (Value₂: nil): enumvalue`
   *TBD*

:mini:`meth (Value₁: nil) | (Value₂: enumvalue): enumvalue`
   *TBD*

:mini:`meth (Value₁: enumvalue) | (Value₂: enumvalue): enumvalue`
   *TBD*

:mini:`meth (Typelib: typelib) :: (Name: string): any | error`
   *TBD*

:mini:`meth (Object: objectinstance):connect(Signal: string, Handler: function): Object`
   *TBD*

:mini:`type objectproperty`
   *TBD*

:mini:`meth (Object: objectinstance) :: (Property: string): any`
   *TBD*

