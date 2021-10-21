Minilang
========

Introduction
------------

*Minilang* is a small imperative language, designed to be embedded into programs
written in *C*. Although initially developed for *Rabs* `<https://rabs.readthedocs.io>`_ to specify build functions and dependencies, it has evolved into a comprehensive embeddable scripting language. It has the following goals / features:

**Minimal dependencies**
   *Minilang* has no mandatory dependencies other than the Hans Boehm garbage
   collector and standard *C* libraries.

**Minimal language**
   *Minilang* is a fairly simple language with no syntax for classes, modules, etc. This allows it to be embedded in applications with strict control on available features. There is no loss in expression though as the syntax allows classes and modules to be added seamlessly.

**Expression based**
  Other than declarations, everything else in *Minilang* is an expression, avoiding the need for temporary variables for intermediate values resulting in concise code. Of course, variables can be defined for clarity as required.

**One-shot Continuations**
   All function calls in *Minilang* are implemented using one-shot continuations. This allows asynchronous functions and preemptive multitasking to be added if required, without imposing them when they are not needed.

**Flexible API**
   The *Minilang* compiler API uses callbacks where possible giving applications complete flexibility on how to use *Minilang*.

**Easy to embed & extend**
   *Minilang* provides a comprehensive embedding API to support a wider range of use cases. It is easy to create new functions in *C* to use in *Minilang*.

**Full closures**
   Functions in *Minilang* automatically capture their environment, creating
   closures at runtime. Closures can be passed around as first-class values.


Sample
------

.. code-block:: mini

   print("Hello world!\n")
   
   var L := [1, 2, 3, 4, 5]
   
   for X in L do
      print('X = {X}\n')
   end

Details
=======

.. toctree::
   :maxdepth: 1
   
   /start
   /language
   /library
   /features
   /embedding
   /extending
   /api
   /internals   
