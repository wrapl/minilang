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

      buffer(16) :> <16:4052B8A5B47F00006C656D656E74206E>


:mini:`meth (Buffer: buffer) + (Offset: integer): buffer`
   Returns the buffer at offset :mini:`Offset` from :mini:`Address`.

   .. code-block:: mini

      let B := buffer(16)
      :> <16:A02A4EADB47F000065725F74202A4E65>
      B + 8 :> <8:65725F74202A4E65>


:mini:`meth (Buffer: buffer) @ (Length: integer): buffer`
   Returns the same buffer as :mini:`Buffer`,  limited to :mini:`Length` bytes.

   .. code-block:: mini

      let B := buffer(16)
      :> <16:2050B8A5B47F000062757465733F3A6E>
      B @ 8 :> <8:2050B8A5B47F0000>


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


