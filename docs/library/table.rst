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

:mini:`meth (Table: table)[Name: string]` |rarr| :mini:`array`
   Returns the column :mini:`Name` from :mini:`Table`.


:mini:`meth ::(Table: table, Name: string)` |rarr| :mini:`array`
   Returns the column :mini:`Name` from :mini:`Table`.


:mini:`type tablerow < iteratable`
   A row in a table.


:mini:`meth (Table: table)[Row: integer]` |rarr| :mini:`tablerow`
   Returns the :mini:`Row`-th row of :mini:`Table`.


:mini:`meth (Row: tablerow)[Name: string]` |rarr| :mini:`any`
   Returns the value from column :mini:`Name` in :mini:`Row`.


:mini:`meth ::(Row: tablerow, Name: string)` |rarr| :mini:`any`
   Returns the value from column :mini:`Name` in :mini:`Row`.


:mini:`meth string(Arg₁: tablerow)`

