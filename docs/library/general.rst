.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

general
=======

.. rst-class:: mini-api

.. _fun-clock:

:mini:`fun clock()`
   *TBD*


.. _fun-halt:

:mini:`fun halt(Code?: integer)`
   Causes the current process to exit with optional exit code :mini:`Code` or :mini:`0` if omitted.


.. _fun-now:

:mini:`fun now()`
   *TBD*


.. _fun-assign:

:mini:`fun assign(Var: any, Value: any): any`
   Functional equivalent of :mini:`Var := Value`.


.. _fun-call:

:mini:`fun call(Fn: any, Arg₁: any, ..., Argₙ: any): any`
   Returns :mini:`Fn(Arg₁,  ...,  Argₙ)`.


.. _fun-cas:

:mini:`fun cas(Var: any, Old: any, New: any): any`
   If the value of :mini:`Var` is *identically* equal to :mini:`Old`,  then sets :mini:`Var` to :mini:`New` and returns :mini:`New`. Otherwise leaves :mini:`Var` unchanged and returns :mini:`nil`.

   .. code-block:: mini

      var X := 10
      cas(X, 10, 11) :> 11
      X :> 11
      cas(X, 20, 21) :> nil
      X :> 11


.. _fun-copy:

:mini:`fun copy(Value: any, Fn?: function): any`
   Returns a copy of :mini:`Value` using a new :mini:`copy` instance which applies :mini:`Fn(Copy,  Value)` to each value. If omitted,  :mini:`Fn` defaults to :mini:`:copy`.


.. _fun-deref:

:mini:`fun deref(Value: any): any`
   Returns the dereferenced value of :mini:`Value`.


.. _fun-exchange:

:mini:`fun exchange(Var₁: any, ..., Varₙ: any)`
   Assigns :mini:`Varᵢ := Varᵢ₊₁` for each :mini:`1 <= i < n` and :mini:`Varₙ := Var₁`.


.. _fun-findall:

:mini:`fun findall(Value: any, Filter?: boolean|type): list`
   Returns a list of all unique values referenced by :mini:`Value` (including :mini:`Value`).


.. _fun-isconstant:

:mini:`fun isconstant(Value: any): any | nil`
   Returns :mini:`some` if it is a constant (i.e. directly immutable and not referencing any mutable values),  otherwise returns :mini:`nil`.

   .. code-block:: mini

      isconstant(1) :> 1
      isconstant(1.5) :> 1.5
      isconstant("Hello") :> "Hello"
      isconstant(true) :> true
      isconstant([1, 2, 3]) :> nil
      isconstant((1, 2, 3)) :> (1, 2, 3)
      isconstant((1, [2], 3)) :> nil


.. _fun-print:

:mini:`fun print(Values: any, ...): nil`
   Prints :mini:`Values` to standard output,  converting to strings if necessary.


.. _fun-replace:

:mini:`fun replace(Var₁: any, ..., Varₙ: any, Value: any)`
   Assigns :mini:`Varᵢ := Varᵢ₊₁` for each :mini:`1 <= i < n` and :mini:`Varₙ := Value`. Returns the old value of :mini:`Var₁`.


.. _fun-visit:

:mini:`fun visit(Value: any, Fn: function): any`
   Returns :mini:`Fn(V,  Value)` where :mini:`V` is a newly created :mini:`visitor`.


.. _type-gir-function:

:mini:`type gir::function < function`
   *TBD*


.. _type-visitor:

:mini:`type visitor < function`
   Used to apply a transformation recursively to values.
   
   :mini:`fun (V: visitor)(Value: any,  Result: any): any`
      Adds the pair :mini:`(Value,  Result)` to :mini:`V`'s cache and returns :mini:`Result`.
   
   :mini:`fun (V: visitor)(Value: any): any`
      Visits :mini:`Value` with :mini:`V` returning the result.


:mini:`meth (Visitor: visitor):const(Value: any): any`
   Default visitor implementation,  just returns :mini:`Value`.


:mini:`meth (Visitor: visitor):copy(Value: any): any`
   Default visitor implementation,  just returns :mini:`Value`.


:mini:`meth (Visitor: visitor):visit(Value: any): any`
   Default visitor implementation,  just returns :mini:`nil`.


