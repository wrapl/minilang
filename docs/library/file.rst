.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

file
====

.. rst-class:: mini-api

.. _type-dir:

:mini:`type dir < sequence`
   *TBD*


.. _fun-dir:

:mini:`fun dir(Path: string): dir`
   *TBD*


:mini:`meth (Dir: dir):read: string`
   *TBD*


.. _type-file:

:mini:`type file < stream`
   A file handle for reading / writing.


.. _fun-file:

:mini:`fun file(Path: string, Mode: string): file`
   Opens the file at :mini:`Path` depending on :mini:`Mode`, 
   
   * :mini:`"r"`: opens the file for reading, 
   * :mini:`"w"`: opens the file for writing, 
   * :mini:`"a"`: opens the file for appending.


:mini:`meth (File: file):close`
   Closes :mini:`File`.


:mini:`meth (File: file):eof: File | nil`
   Returns :mini:`File` if :mini:`File` is closed,  otherwise return :mini:`nil`.


:mini:`meth (File: file):flush`
   Flushes any pending writes to :mini:`File`.


.. _type-popen:

:mini:`type popen < file`
   A file that reads or writes to a running subprocess.


.. _fun-popen:

:mini:`fun popen(Command: string, Mode: string): popen`
   Executes :mini:`Command` with the shell and returns an open file to communicate with the subprocess depending on :mini:`Mode`, 
   
   * :mini:`"r"`: opens the file for reading, 
   * :mini:`"w"`: opens the file for writing.


:mini:`meth (File: popen):close: integer`
   Waits for the subprocess to finish and returns the exit status.


.. _fun-file-rename:

:mini:`fun file::rename(Old: string, New: string)`
   Renames the file :mini:`Old` to :mini:`New`.


.. _fun-file-unlink:

:mini:`fun file::unlink(Path: string)`
   Removes the file at :mini:`Path`.


