file
====

.. include:: <isonum.txt>

:mini:`def file`

:mini:`fun file(Path: string, Mode: string)` |rarr| :mini:`file`

:mini:`meth :read(File: file)` |rarr| :mini:`string`

:mini:`meth :read(File: file, Length: integer)` |rarr| :mini:`string`

:mini:`meth :write(File: file, String: string)` |rarr| :mini:`File`

:mini:`meth :write(File: file, Buffer: stringbuffer)` |rarr| :mini:`File`

:mini:`meth :eof(File: file)` |rarr| :mini:`File` or :mini:`nil`

:mini:`meth :close(File: file)` |rarr| :mini:`nil`

:mini:`fun file::rename(Old: string, New: string)` |rarr| :mini:`nil`

:mini:`fun file::unlink(Path: string)`

:mini:`def dir < iteratable`

:mini:`fun dir(Path: string)` |rarr| :mini:`dir`

:mini:`meth :read(Dir: dir)` |rarr| :mini:`string`

