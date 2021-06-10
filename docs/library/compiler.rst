compiler
========

.. include:: <isonum.txt>

:mini:`type macro`

:mini:`type compilerfunction < state`

:mini:`fun compiler(Global: function|map, ?Read: function)` |rarr| :mini:`compiler`

:mini:`type compiler < state`

:mini:`fun compiler()`

:mini:`type parser`

:mini:`meth :compile(Parser: parser, Compiler: compiler)` |rarr| :mini:`any`

:mini:`meth :compile(Parser: parser, Compiler: compiler, Parameters: list)` |rarr| :mini:`any`

:mini:`meth :source(Parser: parser, Source: string, Line: integer)` |rarr| :mini:`tuple`

:mini:`meth :reset(Parser: parser)` |rarr| :mini:`parser`

:mini:`meth :input(Parser: parser, String: string)` |rarr| :mini:`compiler`

:mini:`meth :clear(Parser: parser)` |rarr| :mini:`string`

:mini:`meth :evaluate(Parser: parser, Compiler: compiler)` |rarr| :mini:`any`

:mini:`meth :run(Compiler: parser, Arg₂: compiler)` |rarr| :mini:`any`

:mini:`meth (Compiler: compiler)[Name: string]` |rarr| :mini:`any`

:mini:`type global`

:mini:`meth :var(Compiler: compiler, Name: string)` |rarr| :mini:`global`

:mini:`meth :var(Compiler: compiler, Name: string, Type: type)` |rarr| :mini:`global`

:mini:`meth :let(Compiler: compiler, Name: string, Value: any)` |rarr| :mini:`global`

:mini:`meth :def(Compiler: compiler, Name: string, Value: any)` |rarr| :mini:`global`

:mini:`meth :vars(Compiler: compiler)` |rarr| :mini:`map`

