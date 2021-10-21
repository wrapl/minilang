Methods
=======

All methods in *Minilang* are multimethods, that is they use the runtime types of all arguments to determine which function to call.

Methods are written as :mini:`:name` (or :mini:`:"name"` if *name* contains non-alphanumeric characters). Methods also act as *atoms* or *symbols*, two methods with the same name are identically equal. Infix operators such as :mini:`+`, :mini:`*`, etc, are equivalent to methods with the same name (:mini:`:"+"`, :mini:`:"*"`, etc).

If no suitable method is found for a specific combination of arguments, an error is raised. Methods can be defined using the :mini:`meth` keyword:

.. code-block:: mini

   print('5 * \"word\" = {5 * "word"}\n')

.. code-block:: console

   MethodError: no method found for *(int32, string)
      <console>:1

.. code-block:: mini

   meth *(Count: integer, String: string) sum(1 .. Count;) String
   
   print('5 * \"word\" = {5 * "word"}\n')

.. code-block:: console

   5 * "word" = wordwordwordwordword

