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

A number of build options can be defined when invoking *rabs* to enable some additional *Minilang* features:

:-DMODULES: Adds support for loading modules at runtime. On *Linux*, this includes support for shared object (:file:`.so`) files.
:-DUSEMATH: Adds several maths functions and multiple dimensional numeric arrays to the library.
:-DDEBUGGER: Adds debugging functions to the REPL console.
:-DUSEGTK: Adds *gobject-introspection* functions to the library and adds a GTK+ console to the interpreter.
:-DUSECBOR: Adds *CBOR* functions to the library.

Using *Make*
~~~~~~~~~~~~

.. code-block:: console

   $ git clone https://github.com/wrapl/minilang
   $ cd minilang
   $ make

