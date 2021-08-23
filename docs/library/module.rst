module
======

:mini:`fun module(Path: string, Lookup: function): module`
   Returns a generic module which calls resolves :mini:`Module::Import` by calling :mini:`Lookup(Module, Import)`, caching results for future use.


:mini:`type module`
   *TBD*

:mini:`meth (Module: module) :: (Name: string): any`
   Imports a symbol from a module.


:mini:`meth string(Arg₁: module)`
   *TBD*

:mini:`meth :path(Arg₁: module)`
   *TBD*

:mini:`meth :exports(Arg₁: module)`
   *TBD*

:mini:`type minimodule < module`
   *TBD*

:mini:`meth (Arg₁: minimodule) :: (Arg₂: string)`
   *TBD*

:mini:`type modulestate`
   *TBD*

