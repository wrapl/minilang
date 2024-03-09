.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

gir
===

.. rst-class:: mini-api

:mini:`fun gir::install()`
   *TBD*


:mini:`type gir < (MLSequence`
   A gobject-introspection typelib.


:mini:`type gir::baseinfo < (MLType`
   *TBD*


:mini:`meth (Arg₁: gir::baseinfo):name`
   *TBD*


:mini:`type gir::callback < (GirBaseInfo`
   A gobject-introspection callback type.


:mini:`meth (Arg₁: gir::callback):list`
   *TBD*


:mini:`type gir::callbackinstance`
   A gobject-introspection callback instance.


:mini:`type gir::enum < (GirBaseInfo`
   A gobject-instrospection enum type.


:mini:`type gir::enumvalue`
   A gobject-instrospection enum value.


:mini:`meth (Value₁: gir::enumvalue) | (Value₂: gir::enumvalue): enumvalue`
   *TBD*


:mini:`meth (Value₁: gir::enumvalue) | (Value₂: nil): enumvalue`
   *TBD*


:mini:`meth (Value: string::buffer):append(Arg₂: gir::enumvalue): string`
   *TBD*


:mini:`type gir::fieldref`
   *TBD*


:mini:`type gir::fieldref-boolean < (GirFieldRef`
   *TBD*


:mini:`type gir::fieldref-double < (GirFieldRef`
   *TBD*


:mini:`type gir::fieldref-float < (GirFieldRef`
   *TBD*


:mini:`type gir::fieldref-int16 < (GirFieldRef`
   *TBD*


:mini:`type gir::fieldref-int32 < (GirFieldRef`
   *TBD*


:mini:`type gir::fieldref-int64 < (GirFieldRef`
   *TBD*


:mini:`type gir::fieldref-int8 < (GirFieldRef`
   *TBD*


:mini:`type gir::fieldref-uint16 < (GirFieldRef`
   *TBD*


:mini:`type gir::fieldref-uint32 < (GirFieldRef`
   *TBD*


:mini:`type gir::fieldref-uint64 < (GirFieldRef`
   *TBD*


:mini:`type gir::fieldref-uint8 < (GirFieldRef`
   *TBD*


:mini:`type gir::fieldref-utf8 < (GirFieldRef`
   *TBD*


:mini:`type gir::function < (MLFunction`
   *TBD*


:mini:`meth (Arg₁: gir::function):list`
   *TBD*


:mini:`type gir::instance`
   *TBD*


:mini:`type gir::module`
   *TBD*


:mini:`meth (Arg₁: gir::module) :: (Arg₂: string)`
   *TBD*


:mini:`type gir::object < (GirBaseInfo`
   A gobject-introspection object type.


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


:mini:`type gir::objectproperty`
   *TBD*


:mini:`type gir::struct < (GirBaseInfo`
   A gobject-introspection struct type.


:mini:`type gir::structinstance`
   A gobject-introspection struct instance.


:mini:`meth (Struct: string::buffer):append(Arg₂: gir::structinstance): string`
   *TBD*


:mini:`meth (Typelib: gir::typelib) :: (Name: string): any | error`
   *TBD*


:mini:`type gir::union < (GirBaseInfo`
   A gobject-introspection struct type.


:mini:`type gir::unioninstance`
   A gobject-introspection struct instance.


:mini:`meth (Union: string::buffer):append(Arg₂: gir::unioninstance): string`
   *TBD*


:mini:`meth integer(Value: gir::enumvalue): integer`
   *TBD*


:mini:`meth (Value₁: nil) | (Value₂: gir::enumvalue): enumvalue`
   *TBD*


:mini:`fun sleep(Arg₁: number)`
   *TBD*


:mini:`meth (Arg₁: string):GirTypelibT`
   *TBD*


:mini:`meth (Arg₁: string):GirTypelibT(Arg₂: string)`
   *TBD*


