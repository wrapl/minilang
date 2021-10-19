address
=======

:mini:`type address`
   An address represents a read-only bounded section of memory.


:mini:`meth :count(Address: address): integer`
   Returns the number bytes visible at :mini:`Address`.


:mini:`meth (Address: address) @ (Length: integer): address`
   Returns the same address as :mini:`Address`, limited to :mini:`Length` bytes.


:mini:`meth (Address: address) + (Offset: integer): address`
   Returns the address at offset :mini:`Offset` from :mini:`Address`.


:mini:`meth (Address₁: address) - (Address₂: address): integer`
   Returns the offset from :mini:`Address₂` to :mini:`Address₁`, provided :mini:`Address₂` is visible to :mini:`Address₁`.


:mini:`meth :get8(Address: address): integer`
   *TBD*

:mini:`meth :get16(Address: address): integer`
   *TBD*

:mini:`meth :get32(Address: address): integer`
   *TBD*

:mini:`meth :get64(Address: address): integer`
   *TBD*

:mini:`meth :getf32(Address: address): real`
   *TBD*

:mini:`meth :getf64(Address: address): real`
   *TBD*

:mini:`meth :gets(Address: address): string`
   *TBD*

:mini:`meth :gets(Address: address, Size: integer): string`
   *TBD*

:mini:`meth string(Arg₁: address)`
   *TBD*

