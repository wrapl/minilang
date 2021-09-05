compiler
========

:mini:`type compilerfunction < state`
   *TBD*

:mini:`fun compiler(Global: function|map, ?Read: function): compiler`
   *TBD*

:mini:`type compiler < state`
   *TBD*

:mini:`fun compiler()`
   *TBD*

:mini:`type parser`
   *TBD*

:mini:`meth :compile(Parser: parser, Compiler: compiler): any`
   *TBD*

:mini:`meth :compile(Parser: parser, Compiler: compiler, Parameters: list): any`
   *TBD*

:mini:`meth :source(Parser: parser, Source: string, Line: integer): tuple`
   *TBD*

:mini:`meth :reset(Parser: parser): parser`
   *TBD*

:mini:`meth :input(Parser: parser, String: string): compiler`
   *TBD*

:mini:`meth :clear(Parser: parser): string`
   *TBD*

:mini:`meth :evaluate(Parser: parser, Compiler: compiler): any`
   *TBD*

:mini:`meth :run(Compiler: parser, Arg₂: compiler): any`
   *TBD*

:mini:`meth (Compiler: compiler)[Name: string]: any`
   *TBD*

:mini:`meth :var(Compiler: compiler, Name: string): variable`
   *TBD*

:mini:`meth :var(Compiler: compiler, Name: string, Type: type): variable`
   *TBD*

:mini:`meth :let(Compiler: compiler, Name: string, Value: any): any`
   *TBD*

:mini:`meth :def(Compiler: compiler, Name: string, Value: any): any`
   *TBD*

:mini:`meth :vars(Compiler: compiler): map`
   *TBD*

:mini:`type global`
   *TBD*

:mini:`meth :command_var(Compiler: compiler, Name: string): variable`
   *TBD*

:mini:`meth :command_var(Compiler: compiler, Name: string, Type: type): variable`
   *TBD*

:mini:`meth :command_let(Compiler: compiler, Name: string, Value: any): any`
   *TBD*

:mini:`meth :command_def(Compiler: compiler, Name: string, Value: any): any`
   *TBD*

