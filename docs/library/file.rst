.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

file
====

.. rst-class:: mini-api

:mini:`fun dir(Path: string): dir`
   *TBD*


:mini:`type dir < sequence`
   *TBD*


:mini:`meth (Dir: dir):read: string`
   *TBD*


:mini:`meth (File: file):eof: File | nil`
   Returns :mini:`File` if :mini:`File` is closed,  otherwise return :mini:`nil`.


:mini:`type popen < file`
   A file that reads or writes to a running subprocess.


:mini:`fun dir::create(Path: string, Mode: integer)`
   *TBD*


:mini:`meth (File: popen):close: integer`
   Waits for the subprocess to finish and returns the exit status.


:mini:`fun mlfilet, symlink(Arg₁: string, Arg₂: string)`
   *TBD*


:mini:`meth (Arg₁: file::stat):mode`
   *TBD*


:mini:`fun file::unlink(Path: string)`
   Removes the file at :mini:`Path`.


:mini:`meth (Arg₁: file::stat):ctime`
   *TBD*


:mini:`fun file::rename(Old: string, New: string)`
   Renames the file :mini:`Old` to :mini:`New`.


:mini:`meth (Arg₁: file::stat):mtime`
   *TBD*


:mini:`fun file::exists(Path: string): string | nil`
   *TBD*


:mini:`meth (Arg₁: file::stat):size`
   *TBD*


:mini:`fun popen(Command: string, Mode: string): popen`
   Executes :mini:`Command` with the shell and returns an open file to communicate with the subprocess depending on :mini:`Mode`, 
   
   * :mini:`"r"`: opens the file for reading, 
   * :mini:`"w"`: opens the file for writing.


:mini:`fun file::stat(Path: string): file::stat`
   *TBD*


:mini:`meth (File: file):close`
   Closes :mini:`File`.


:mini:`meth (File: file):flush`
   Flushes any pending writes to :mini:`File`.


:mini:`type file::mode < enum`
   *TBD*


:mini:`meth (Arg₁: file::stat):atime`
   *TBD*


:mini:`type file::stat`
   *TBD*


:mini:`fun file(Path: string, Mode: string): file`
   Opens the file at :mini:`Path` depending on :mini:`Mode`, 
   
   * :mini:`"r"`: opens the file for reading, 
   * :mini:`"w"`: opens the file for writing, 
   * :mini:`"a"`: opens the file for appending.


:mini:`type file < stream`
   A file handle for reading / writing.


:mini:`fun file::realpath(Path: string): string | nil`
   *TBD*


:mini:`fun dir::remove(Path: string)`
   *TBD*


