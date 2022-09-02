.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

context
=======

.. rst-class:: mini-api

.. _type-context:

:mini:`type context < function`
   A context key can be used to create context specific values.
   If :mini:`key` is a context key,  then calling :mini:`key()` no arguments returns the value associated with the key in the current context,  or :mini:`nil` is no value is associated.
   Calling :mini:`key(Value,  Function)` will invoke :mini:`Function` in a new context where :mini:`key` is associated with :mini:`Value`.


.. _fun-context:

:mini:`fun context(): context`
   Creates a new context specific key.


