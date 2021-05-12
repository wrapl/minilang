compiler
========

.. include:: <isonum.txt>

:mini:`type macro`

:mini:`fun compiler(Global: function|map, ?Read: function)` |rarr| :mini:`compiler`

:mini:`type compiler < state`

:mini:`meth :compile(Compiler: compiler)` |rarr| :mini:`any`

:mini:`meth :compile(Compiler: compiler, Parameters: list)` |rarr| :mini:`any`

:mini:`meth :source(Compiler: compiler, Source: string, Line: integer)` |rarr| :mini:`tuple`

:mini:`meth :reset(Compiler: compiler)` |rarr| :mini:`compiler`

:mini:`meth :input(Compiler: compiler, String: string)` |rarr| :mini:`compiler`

:mini:`meth :clear(Compiler: compiler)` |rarr| :mini:`string`

:mini:`meth :evaluate(Compiler: compiler)` |rarr| :mini:`any`

:mini:`meth :run(Compiler: compiler)` |rarr| :mini:`any`

:mini:`meth (Compiler: compiler)[Name: string]` |rarr| :mini:`any`

:mini:`type global`

:mini:`meth :var(Compiler: compiler, Name: string)` |rarr| :mini:`global`

:mini:`meth :var(Compiler: compiler, Name: string, Type: type)` |rarr| :mini:`global`

:mini:`meth :let(Compiler: compiler, Name: string, Value: any)` |rarr| :mini:`global`

:mini:`meth :def(Compiler: compiler, Name: string, Value: any)` |rarr| :mini:`global`

:mini:`meth :vars(Compiler: compiler)` |rarr| :mini:`map`

