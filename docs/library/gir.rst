gir
===

:mini:`type gir-typelib < sequence`
   A gobject-introspection typelib.


:mini:`type BaseInfoT < type`
   *TBD*

:mini:`fun gir(Name: string): gir-typelib`
   *TBD*

:mini:`type ObjectT < BaseInfoT`
   A gobject-introspection object type.


:mini:`type ObjectInstanceT`
   A gobject-introspection object instance.


:mini:`meth string(Object: ObjectInstanceT): string`
   *TBD*

:mini:`type StructT < BaseInfoT`
   A gobject-introspection struct type.


:mini:`type StructInstanceT`
   A gobject-introspection struct instance.


:mini:`meth string(Struct: StructInstanceT): string`
   *TBD*

:mini:`type FieldRef ## Boolean ## T`
   *TBD*

:mini:`type FieldRef ## Int8 ## T`
   *TBD*

:mini:`type FieldRef ## UInt8 ## T`
   *TBD*

:mini:`type FieldRef ## Int16 ## T`
   *TBD*

:mini:`type FieldRef ## UInt16 ## T`
   *TBD*

:mini:`type FieldRef ## Int32 ## T`
   *TBD*

:mini:`type FieldRef ## UInt32 ## T`
   *TBD*

:mini:`type FieldRef ## Int64 ## T`
   *TBD*

:mini:`type FieldRef ## UInt64 ## T`
   *TBD*

:mini:`type FieldRef ## Float ## T`
   *TBD*

:mini:`type FieldRef ## Double ## T`
   *TBD*

:mini:`type FieldRef ## Utf8 ## T`
   *TBD*

:mini:`type EnumT < BaseInfoT`
   A gobject-instrospection enum type.


:mini:`type EnumValueT`
   A gobject-instrospection enum value.


:mini:`meth string(Value: EnumValueT): string`
   *TBD*

:mini:`meth integer(Value: EnumValueT): integer`
   *TBD*

:mini:`meth (Value₁: EnumValueT) | (Value₂: nil): EnumValueT`
   *TBD*

:mini:`meth (Value₁: nil) | (Value₂: EnumValueT): EnumValueT`
   *TBD*

:mini:`meth (Value₁: EnumValueT) | (Value₂: EnumValueT): EnumValueT`
   *TBD*

:mini:`meth (Typelib: TypelibT) :: (Name: string): any | error`
   *TBD*

:mini:`meth :connect(Object: ObjectInstanceT, Signal: string, Handler: function): Object`
   *TBD*

:mini:`type ObjectPropertyT`
   *TBD*

:mini:`meth (Object: ObjectInstanceT) :: (Property: string): any`
   *TBD*

