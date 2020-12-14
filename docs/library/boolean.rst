boolean
=======

.. include:: <isonum.txt>

:mini:`meth string(Arg₁: boolean)`

:mini:`def boolean < function`

:mini:`meth boolean(String: string)` |rarr| :mini:`boolean` or :mini:`error`
   Returns :mini:`true` if :mini:`String` equals :mini:`"true"` (ignoring case).

   Returns :mini:`false` if :mini:`String` equals :mini:`"false"` (ignoring case).

   Otherwise returns an error.


:mini:`meth -(Bool: boolean)` |rarr| :mini:`boolean`
   Returns the logical inverse of :mini:`Bool`


:mini:`meth /\\(Bool₁: boolean, Bool₂: boolean)` |rarr| :mini:`boolean`
   Returns the logical and of :mini:`Bool₁` and :mini:`Bool₂`.


:mini:`meth \\/(Bool₁: boolean, Bool₂: boolean)` |rarr| :mini:`boolean`
   Returns the logical or of :mini:`Bool₁` and :mini:`Bool₂`.


:mini:`meth <>(Bool₁: boolean, Bool₂: boolean)` |rarr| :mini:`integer`
   Returns :mini:`-1`, :mini:`0` or :mini:`1` depending on whether :mini:`Bool₁` is less than, equal to or greater than :mini:`Bool₂`. :mini:`true` is considered greater than :mini:`false`.


:mini:`meth <op>(Bool₁: boolean, Bool₂: boolean)` |rarr| :mini:`Bool₂` or :mini:`nil`
   :mini:`<op>` is :mini:`=`, :mini:`!=`, :mini:`<`, :mini:`<=`, :mini:`>` or :mini:`>=`

   Returns :mini:`Bool₂` if :mini:`Bool₂ <op> Bool₁` is true, otherwise returns :mini:`nil`.

   :mini:`true` is considered greater than :mini:`false`.


