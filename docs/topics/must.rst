Must
====

A :mini:`must`-declaration can be used to ensure that particular code is guaranteed to run before a block is finished, even if an error occurs or :mini:`ret`, :mini:`exit`, :mini:`while`, :mini:`until` or :mini:`next` is used to exit the block early.

.. parser-rule-diagram:: 'must' expression

.. note::

   Since :mini:`susp`-expressions can resume the current block, they do not run :mini:`must` code. In general, do not use :mini:`must`-declarations with :mini:`susp`-expressions unless it known that the generating function will always be completed.

.. note::

   In order to guarantee a valid state when each :mini:`must`-expression is executed, each :mini:`must`-declaration implicitly creates a new block around the code that follows it. Given code like:

   .. code-block:: mini
      :linenos:

      decls
      code
      must X
      decls
      code

   the compiler treats it internally as similar to:

   .. code-block:: mini
      :linenos:

      decls
      code
      do
         decls
         code
         X
      on Error do
         X
         Error:raise
      end

   This means that some forward declarations that work without :mini:`must` may not work when :mini:`must` is present. This limitation might be removed in the future.
