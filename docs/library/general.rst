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
   


.. _fun-now:

:mini:`fun now()`
   *TBD*


.. _fun-cas:

:mini:`fun cas(Var: any, Old: any, New: any): any`
   If the value of :mini:`Var` is identically equal to :mini:`Old`,  then sets :mini:`Var` to :mini:`New` and returns :mini:`New`. Otherwise leaves :mini:`Var` unchanged and returns :mini:`nil`.

   .. code-block:: mini

      var X := 10
      with Old := X do cas(X, Old, Old + 1) end :> 11
      X :> 11


.. _fun-exchange:

:mini:`fun exchange(Var₁, : any, ...)`
   Assigns :mini:`Varᵢ := Varᵢ₊₁` for each :mini:`1 <= i < n` and :mini:`Varₙ := Var₁`.


.. _fun-print:

:mini:`fun print(Values..: any): nil`
   Prints :mini:`Values` to standard output,  converting to strings if necessary.


.. _fun-replace:

:mini:`fun replace(Var₁, : any, ..., Value: any)`
   Assigns :mini:`Varᵢ := Varᵢ₊₁` for each :mini:`1 <= i < n` and :mini:`Varₙ := Value`. Returns the old value of :mini:`Var₁`.


