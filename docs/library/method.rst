.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

method
======

.. rst-class:: mini-api

:mini:`fun method::list(): list[method]`
   *TBD*


:mini:`fun method::isolate(Args: any, ..., F: :function): any`
   Evaluates :mini:`Fn(Args)` in a new method context and returns the result.


:mini:`type method < (MLFunction`
   A map of type signatures to functions. Each type signature consists of a number of types and a flag denoting whether the signature is variadic.
   
   :mini:`(M: method)(Arg₁,  ...,  Argₙ)`
      Calls :mini:`Fn(Arg₁,  ...,  Argₙ)` where :mini:`Fn` is the function associated with the closest matching type signature defined in :mini:`M`.
   
      A type signature :mini:`(Type₁,  ...,  Type/k,  Variadic)` matches if :mini:`type(Argᵢ) < Typeᵢ` for each :math:`i = 1,  ...,  k` and either :math:`n = k` or :math:`n < k` and :math:`Variadic` is true.
   
      * A type signature is considered a closer match if its types are closer in terms of subtyping to the types of the arguments.
      * A type signature with the same number of types as arguments is considered a closer match than a matching variadic signature with fewer types.


:mini:`meth method(Name: string): method`
   Returns the method with name :mini:`Name`.


:mini:`meth method(): method`
   Returns a new anonymous method.


:mini:`meth (Arg₁: method):MLMethodDefault(...)`
   *TBD*


:mini:`meth (Arg₁: method)[...]`
   *TBD*


:mini:`meth (Arg₁: method):list`
   *TBD*


:mini:`meth method::define(Method: method, Types..: :type, ..?: any, Functio: :function, ...): Function`
   Adds a new type signature and associated function to :mini:`Method`. If the last argument is :mini:`..` then the signature is variadic. Method definitions using :mini:`meth` are translated into calls to :mini:`method::set`.


:mini:`meth (Method: method):name: string`
   Returns the name of :mini:`Method`.


:mini:`meth (Arg₁: string::buffer):append(Arg₂: method)`
   *TBD*


:mini:`meth (Arg₁: string::buffer):append(Arg₂: method::anon)`
   *TBD*


:mini:`type method::context`
   A context for isolating method definitions.
   
   :mini:`(C: method::context)(Args: any,  ...,  Fn: function): any`
       Calls :mini:`Fn(Args)` in a new context using :mini:`C` for method definitions.


:mini:`fun method::context(): method::context`
   Returns a new context for method definitions. The new context will inherit methods definitions from the current context.


