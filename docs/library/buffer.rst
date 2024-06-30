.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

buffer
======

.. rst-class:: mini-api

:mini:`meth (Source: address):buffer: buffer`
   Allocates a new buffer with the same size and initial contents as :mini:`Source`.

   .. code-block:: mini

      buffer("Hello world") :> <11:48656C6C6F20776F726C64>


:mini:`type buffer < address`
   A buffer represents a writable bounded section of memory.


:mini:`meth (Buffer: buffer):put(Value: address): buffer`
   Puts the bytes of :mini:`Value` in :mini:`Buffer`.

   .. code-block:: mini

      buffer(10):put("Hello\0\0\0\0\0")
      :> <10:48656C6C6F0000000000>


:mini:`meth (Buffer: buffer):put16(Value: integer): buffer`
   Puts :mini:`Value` in :mini:`Buffer` as an 16-bit signed value. Follows the platform byte order.

   .. code-block:: mini

      buffer(2):put16(12345) :> <2:3930>


:mini:`meth (Buffer: buffer):put16b(Value: integer): buffer`
   Puts :mini:`Value` in :mini:`Buffer` as an 16-bit signed value. Uses big endian byte order.

   .. code-block:: mini

      buffer(2):put16b(12345) :> <2:3039>


:mini:`meth (Buffer: buffer):put16l(Value: integer): buffer`
   Puts :mini:`Value` in :mini:`Buffer` as an 16-bit signed value. Uses little endian byte order.

   .. code-block:: mini

      buffer(2):put16l(12345) :> <2:3930>


:mini:`meth (Buffer: buffer):put32(Value: integer): buffer`
   Puts :mini:`Value` in :mini:`Buffer` as an 32-bit signed value. Follows the platform byte order.

   .. code-block:: mini

      buffer(4):put32(12345) :> <4:39300000>


:mini:`meth (Buffer: buffer):put32b(Value: integer): buffer`
   Puts :mini:`Value` in :mini:`Buffer` as an 32-bit signed value. Uses big endian byte order.

   .. code-block:: mini

      buffer(4):put32b(12345) :> <4:00003039>


:mini:`meth (Buffer: buffer):put32l(Value: integer): buffer`
   Puts :mini:`Value` in :mini:`Buffer` as an 32-bit signed value. Uses little endian byte order.

   .. code-block:: mini

      buffer(4):put32l(12345) :> <4:39300000>


:mini:`meth (Buffer: buffer):put64(Value: integer): buffer`
   Puts :mini:`Value` in :mini:`Buffer` as an 64-bit signed value. Follows the platform byte order.

   .. code-block:: mini

      buffer(8):put64(12345) :> <8:3930000000000000>


:mini:`meth (Buffer: buffer):put64b(Value: integer): buffer`
   Puts :mini:`Value` in :mini:`Buffer` as an 64-bit signed value. Uses big endian byte order.

   .. code-block:: mini

      buffer(8):put64b(12345) :> <8:0000000000003039>


:mini:`meth (Buffer: buffer):put64l(Value: integer): buffer`
   Puts :mini:`Value` in :mini:`Buffer` as an 64-bit signed value. Uses little endian byte order.

   .. code-block:: mini

      buffer(8):put64l(12345) :> <8:3930000000000000>


:mini:`meth (Buffer: buffer):put8(Value: integer): buffer`
   Puts :mini:`Value` in :mini:`Buffer` as an 8-bit signed value.

   .. code-block:: mini

      buffer(1):put8(64) :> <1:40>


:mini:`meth (Buffer: buffer):putf32(Value: real): buffer`
   Puts :mini:`Value` in :mini:`Buffer` as a single precision floating point value. Follows the platform endiness.

   .. code-block:: mini

      buffer(4):putf32(1.23456789) :> <4:52069E3F>


:mini:`meth (Buffer: buffer):putf64(Value: real): buffer`
   Puts :mini:`Value` in :mini:`Buffer` as a double precision floating point value. Follows the platform endiness.

   .. code-block:: mini

      buffer(8):putf64(1.23456789) :> <8:1BDE8342CAC0F33F>


:mini:`meth (Buffer: buffer):putu16(Value: integer): buffer`
   Puts :mini:`Value` in :mini:`Buffer` as an 16-bit unsigned value. Follows the platform byte order.

   .. code-block:: mini

      buffer(2):putu16(12345) :> <2:3930>


:mini:`meth (Buffer: buffer):putu16b(Value: integer): buffer`
   Puts :mini:`Value` in :mini:`Buffer` as an 16-bit unsigned value. Uses big endian byte order.

   .. code-block:: mini

      buffer(2):putu16b(12345) :> <2:3039>


:mini:`meth (Buffer: buffer):putu16l(Value: integer): buffer`
   Puts :mini:`Value` in :mini:`Buffer` as an 16-bit unsigned value. Uses little endian byte order.

   .. code-block:: mini

      buffer(2):putu16l(12345) :> <2:3930>


:mini:`meth (Buffer: buffer):putu32(Value: integer): buffer`
   Puts :mini:`Value` in :mini:`Buffer` as an 32-bit unsigned value. Follows the platform byte order.

   .. code-block:: mini

      buffer(4):putu32(12345) :> <4:39300000>


:mini:`meth (Buffer: buffer):putu32b(Value: integer): buffer`
   Puts :mini:`Value` in :mini:`Buffer` as an 32-bit unsigned value. Uses big endian byte order.

   .. code-block:: mini

      buffer(4):putu32b(12345) :> <4:00003039>


:mini:`meth (Buffer: buffer):putu32l(Value: integer): buffer`
   Puts :mini:`Value` in :mini:`Buffer` as an 32-bit unsigned value. Uses little endian byte order.

   .. code-block:: mini

      buffer(4):putu32l(12345) :> <4:39300000>


:mini:`meth (Buffer: buffer):putu64(Value: integer): buffer`
   Puts :mini:`Value` in :mini:`Buffer` as an 64-bit unsigned value. Follows the platform byte order.

   .. code-block:: mini

      buffer(8):putu64(12345) :> <8:3930000000000000>


:mini:`meth (Buffer: buffer):putu64b(Value: integer): buffer`
   Puts :mini:`Value` in :mini:`Buffer` as an 64-bit unsigned value. Uses big endian byte order.

   .. code-block:: mini

      buffer(8):putu64b(12345) :> <8:0000000000003039>


:mini:`meth (Buffer: buffer):putu64l(Value: integer): buffer`
   Puts :mini:`Value` in :mini:`Buffer` as an 64-bit unsigned value. Uses little endian byte order.

   .. code-block:: mini

      buffer(8):putu64l(12345) :> <8:3930000000000000>


:mini:`meth (Buffer: buffer):putu8(Value: integer): buffer`
   Puts :mini:`Value` in :mini:`Buffer` as an 8-bit unsigned value.

   .. code-block:: mini

      buffer(1):put8(64) :> <1:40>


:mini:`meth (Length: integer):buffer: buffer`
   Allocates a new buffer with :mini:`Length` bytes.

   .. code-block:: mini

      buffer(16) :> <16:60000105D17F00004D4C537472696E67>


