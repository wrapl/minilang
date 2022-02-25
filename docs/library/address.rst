.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

address
=======

.. _type-address:

:mini:`type address`
   An address represents a read-only bounded section of memory.


:mini:`meth (Address: address):size: integer`
   Returns the number of bytes visible at :mini:`Address`.


:mini:`meth (Address: address):length: integer`
   Returns the number of bytes visible at :mini:`Address`.


:mini:`meth (Address: address) @ (Length: integer): address`
   Returns the same address as :mini:`Address`,  limited to :mini:`Length` bytes.


:mini:`meth (Address: address) + (Offset: integer): address`
   Returns the address at offset :mini:`Offset` from :mini:`Address`.


:mini:`meth (Address₁: address) - (Address₂: address): integer`
   Returns the offset from :mini:`Address₂` to :mini:`Address₁`,  provided :mini:`Address₂` is visible to :mini:`Address₁`.


:mini:`meth (Address: address):get8: integer`
   Returns the signed 8-bit value at :mini:`Address`.


:mini:`meth (Address: address):get16: integer`
   Returns the signed 16-bit value at :mini:`Address`. Currently follows the platform endiness.


:mini:`meth (Address: address):get32: integer`
   Returns the signed 32-bit value at :mini:`Address`. Currently follows the platform endiness.


:mini:`meth (Address: address):get64: integer`
   Returns the signed 64-bit value at :mini:`Address`. Currently follows the platform endiness.


:mini:`meth (Address: address):getu8: integer`
   Returns the unsigned 8-bit value at :mini:`Address`.


:mini:`meth (Address: address):getu16: integer`
   Returns the unsigned 16-bit value at :mini:`Address`. Currently follows the platform endiness.


:mini:`meth (Address: address):getu32: integer`
   Returns the unsigned 32-bit value at :mini:`Address`. Currently follows the platform endiness.


:mini:`meth (Address: address):getu64: integer`
   Returns the unsigned 64-bit value at :mini:`Address`. Currently follows the platform endiness.

   .. warning::

      Minilang currently uses signed 64-bit integers so this method will produce incorrect results if the actual value is too large to fit. This may change in future implementations or if arbitrary precision integers are added to the runtime.


:mini:`meth (Address: address):getf32: real`
   Returns the single precision floating point value at :mini:`Address`. Currently follows the platform endiness.


:mini:`meth (Address: address):getf64: real`
   Returns the double precision floating point value at :mini:`Address`. Currently follows the platform endiness.


:mini:`meth (Address: address):gets: string`
   Returns the string consisting of the bytes at :mini:`Address`.


:mini:`meth (Address: address):gets(Size: integer): string`
   Returns the string consisting of the first :mini:`Size` bytes at :mini:`Address`.


:mini:`meth (Buffer: string::buffer):append(Value: address)`
   Appends a string representation of :mini:`Value` to :mini:`Buffer`.


