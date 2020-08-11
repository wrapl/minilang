file
====

.. include:: <isonum.txt>

:mini:`file`
   *Defined at line 22 in src/ml_file.c*

:mini:`fun file(Path: string, Mode: string)` |rarr| :mini:`file`
   *Defined at line 34 in src/ml_file.c*

:mini:`meth :read(File: file)` |rarr| :mini:`string`
   *Defined at line 75 in src/ml_file.c*

:mini:`meth :read(File: file, Length: integer)` |rarr| :mini:`string`
   *Defined at line 91 in src/ml_file.c*

:mini:`meth :write(File: file, String: string)` |rarr| :mini:`File`
   *Defined at line 118 in src/ml_file.c*

:mini:`meth :write(File: file, Buffer: stringbuffer)` |rarr| :mini:`File`
   *Defined at line 147 in src/ml_file.c*

:mini:`meth :eof(File: file)` |rarr| :mini:`File` or :mini:`nil`
   *Defined at line 158 in src/ml_file.c*

:mini:`meth :close(File: file)` |rarr| :mini:`nil`
   *Defined at line 167 in src/ml_file.c*

:mini:`fun file::rename(Old: string, New: string)` |rarr| :mini:`nil`
   *Defined at line 186 in src/ml_file.c*

:mini:`fun file::unlink(Path: string)`
   *Defined at line 203 in src/ml_file.c*

:mini:`dir`
   :Parents: :mini:`iteratable`

   *Defined at line 225 in src/ml_file.c*

:mini:`fun dir(Path: string)` |rarr| :mini:`dir`
   *Defined at line 236 in src/ml_file.c*

:mini:`meth :read(Dir: dir)` |rarr| :mini:`string`
   *Defined at line 252 in src/ml_file.c*

