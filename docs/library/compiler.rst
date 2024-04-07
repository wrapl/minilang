.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

compiler
========

.. rst-class:: mini-api

:mini:`fun source(): tuple[string, integer]`
   Returns the caller source location. Evaluated at compile time if possible.


:mini:`fun not(Arg₁: any)`
   *TBD*


:mini:`type command::decl`
   *TBD*


:mini:`type compiler < state`
   *TBD*


:mini:`fun compiler(Global: function|map): compiler`
   *TBD*


:mini:`meth (Compiler: compiler)[Name: string]: any`
   *TBD*


:mini:`meth (Compiler: compiler):command_def(Name: string, Value: any): any`
   *TBD*


:mini:`meth (Compiler: compiler):command_let(Name: string, Value: any): any`
   *TBD*


:mini:`meth (Compiler: compiler):command_var(Name: string): variable`
   *TBD*


:mini:`meth (Compiler: compiler):command_var(Name: string, Type: type): variable`
   *TBD*


:mini:`meth (Compiler: compiler):def(Name: string, Value: any): any`
   *TBD*


:mini:`meth (Compiler: compiler):let(Name: string, Value: any): any`
   *TBD*


:mini:`meth (Compiler: compiler):var(Name: string): variable`
   *TBD*


:mini:`meth (Compiler: compiler):var(Name: string, Type: type): variable`
   *TBD*


:mini:`meth (Compiler: compiler):vars: map`
   *TBD*


:mini:`type compiler::function < state`
   *TBD*


:mini:`meth $(Arg₁: expr)`
   *TBD*


:mini:`meth (Expr: expr):compile(Compiler: compiler): any`
   *TBD*


:mini:`meth (Expr: expr):compile(Compiler: compiler, Arg₃: list): any`
   *TBD*


:mini:`meth (Arg₁: expr):end`
   *TBD*


:mini:`meth (Arg₁: expr):source`
   *TBD*


:mini:`meth (Arg₁: expr):start`
   *TBD*


:mini:`type function::inline < function`
   *TBD*


:mini:`type global`
   *TBD*


:mini:`type macro::subst < function`
   *TBD*


:mini:`type parser`
   *TBD*


:mini:`fun parser(Read?: function): parser`
   *TBD*


:mini:`meth (Parser: parser):clear: string`
   *TBD*


:mini:`meth (Parser: parser):compile(Compiler: compiler): any`
   *TBD*


:mini:`meth (Parser: parser):compile(Compiler: compiler, Parameters: list): any`
   *TBD*


:mini:`meth (Parser: parser):escape(Callback: function): parser`
   *TBD*


:mini:`meth (Parser: parser):evaluate(Compiler: compiler): any`
   *TBD*


:mini:`meth (Parser: parser):input(String: string): compiler`
   *TBD*


:mini:`meth (Parser: parser):parse: expr`
   *TBD*


:mini:`meth (Parser: parser):permissive(Permissive: boolean): parser`
   *TBD*


:mini:`meth (Parser: parser):reset: parser`
   *TBD*


:mini:`meth (Parser: parser):run(Compiler: compiler): any`
   *TBD*


:mini:`meth (Parser: parser):source(Source: string, Line: integer): tuple`
   *TBD*


:mini:`meth (Parser: parser):special(Callback: function): parser`
   *TBD*


:mini:`meth (Parser: parser):special(Callback: list): parser`
   *TBD*


:mini:`meth (Parser: parser):warnings: parser`
   *TBD*


