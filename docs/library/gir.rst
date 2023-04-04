.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

gir
===

.. rst-class:: mini-api

.. _fun-girtype:

:mini:`fun girtype()`
   *TBD*


.. _fun-girrun:

:mini:`fun girrun(Arg₁: any)`
   *TBD*


.. _type-gir:

:mini:`type gir < sequence`
   A gobject-introspection typelib.


.. _type-gir-baseinfo:

:mini:`type gir::baseinfo < type`
   *TBD*


:mini:`meth (Arg₁: gir::baseinfo):name`
   *TBD*


.. _type-gir-callback:

:mini:`type gir::callback < gir::baseinfo`
   A gobject-introspection callback type.


.. _type-gir-callbackinstance:

:mini:`type gir::callbackinstance`
   A gobject-introspection callback instance.


.. _type-gir-enum:

:mini:`type gir::enum < gir::baseinfo`
   A gobject-instrospection enum type.


.. _type-gir-enumvalue:

:mini:`type gir::enumvalue`
   A gobject-instrospection enum value.


:mini:`meth (Value₁: gir::enumvalue) | (Value₂: gir::enumvalue): enumvalue`
   *TBD*


:mini:`meth (Value₁: gir::enumvalue) | (Value₂: nil): enumvalue`
   *TBD*


:mini:`meth (Value: string::buffer):append(Arg₂: gir::enumvalue): string`
   *TBD*


.. _type-gir-fieldref:

:mini:`type gir::fieldref`
   *TBD*


.. _type-gir-fieldref-boolean:

:mini:`type gir::fieldref-boolean < gir::fieldref`
   *TBD*


.. _type-gir-fieldref-double:

:mini:`type gir::fieldref-double < gir::fieldref`
   *TBD*


.. _type-gir-fieldref-float:

:mini:`type gir::fieldref-float < gir::fieldref`
   *TBD*


.. _type-gir-fieldref-int16:

:mini:`type gir::fieldref-int16 < gir::fieldref`
   *TBD*


.. _type-gir-fieldref-int32:

:mini:`type gir::fieldref-int32 < gir::fieldref`
   *TBD*


.. _type-gir-fieldref-int64:

:mini:`type gir::fieldref-int64 < gir::fieldref`
   *TBD*


.. _type-gir-fieldref-int8:

:mini:`type gir::fieldref-int8 < gir::fieldref`
   *TBD*


.. _type-gir-fieldref-uint16:

:mini:`type gir::fieldref-uint16 < gir::fieldref`
   *TBD*


.. _type-gir-fieldref-uint32:

:mini:`type gir::fieldref-uint32 < gir::fieldref`
   *TBD*


.. _type-gir-fieldref-uint64:

:mini:`type gir::fieldref-uint64 < gir::fieldref`
   *TBD*


.. _type-gir-fieldref-uint8:

:mini:`type gir::fieldref-uint8 < gir::fieldref`
   *TBD*


.. _type-gir-fieldref-utf8:

:mini:`type gir::fieldref-utf8 < gir::fieldref`
   *TBD*


.. _type-gir-function:

:mini:`type gir::function < function`
   *TBD*


:mini:`meth (Arg₁: gir::function):list`
   *TBD*


.. _type-gir-instance:

:mini:`type gir::instance`
   *TBD*


.. _type-gir-module:

:mini:`type gir::module`
   *TBD*


:mini:`meth (Arg₁: gir::module) :: (Arg₂: string)`
   *TBD*


.. _type-gir-object:

:mini:`type gir::object < gir::baseinfo`
   A gobject-introspection object type.


.. _type-gir-objectinstance:

:mini:`type gir::objectinstance`
   A gobject-introspection object instance.


:mini:`meth (Object: gir::objectinstance) :: (Property: string): any`
   *TBD*


:mini:`meth (Object: gir::objectinstance):connect(Signal: string, Handler: function): Object`
   *TBD*


:mini:`meth (Arg₁: gir::objectinstance):disconnect(Arg₂: integer)`
   *TBD*


:mini:`meth (Object: string::buffer):append(Arg₂: gir::objectinstance): string`
   *TBD*


.. _type-gir-objectproperty:

:mini:`type gir::objectproperty`
   *TBD*


.. _type-gir-struct:

:mini:`type gir::struct < gir::baseinfo`
   A gobject-introspection struct type.


.. _type-gir-structinstance:

:mini:`type gir::structinstance`
   A gobject-introspection struct instance.


:mini:`meth (Struct: string::buffer):append(Arg₂: gir::structinstance): string`
   *TBD*


.. _type-gir-type:

:mini:`type gir::type < type`
   *TBD*


:mini:`meth (Typelib: gir::typelib) :: (Name: string): any | error`
   *TBD*


.. _type-gir-union:

:mini:`type gir::union < gir::baseinfo`
   A gobject-introspection struct type.


.. _type-gir-unioninstance:

:mini:`type gir::unioninstance`
   A gobject-introspection struct instance.


:mini:`meth (Union: string::buffer):append(Arg₂: gir::unioninstance): string`
   *TBD*


:mini:`meth integer(Value: gir::enumvalue): integer`
   *TBD*


:mini:`meth (Value₁: nil) | (Value₂: gir::enumvalue): enumvalue`
   *TBD*


.. _fun-sleep:

:mini:`fun sleep(Arg₁: number)`
   *TBD*


:mini:`meth (Arg₁: string):GirTypelibT`
   *TBD*


:mini:`meth (Arg₁: string):GirTypelibT(Arg₂: string)`
   *TBD*


