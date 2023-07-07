.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

file
====

.. rst-class:: mini-api

:mini:`type dir < sequence`
   *TBD*


:mini:`fun dir(Path: string): dir`
   *TBD*


:mini:`meth (Dir: dir):read: string`
   *TBD*


:mini:`type file < stream`
   A file handle for reading / writing.


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


:mini:`type popen < file`
   A file that reads or writes to a running subprocess.


:mini:`fun popen(Command: string, Mode: string): popen`
   Executes :mini:`Command` with the shell and returns an open file to communicate with the subprocess depending on :mini:`Mode`, 
   
   * :mini:`"r"`: opens the file for reading, 
   * :mini:`"w"`: opens the file for writing.


:mini:`meth (File: popen):close: integer`
   Waits for the subprocess to finish and returns the exit status.


:mini:`fun file::exists(Path: string): string | nil`
   *TBD*


:mini:`fun file::rename(Old: string, New: string)`
   Renames the file :mini:`Old` to :mini:`New`.


:mini:`fun file::unlink(Path: string)`
   Removes the file at :mini:`Path`.


