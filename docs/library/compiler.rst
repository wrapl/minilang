compiler
========

:mini:`type compiler::function < state`
   *TBD*

:mini:`meth $(Arg₁: expr)`
   *TBD*

:mini:`meth (Arg₁: expr):source`
   *TBD*

:mini:`meth (Arg₁: expr):start`
   *TBD*

:mini:`meth (Arg₁: expr):end`
   *TBD*

:mini:`fun compiler(Global: function|map): compiler`
   *TBD*

:mini:`type compiler < state`
   *TBD*

:mini:`fun parser(?Read: function): parser`
   *TBD*

:mini:`type parser`
   *TBD*

:mini:`meth (Parser: parser):compile(Compiler: compiler): any`
   *TBD*

:mini:`meth (Parser: parser):compile(Compiler: compiler, Parameters: list): any`
   *TBD*

:mini:`meth (Parser: parser):source(Source: string, Line: integer): tuple`
   *TBD*

:mini:`meth (Parser: parser):reset: parser`
   *TBD*

:mini:`meth (Parser: parser):input(String: string): compiler`
   *TBD*

:mini:`meth (Parser: parser):clear: string`
   *TBD*

:mini:`meth (Parser: parser):evaluate(Compiler: compiler): any`
   *TBD*

:mini:`meth (Compiler: parser):run(Arg₂: compiler): any`
   *TBD*

:mini:`meth (Compiler: compiler)[Name: string]: any`
   *TBD*

:mini:`meth (Compiler: compiler):var(Name: string): variable`
   *TBD*

:mini:`meth (Compiler: compiler):var(Name: string, Type: type): variable`
   *TBD*

:mini:`meth (Compiler: compiler):let(Name: string, Value: any): any`
   *TBD*

:mini:`meth (Compiler: compiler):def(Name: string, Value: any): any`
   *TBD*

:mini:`meth (Compiler: compiler):vars: map`
   *TBD*

:mini:`type global`
   *TBD*

:mini:`meth (Compiler: compiler):command_var(Name: string): variable`
   *TBD*

:mini:`meth (Compiler: compiler):command_var(Name: string, Type: type): variable`
   *TBD*

:mini:`meth (Compiler: compiler):command_let(Name: string, Value: any): any`
   *TBD*

:mini:`meth (Compiler: compiler):command_def(Name: string, Value: any): any`
   *TBD*

