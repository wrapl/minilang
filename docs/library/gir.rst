gir
===

:mini:`type gir-typelib < sequence`
   A gobject-introspection typelib.


:mini:`type gir::base < type`
   *TBD*

:mini:`type gir < function`
   *TBD*

:mini:`fun gir(Name: string): gir-typelib`
   *TBD*

:mini:`type gir::object < gir::base`
   A gobject-introspection object type.


:mini:`type gir::object`
   A gobject-introspection object instance.


:mini:`meth (Object: string::buffer):append(Arg₂: gir::object): string`
   *TBD*

:mini:`type gir::struct < gir::base`
   A gobject-introspection struct type.


:mini:`type gir::struct`
   A gobject-introspection struct instance.


:mini:`meth (Struct: string::buffer):append(Arg₂: gir::struct): string`
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

:mini:`type gir::enum < gir::base`
   A gobject-instrospection enum type.


:mini:`type gir::enum`
   A gobject-instrospection enum value.


:mini:`meth (Value: string):append(Arg₂: gir::enum): string`
   *TBD*

:mini:`meth integer(Value: gir::enum): integer`
   *TBD*

:mini:`meth (Value₁: gir::enum) | (Value₂: nil): enumvalue`
   *TBD*

:mini:`meth (Value₁: nil) | (Value₂: gir::enum): enumvalue`
   *TBD*

:mini:`meth (Value₁: gir::enum) | (Value₂: gir::enum): enumvalue`
   *TBD*

:mini:`meth (Typelib: gir::typelib) :: (Name: string): any | error`
   *TBD*

:mini:`meth (Object: gir::object):connect(Signal: string, Handler: function): Object`
   *TBD*

:mini:`type gir::object`
   *TBD*

:mini:`meth (Object: gir::object) :: (Property: string): any`
   *TBD*

:mini:`fun sleep(Arg₁: number)`
   *TBD*

:mini:`fun mlgirrun(Arg₁: any)`
   *TBD*

