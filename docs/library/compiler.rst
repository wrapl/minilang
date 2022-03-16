.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

compiler
========

.. _type-compiler:

:mini:`type compiler < state`
   *TBD*


.. _fun-compiler:

:mini:`fun compiler(Global: function|map): compiler`
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


:mini:`meth (Compiler: compiler)[Name: string]: any`
   *TBD*


.. _type-compiler-function:

:mini:`type compiler::function < state`
   *TBD*


:mini:`meth $(Arg₁: expr)`
   *TBD*


:mini:`meth (Arg₁: expr):end`
   *TBD*


:mini:`meth (Arg₁: expr):source`
   *TBD*


:mini:`meth (Arg₁: expr):start`
   *TBD*


.. _type-global:

:mini:`type global`
   *TBD*


.. _type-parser:

:mini:`type parser`
   *TBD*


.. _fun-parser:

:mini:`fun parser(Read?: function): parser`
   *TBD*


:mini:`meth (Parser: parser):clear: string`
   *TBD*


:mini:`meth (Parser: parser):compile(Compiler: compiler): any`
   *TBD*


:mini:`meth (Parser: parser):compile(Compiler: compiler, Parameters: list): any`
   *TBD*


:mini:`meth (Parser: parser):evaluate(Compiler: compiler): any`
   *TBD*


:mini:`meth (Parser: parser):input(String: string): compiler`
   *TBD*


:mini:`meth (Parser: parser):parse: expr`
   *TBD*


:mini:`meth (Parser: parser):parse(Arg₂: string): expr`
   *TBD*


:mini:`meth (Parser: parser):permissive(Permissive: boolean): parser`
   *TBD*


:mini:`meth (Parser: parser):reset: parser`
   *TBD*


:mini:`meth (Compiler: parser):run(Arg₂: compiler): any`
   *TBD*


:mini:`meth (Parser: parser):source(Source: string, Line: integer): tuple`
   *TBD*


