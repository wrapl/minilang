.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

table
=====

.. rst-class:: mini-api

:mini:`meth table(Arg₁: map, ...)`
   *TBD*


:mini:`meth table(Arg₁₁ is Value₁, Arg₂: any, ...)`
   *TBD*


:mini:`meth table(Arg₁₁ is Value₁, Arg₂: type, ...)`
   *TBD*


:mini:`meth (Arg₁: string::buffer):AppendMethod(Arg₂: table)`
   *TBD*


:mini:`type table < sequence`
   A table is a set of named arrays. The arrays must have the same length.


:mini:`meth table()`
   *TBD*


:mini:`meth (Arg₁: table) :: (Arg₂: string)`
   *TBD*


:mini:`meth (Arg₁: table)[Arg₂: integer]`
   *TBD*


:mini:`meth (Arg₁: table)[Arg₂: string]`
   *TBD*


:mini:`meth (Arg₁: table):capacity`
   *TBD*


:mini:`meth (Arg₁: table):columns`
   *TBD*


:mini:`meth (Arg₁: table):delete(Arg₂: string)`
   *TBD*


:mini:`meth (Arg₁: table):insert(Arg₂: integer, Arg₃: list)`
   *TBD*


:mini:`meth (Arg₁: table):insert(Arg₂: integer, Arg₃₁ is Value₁, ...)`
   *TBD*


:mini:`meth (Arg₁: table):insert(Arg₂₁ is Value₁, Arg₃: any, ...)`
   *TBD*


:mini:`meth (Arg₁: table):insert(Arg₂: string, Arg₃: any)`
   *TBD*


:mini:`meth (Arg₁: table):length`
   *TBD*


:mini:`meth (Arg₁: table):offset`
   *TBD*


:mini:`meth (Arg₁: table):permute(Arg₂: permutation)`
   *TBD*


:mini:`meth (Arg₁: table):push(Arg₂: list)`
   *TBD*


:mini:`meth (Arg₁: table):push(Arg₂₁ is Value₁, ...)`
   *TBD*


:mini:`meth (Arg₁: table):put(Arg₂: list)`
   *TBD*


:mini:`meth (Arg₁: table):put(Arg₂₁ is Value₁, ...)`
   *TBD*


:mini:`meth (Table: table):sort(Compare: function): Table`
   Sorts :mini:`Table` in-place using :mini:`Compare` and returns it.


:mini:`meth (Table: table):sort(By: function, Order: function): Table`
   Sorts :mini:`Table` in-place using :mini:`Order(By(Rowᵢ),  By(Rowⱼ))` as the comparison function (evaluating :mini:`By(Rowᵢ)` only once for each :mini:`i`).


:mini:`type table::column`
   *TBD*


:mini:`type table::row`
   *TBD*


:mini:`meth (Arg₁: table::row) :: (Arg₂: string)`
   *TBD*


:mini:`meth (Arg₁: table::row)[Arg₂: string]`
   *TBD*


:mini:`meth (Buffer: string::buffer):append(Value: table::row)`
   Appends a representation of :mini:`Value` to :mini:`Buffer`.


