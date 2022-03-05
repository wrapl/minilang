.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

buffer
======

.. _type-buffer:

:mini:`type buffer < address`
   A buffer represents a writable bounded section of memory.



.. _fun-buffer:

:mini:`fun buffer(Length: integer): buffer`
   Allocates a new buffer with :mini:`Length` bytes.



:mini:`meth (Buffer: buffer) + (Offset: integer): buffer`
   Returns the buffer at offset :mini:`Offset` from :mini:`Address`.



:mini:`meth (Buffer: buffer):put(Value: address): buffer`
   Puts the bytes of :mini:`Value` in :mini:`Buffer`.



:mini:`meth (Buffer: buffer):put16(Value: integer): buffer`
   Puts :mini:`Value` in :mini:`Buffer` as an 16-bit signed value. Currently follows the platform endiness.



:mini:`meth (Buffer: buffer):put32(Value: integer): buffer`
   Puts :mini:`Value` in :mini:`Buffer` as an 32-bit signed value. Currently follows the platform endiness.



:mini:`meth (Buffer: buffer):put32f(Value: real): buffer`
   Puts :mini:`Value` in :mini:`Buffer` as a single precision floating point value. Currently follows the platform endiness.



:mini:`meth (Buffer: buffer):put64(Value: integer): buffer`
   Puts :mini:`Value` in :mini:`Buffer` as an 64-bit signed value. Currently follows the platform endiness.



:mini:`meth (Buffer: buffer):put64f(Value: real): buffer`
   Puts :mini:`Value` in :mini:`Buffer` as a double precision floating point value. Currently follows the platform endiness.



:mini:`meth (Buffer: buffer):put8(Value: integer): buffer`
   Puts :mini:`Value` in :mini:`Buffer` as an 8-bit signed value.



:mini:`meth (Buffer: buffer) @ (Length: integer): buffer`
   Returns the same buffer as :mini:`Buffer`,  limited to :mini:`Length` bytes.



