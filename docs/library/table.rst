.. include:: <isonum.txt>

table
=====

:mini:`type table < sequence`
   A table is a set of named arrays. The arrays must have the same length.


:mini:`meth table(): table`
   Returns an empty table.


:mini:`meth table(Columns: map): table`
   Returns a table with the entries from :mini:`Columns`. The keys of :mini:`Columns` must be strings,  the values of :mini:`Columns` are converted to arrays using :mini:`array()` if necessary.


:mini:`meth table(Names₁ is Value₁, Value₁,  ...,  Valueₙ: any): table`
   Returns a table using :mini:`Names` for column names and :mini:`Values` as column values,  converted to arrays using :mini:`array()` if necessary.


:mini:`meth (Table: table):insert(Name: string, Value: array): table`
   Insert the column :mini:`Name` with values :mini:`Value` into :mini:`Table`.


:mini:`meth (Table: table):insert(Names₁ is Value₁, Value₁,  ...,  Valueₙ: array): table`
   Insert columns with names from :mini:`Names` and values :mini:`Value₁`,  ...,  :mini:`Valueₙ` into :mini:`Table`.


:mini:`meth (Table: table):delete(Name: string): array`
   Remove the column :mini:`Name` from :mini:`Table` and return the value array.


:mini:`meth (Arg₁: string::buffer):append(Arg₂: table)`
   *TBD*

:mini:`meth (Table: table)[Name: string, ...]: array`
   Returns the column :mini:`Name` from :mini:`Table`.


:mini:`meth (Table: table) :: (Name: string, ...): array`
   Returns the column :mini:`Name` from :mini:`Table`.


:mini:`type table::row < sequence`
   A row in a table.


:mini:`meth (Table: table)[Row: integer]: tablerow`
   Returns the :mini:`Row`-th row of :mini:`Table`.


:mini:`meth (Row: table::row)[Name: string]: any`
   Returns the value from column :mini:`Name` in :mini:`Row`.


:mini:`meth (Row: table::row) :: (Name: string): any`
   Returns the value from column :mini:`Name` in :mini:`Row`.


:mini:`meth (Arg₁: string::buffer):append(Arg₂: table::row)`
   *TBD*

