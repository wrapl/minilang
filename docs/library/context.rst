context
=======

.. include:: <isonum.txt>

:mini:`fun context()` |rarr| :mini:`context`
   Creates a new context specific key.

   *Defined at line 95 in src/ml_runtime.c*

:mini:`context`
   A context key can be used to create context specific values.

   If :mini:`key` is a context key, then calling :mini:`key()` no arguments returns the value associated with the key in the current context, or :mini:`nil` is no value is associated.

   Calling :mini:`key(Value, Function)` will invoke :mini:`Function` in a new context where :mini:`key` is associated with :mini:`Value`.

   :Parents: :mini:`cfunction`

   *Defined at line 105 in src/ml_runtime.c*

