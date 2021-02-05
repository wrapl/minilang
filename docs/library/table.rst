table
=====

.. include:: <isonum.txt>

:mini:`type table < iteratable`
   A table is a set of named arrays. The arrays must have the same length.


:mini:`meth table()` |rarr| :mini:`table`
   Returns an empty table.


:mini:`meth table(Columns: map)` |rarr| :mini:`table`
   Returns a table with the entries from :mini:`Columns`. The keys of :mini:`Columns` must be strings, the values of :mini:`Columns` are converted to arrays using :mini:`array()` if necessary.


:mini:`meth table(Names: names, Value₁, ..., Valueₙ: any)` |rarr| :mini:`table`
   Returns a table using :mini:`Names` for column names and :mini:`Values` as column values, converted to arrays using :mini:`array()` if necessary.


:mini:`meth :insert(Table: table, Name: string, Value: array)` |rarr| :mini:`table`
   Insert the column :mini:`Name` with values :mini:`Value` into :mini:`Table`.


:mini:`meth :insert(Table: table, Names: names, Value₁, ..., Valueₙ: array)` |rarr| :mini:`table`
   Insert columns with names from :mini:`Names` and values :mini:`Value₁`, ..., :mini:`Valueₙ` into :mini:`Table`.


:mini:`meth :delete(Table: table, Name: string)` |rarr| :mini:`array`
   Remove the column :mini:`Name` from :mini:`Table` and return the value array.


:mini:`meth string(Arg₁: table)`

:mini:`meth (Arg₁: table)[Arg₂: string]`

:mini:`meth ::(Arg₁: table, Arg₂: string)`

:mini:`type tablerow < iteratable`

:mini:`meth (Arg₁: table)[Arg₂: integer]`

:mini:`meth (Arg₁: tablerow)[Arg₂: string]`

:mini:`meth ::(Arg₁: tablerow, Arg₂: string)`

:mini:`meth string(Arg₁: tablerow)`

