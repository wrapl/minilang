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


.. _fun-copy:

:mini:`fun copy(Value: any, Fn?: function): any`
   Creates a deep copy of :mini:`Value`,  calling :mini:`Fn(Copier,  Value)` to copy individual values.
   If omitted,  :mini:`Fn` defaults to :mini:`:copy`.


.. _fun-deref:

:mini:`fun deref(Value: any): any`
   Returns the dereferenced value of :mini:`Value`.


.. _fun-exchange:

:mini:`fun exchange(Var₁: any, ..., Varₙ: any)`
   Assigns :mini:`Varᵢ := Varᵢ₊₁` for each :mini:`1 <= i < n` and :mini:`Varₙ := Var₁`.


.. _fun-findrefs:

:mini:`fun findrefs(Value: any, Filter?: boolean|type): list`
   Returns a list of all unique values referenced by :mini:`Value` (including :mini:`Value`).


.. _fun-isconstant:

:mini:`fun isconstant(Value: any): any | nil`
   Returns :mini:`some` if it is a constant (i.e. directly immutable and not referencing any mutable values),  otherwise returns :mini:`nil`.

   .. code-block:: mini

      isconstant(1) :> some
      isconstant(1.5) :> some
      isconstant("Hello") :> some
      isconstant(true) :> some
      isconstant([1, 2, 3]) :> nil
      isconstant((1, 2, 3)) :> some
      isconstant((1, [2], 3)) :> nil


.. _fun-print:

:mini:`fun print(Values: any, ...): nil`
   Prints :mini:`Values` to standard output,  converting to strings if necessary.


.. _fun-replace:

:mini:`fun replace(Var₁: any, ..., Varₙ: any, Value: any)`
   Assigns :mini:`Varᵢ := Varᵢ₊₁` for each :mini:`1 <= i < n` and :mini:`Varₙ := Value`. Returns the old value of :mini:`Var₁`.


.. _type-copy:

:mini:`type copy < function`
   Used to copy values inside a call to :mini:`copy(Value)`.
   If :mini:`Copy` is an instance of :mini:`copy` then
   
   * :mini:`Copy(X,  Y)` add the mapping :mini:`X -> Y` to :mini:`Copy` and returns :mini:`Y`, 
   * :mini:`Copy(X)` creates a copy of :mini:`X` using the value of :mini:`Fn` passed to :mini:`copy`.

   .. code-block:: mini

      copy([1, {"A" is 2.5}]; Copy, X) do print('Copying {X}\n'); Copy:copy(X) end
      :> [1, {"A" is 2.5}]

   .. code-block:: console

      Copying [1, {A is 2.5}]
      Copying 1
      Copying {A is 2.5}
      Copying A
      Copying 2.5


:mini:`meth (Copy: copy):copy(Value: any): any`
   Default copy implementation,  just returns :mini:`Value`.


