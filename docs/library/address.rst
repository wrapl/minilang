address
=======

:mini:`type address`
   An address represents a read-only bounded section of memory.


:mini:`meth (Address: address):count: integer`
   Returns the number bytes visible at :mini:`Address`.


:mini:`meth (Address: address) @ (Length: integer): address`
   Returns the same address as :mini:`Address`,  limited to :mini:`Length` bytes.


:mini:`meth (Address: address) + (Offset: integer): address`
   Returns the address at offset :mini:`Offset` from :mini:`Address`.


:mini:`meth (Address₁: address) - (Address₂: address): integer`
   Returns the offset from :mini:`Address₂` to :mini:`Address₁`,  provided :mini:`Address₂` is visible to :mini:`Address₁`.


:mini:`meth (Address: address):get8: integer`
   *TBD*

:mini:`meth (Address: address):get16: integer`
   *TBD*

:mini:`meth (Address: address):get32: integer`
   *TBD*

:mini:`meth (Address: address):get64: integer`
   *TBD*

:mini:`meth (Address: address):getu8: integer`
   *TBD*

:mini:`meth (Address: address):getu16: integer`
   *TBD*

:mini:`meth (Address: address):getu32: integer`
   *TBD*

:mini:`meth (Address: address):getu64: integer`
   *TBD*

:mini:`meth (Address: address):getf32: real`
   *TBD*

:mini:`meth (Address: address):getf64: real`
   *TBD*

:mini:`meth (Address: address):gets: string`
   *TBD*

:mini:`meth (Address: address):gets(Size: integer): string`
   *TBD*

:mini:`meth (Arg₁: string::buffer):append(Arg₂: address)`
   *TBD*

