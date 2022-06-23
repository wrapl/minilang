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

   .. code-block:: mini

      buffer(16) :> <16:E0A25C74BF7F00006368616E6E656C0A>


:mini:`meth (Buffer: buffer) + (Offset: integer): buffer`
   Returns the buffer at offset :mini:`Offset` from :mini:`Address`.

   .. code-block:: mini

      let B := buffer(16)
      :> <16:603EC06CBF7F00006F72292072657475>
      B + 8 :> <8:6F72292072657475>


:mini:`meth (Buffer: buffer) @ (Length: integer): buffer`
   Returns the same buffer as :mini:`Buffer`,  limited to :mini:`Length` bytes.

   .. code-block:: mini

      let B := buffer(16)
      :> <16:00A05C74BF7F00007970652074687265>
      B @ 8 :> <8:00A05C74BF7F0000>


:mini:`meth (Buffer: buffer):put(Value: address): buffer`
   Puts the bytes of :mini:`Value` in :mini:`Buffer`.

   .. code-block:: mini

      buffer(10):put("Hello\0\0\0\0\0")
      :> <10:48656C6C6F0000000000>


:mini:`meth (Buffer: buffer):put16(Value: integer): buffer`
   Puts :mini:`Value` in :mini:`Buffer` as an 16-bit signed value. Currently follows the platform endiness.

   .. code-block:: mini

      buffer(2):put16(12345) :> <2:3930>


:mini:`meth (Buffer: buffer):put32(Value: integer): buffer`
   Puts :mini:`Value` in :mini:`Buffer` as an 32-bit signed value. Currently follows the platform endiness.

   .. code-block:: mini

      buffer(4):put32(12345678) :> <4:4E61BC00>


:mini:`meth (Buffer: buffer):put64(Value: integer): buffer`
   Puts :mini:`Value` in :mini:`Buffer` as an 64-bit signed value. Currently follows the platform endiness.

   .. code-block:: mini

      buffer(8):put64(123456789123) :> <8:831A99BE1C000000>


:mini:`meth (Buffer: buffer):put8(Value: integer): buffer`
   Puts :mini:`Value` in :mini:`Buffer` as an 8-bit signed value.

   .. code-block:: mini

      buffer(1):put8(64) :> <1:40>


:mini:`meth (Buffer: buffer):putf32(Value: real): buffer`
   Puts :mini:`Value` in :mini:`Buffer` as a single precision floating point value. Currently follows the platform endiness.

   .. code-block:: mini

      buffer(4):putf32(1.23456789) :> <4:52069E3F>


:mini:`meth (Buffer: buffer):putf64(Value: real): buffer`
   Puts :mini:`Value` in :mini:`Buffer` as a double precision floating point value. Currently follows the platform endiness.

   .. code-block:: mini

      buffer(8):putf64(1.23456789) :> <8:1BDE8342CAC0F33F>


