.. include:: <isonum.txt>

gir
===

.. _type-gir-typelib:

:mini:`type gir-typelib < sequence`
   A gobject-introspection typelib.


.. _type-gir-base:

:mini:`type gir::base < type`
   *TBD*

:mini:`meth (Arg₁: string):GirTypelibT`
   *TBD*

:mini:`meth (Arg₁: string):GirTypelibT(Arg₂: string)`
   *TBD*

.. _type-gir-object:

:mini:`type gir::object < gir::base`
   A gobject-introspection object type.


.. _type-gir-object:

:mini:`type gir::object`
   A gobject-introspection object instance.


:mini:`meth (Object: string::buffer):append(Arg₂: gir::object): string`
   *TBD*

.. _type-gir-struct:

:mini:`type gir::struct < gir::base`
   A gobject-introspection struct type.


.. _type-gir-struct:

:mini:`type gir::struct`
   A gobject-introspection struct instance.


:mini:`meth (Struct: string::buffer):append(Arg₂: gir::struct): string`
   *TBD*

.. _type-fieldref-boolean:

:mini:`type fieldref::boolean`
   *TBD*

.. _type-fieldref-int8:

:mini:`type fieldref::int8`
   *TBD*

.. _type-fieldref-uint8:

:mini:`type fieldref::uint8`
   *TBD*

.. _type-fieldref-int16:

:mini:`type fieldref::int16`
   *TBD*

.. _type-fieldref-uint16:

:mini:`type fieldref::uint16`
   *TBD*

.. _type-fieldref-int32:

:mini:`type fieldref::int32`
   *TBD*

.. _type-fieldref-uint32:

:mini:`type fieldref::uint32`
   *TBD*

.. _type-fieldref-int64:

:mini:`type fieldref::int64`
   *TBD*

.. _type-fieldref-uint64:

:mini:`type fieldref::uint64`
   *TBD*

.. _type-fieldref-float:

:mini:`type fieldref::float`
   *TBD*

.. _type-fieldref-double:

:mini:`type fieldref::double`
   *TBD*

.. _type-fieldref-utf8:

:mini:`type fieldref::utf8`
   *TBD*

.. _type-gir-enum:

:mini:`type gir::enum < gir::base`
   A gobject-instrospection enum type.


.. _type-gir-enum:

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

.. _type-gir-object:

:mini:`type gir::object`
   *TBD*

:mini:`meth (Object: gir::object) :: (Property: string): any`
   *TBD*

.. _fun-sleep:

:mini:`fun sleep(Arg₁: number)`
   *TBD*

.. _fun-girrun:

:mini:`fun girrun(Arg₁: any)`
   *TBD*

