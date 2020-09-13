file
====

.. include:: <isonum.txt>

:mini:`file`
   *Defined at line 22 in src/ml_file.c*

:mini:`fun file(Path: string, Mode: string)` |rarr| :mini:`file`
:mini:`meth :read(File: file)` |rarr| :mini:`string`
   *Defined at line 74 in src/ml_file.c*

:mini:`meth :read(File: file, Length: integer)` |rarr| :mini:`string`
   *Defined at line 90 in src/ml_file.c*

:mini:`meth :write(File: file, String: string)` |rarr| :mini:`File`
   *Defined at line 117 in src/ml_file.c*

:mini:`meth :write(File: file, Buffer: stringbuffer)` |rarr| :mini:`File`
   *Defined at line 146 in src/ml_file.c*

:mini:`meth :eof(File: file)` |rarr| :mini:`File` or :mini:`nil`
   *Defined at line 157 in src/ml_file.c*

:mini:`meth :close(File: file)` |rarr| :mini:`nil`
   *Defined at line 166 in src/ml_file.c*

:mini:`fun file::rename(Old: string, New: string)` |rarr| :mini:`nil`
:mini:`fun file::unlink(Path: string)`
:mini:`dir`
   :Parents: :mini:`iteratable`

   *Defined at line 224 in src/ml_file.c*

:mini:`fun dir(Path: string)` |rarr| :mini:`dir`
:mini:`meth :read(Dir: dir)` |rarr| :mini:`string`
   *Defined at line 251 in src/ml_file.c*

