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


:mini:`type table < sequence`
   A table is a set of named arrays. The arrays must have the same length.


:mini:`meth table(): table`
   Returns an empty table.


:mini:`meth table(Names: list, Rows: list): table`
   Returns a table using :mini:`Names` for column names and :mini:`Rows` as rows,  where each row in :mini:`Rows` is a list of values corresponding to :mini:`Names`.


:mini:`meth table(Columns: map): table`
   Returns a table with the entries from :mini:`Columns`. The keys of :mini:`Columns` must be strings,  the values of :mini:`Columns` are converted to arrays using :mini:`array()` if necessary.


:mini:`meth table(Names₁ is Value₁, Value₁,  : any, ...): table`
   Returns a table using :mini:`Names` for column names and :mini:`Values` as column values,  converted to arrays using :mini:`array()` if necessary.


:mini:`meth table()`
   *TBD*


:mini:`meth (Arg₁: table) :: (Arg₂: string)`
   *TBD*


:mini:`meth (Table: table) :: (Name: string, ...): array`
   Returns the column :mini:`Name` from :mini:`Table`.


:mini:`meth (Arg₁: table)[Arg₂: integer]`
   *TBD*


:mini:`meth (Table: table)[Row: integer]: tablerow`
   Returns the :mini:`Row`-th row of :mini:`Table`.


:mini:`meth (Table: table)[Name: string, ...]: array`
   Returns the column :mini:`Name` from :mini:`Table`.


:mini:`meth (Arg₁: table)[Arg₂: string]`
   *TBD*


:mini:`meth (Arg₁: table):capacity`
   *TBD*


:mini:`meth (Arg₁: table):columns`
   *TBD*


:mini:`meth (Table: table):delete(Name: string): array`
   Remove the column :mini:`Name` from :mini:`Table` and return the value array.


:mini:`meth (Arg₁: table):delete(Arg₂: string)`
   *TBD*


:mini:`meth (Arg₁: table):insert(Arg₂: integer, Arg₃₁ is Value₁, ...)`
   *TBD*


:mini:`meth (Arg₁: table):insert(Arg₂₁ is Value₁, Arg₃: array, ...)`
   *TBD*


:mini:`meth (Table: table):insert(Names₁ is Value₁, Value₁,  : array, ...): table`
   Insert columns with names from :mini:`Names` and values :mini:`Value₁`,  ...,  :mini:`Valueₙ` into :mini:`Table`.


:mini:`meth (Table: table):insert(Name: string, Value: array): table`
   Insert the column :mini:`Name` with values :mini:`Value` into :mini:`Table`.


:mini:`meth (Arg₁: table):insert(Arg₂: string, Arg₃: array)`
   *TBD*


:mini:`meth (Arg₁: table):length`
   *TBD*


:mini:`meth (Arg₁: table):offset`
   *TBD*


:mini:`meth (Arg₁: table):push(Arg₂₁ is Value₁, ...)`
   *TBD*


:mini:`meth (Arg₁: table):put(Arg₂₁ is Value₁, ...)`
   *TBD*


:mini:`meth (Buffer: string::buffer):append(Value: table)`
   Appends a representation of :mini:`Value` to :mini:`Buffer`.


:mini:`type table::column`
   *TBD*


:mini:`type table::row < sequence`
   A row in a table.


:mini:`type table::row`
   *TBD*


:mini:`meth (Row: table::row) :: (Name: string): any`
   Returns the value from column :mini:`Name` in :mini:`Row`.


:mini:`meth (Arg₁: table::row) :: (Arg₂: string)`
   *TBD*


:mini:`meth (Arg₁: table::row)[Arg₂: string]`
   *TBD*


:mini:`meth (Row: table::row)[Name: string]: any`
   Returns the value from column :mini:`Name` in :mini:`Row`.


:mini:`meth (Buffer: string::buffer):append(Value: table::row)`
   Appends a representation of :mini:`Value` to :mini:`Buffer`.


:mini:`meth (Buffer: string::buffer):append(Value: table::row)`
   Appends a representation of :mini:`Value` to :mini:`Buffer`.


