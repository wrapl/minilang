Minilang
========

Introduction
------------

*Minilang* is a small imperative language, designed to be embedded into programs
written in *C*. It has the following goals / features:

**Minimal dependencies**
   *Minilang* has no dependencies other than the Hans Boehm garbage
   collector, and standard *C* libraries.

**Full Closures**
   Functions in *Minilang* automatically capture their environment, creating
   closures at runtime. Closures can be passed around as first-class values,
   which are used to build targets in *Rabs*. 

**Dynamic scoping**
   The *Minilang* interpreter provides a callback for identifier resolution,
   allowing programs to provide dynamic scoping. This is used within *Rabs* to
   provide dynamic symbol resolution.

**Easy to embed & extend**
   *Minilang* provides a comprehensive embedding API to support a wider range of use cases. It is easy to create new functions in *C* to use in *Minilang*.

Sample
------

.. code-block:: mini

   var L := [1, 2, 3, 4, 5]
 
   for X in L do
      print('X = {X}\n')
   end

Details
=======

.. toctree::
   :maxdepth: 2
   
   /start
   /language
   /types
   /grammar
   /embedding
   /api
   /internals

.. toctree::
   :maxdepth: 1
   
   /library
