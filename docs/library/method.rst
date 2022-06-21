.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

method
======

.. _fun-method-list:

:mini:`fun method::list(): list[method]`
   *TBD*


.. _fun-method-set:

:mini:`fun method::set(Method: any, Types: type, ..., Function: function): Function`
   *TBD*


.. _type-method:

:mini:`type method < function`
   *TBD*


:mini:`meth method(): method`
   Returns a new anonymous method.


:mini:`meth method(Name: string): method`
   Returns the method with name :mini:`Name`.


:mini:`meth (Arg₁: method)[Arg₂: type, ...]`
   *TBD*


:mini:`meth (Arg₁: method):list`
   *TBD*


:mini:`meth (Method: method):name: string`
   Returns the name of :mini:`Method`.


:mini:`meth (Arg₁: string::buffer):append(Arg₂: method)`
   *TBD*


.. _type-method-context:

:mini:`type method::context`
   A context for isolating method definitions.
   
   :mini:`(C: method::context)(Fn: function,  Args,  ...): any`
       Calls :mini:`Fn(Args)` in a new context using :mini:`C` for method definitions.


.. _fun-method-context:

:mini:`fun method::context(): method::context`
   Returns a new context for method definitions. The new context will inherit methods definitions from the current context.


