Getting Started
===============

Download
--------

Download the *Minilang* source code from
`Github <https://github.com/wrapl/minilang>`_.

Building
--------

There are two ways to build the *Minilang* library (:file:`lib/libminilang.a`)
and interpreter (:file:`bin/minilang`).

Using *Rabs*
~~~~~~~~~~~~

This is the preferred method for building the *Minilang* library and interpreter. It requires installing *Rabs* first, see `here <https://rabs.readthedocs.io/en/latest/quickstart.html>`_ for instructions.

.. code-block:: console

   $ git clone https://github.com/wrapl/minilang
   $ cd minilang
   $ rabs -D<build option 1> -D<build option 2> ...

A number of build options can be defined when invoking *rabs* to enable some additional features:

:-DNANBOXING: Uses nan-boxing techniques to reduce memory usage for numeric values. Provides significant speed improvements but may not be available / fully tested on all platforms. Currently tested on *x64* only.

:-DMATH: Adds several maths functions and multiple dimensional numeric arrays to the library. See :doc:`/library/math` and :doc:`/library/array`.

:-DCOMPLEX: Adds support for complex numbers.

:-DTABLES: Adds support for working with tabular data. See :doc:`/library/table`. Enables ``-DMATH``.

:-DGIR: Adds *gobject-introspection* functions to the library. See :doc:`/library/gir`.

:-DGTK_CONSOLE: Adds a GTK+ console to the interpreter, enables ``-DGIR``.

:-DCBOR: Adds *CBOR* encoding and decoding functions to the library. See :doc:`/library/cbor` and :doc:`/api/cbor`.

:-DCBOR: Adds *JSON* encoding and decoding functions to the library. See :doc:`/library/json`.

:-DSCHEDULER: Adds support for green threads, i.e. running *Minilang* code can be suspended and resumed automatically to simulate parallel execution. 

:-DMODULES: Adds support for loading modules at runtime. On *Linux*, this includes support for shared object (:file:`.so`) files. Enables ``-DSCHEDULER``.

:-DTRE: Uses the TRE library (https://laurikari.net/tre/) for regular expressions, supporting fuzzy matching.

:-DGENERICS: Adds support for generic types.

:-DTIME: Adds support for time values. See :doc:`/library/time`.

:-DUUID: Adds support for UUID values. See :doc:`/library/uuid`.

Using *Make*
~~~~~~~~~~~~

.. code-block:: console

   $ git clone https://github.com/wrapl/minilang
   $ cd minilang
   $ make


Running
-------

Building *Minilang* with either *Rabs* or *Make* will produce an executable interpreter :file:`bin/minilang`. Running :file:`bin/minilang` without arguments will open a *Minilang* REPL where you can test the language. Type :kbd:`Ctrl` + :kbd:`C` to exit the REPL.

.. code-block:: console

   $ ./bin/minilang
   --> list(1 .. 10)
   [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
   --> print("Hello world!\n")
   Hello world!
   nil
   --> <Ctrl+C>
   $

Additional arguments can be passed to :file:`bin/minilang`:

:<file> [<arg₁> <arg₂> ...]: Runs the code in ``<file>`` as a script. 
:-G: Opens a GTK+ console if enabled.
:-m <module>: If built with module support, runs ``<module>`` as a module.
:-s <interval>: If built with a scheduler, enables preemptive multitasking every ``<interval>`` instructions.
 
When run with a script, additional command line arguments are passed in a variable called :mini:`Args`.

*Minilang* treats the first line of a script as a comment if it begins with ``#!`` allowing scripts to be made executable on some operating systems.

.. code-block:: mini
   :caption: echo.mini

   #!<path to minilang executable>
   
   print('Args = {Args}\n')

.. code-block:: console

   $ chmod +x echo.mini
   $ ./echo.mini
   Args = []
   $ ./echo.mini Hello world
   Args = [Hello, world]
   $

Embedding
---------

See :doc:`embedding`.
