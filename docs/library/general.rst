.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

general
=======

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


.. _fun-deref:

:mini:`fun deref(Value: any): any`
   Returns the dereferenced value of :mini:`Value`.


.. _fun-exchange:

:mini:`fun exchange(Var₁: any, ..., Varₙ: any)`
   Assigns :mini:`Varᵢ := Varᵢ₊₁` for each :mini:`1 <= i < n` and :mini:`Varₙ := Var₁`.


.. _fun-findrefs:

:mini:`fun findrefs(Value: any, RefsOnly?: boolean): list`
   Returns a list of all unique values referenced by :mini:`Value` (including :mini:`Value`).


.. _fun-print:

:mini:`fun print(Values: any, ...): nil`
   Prints :mini:`Values` to standard output,  converting to strings if necessary.


.. _fun-replace:

:mini:`fun replace(Var₁: any, ..., Varₙ: any, Value: any)`
   Assigns :mini:`Varᵢ := Varᵢ₊₁` for each :mini:`1 <= i < n` and :mini:`Varₙ := Value`. Returns the old value of :mini:`Var₁`.


