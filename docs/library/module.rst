module
======

:mini:`fun module(Path: string, Lookup: function): module`
   Returns a generic module which calls resolves :mini:`Module::Import` by calling :mini:`Lookup(Import)`, caching results for future use.


:mini:`type module`
   *TBD*

:mini:`meth (Module: module) :: (Name: string): any`
   Imports a symbol from a module.


:mini:`meth (Arg₁: string::buffer):append(Arg₂: module)`
   *TBD*

:mini:`meth (Arg₁: module):path`
   *TBD*

:mini:`meth (Arg₁: module):exports`
   *TBD*

