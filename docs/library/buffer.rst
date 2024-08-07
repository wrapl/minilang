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
   Puts :mini:`Value` in :mini:`Buffer` as an 16-bit signed value. Uses the platform byte order.

   .. code-block:: mini

      buffer(2):put16(12345) :> <2:3930>


:mini:`meth (Buffer: buffer):put16(Value: integer, Order: byte::order): buffer`
   Puts :mini:`Value` in :mini:`Buffer` as an 16-bit signed value. Uses :mini:`Order` byte order.

   .. code-block:: mini

      buffer(2):put16(12345, address::LE) :> <2:3930>
      buffer(2):put16(12345, address::BE) :> <2:3039>


:mini:`meth (Buffer: buffer):put32(Value: integer): buffer`
   Puts :mini:`Value` in :mini:`Buffer` as an 32-bit signed value. Uses the platform byte order.

   .. code-block:: mini

      buffer(4):put32(12345) :> <4:39300000>


:mini:`meth (Buffer: buffer):put32(Value: integer, Order: byte::order): buffer`
   Puts :mini:`Value` in :mini:`Buffer` as an 32-bit signed value. Uses :mini:`Order` byte order.

   .. code-block:: mini

      buffer(4):put32(12345, address::LE) :> <4:39300000>
      buffer(4):put32(12345, address::BE) :> <4:00003039>


:mini:`meth (Buffer: buffer):put64(Value: integer): buffer`
   Puts :mini:`Value` in :mini:`Buffer` as an 64-bit signed value. Uses the platform byte order.

   .. code-block:: mini

      buffer(8):put64(12345) :> <8:3930000000000000>


:mini:`meth (Buffer: buffer):put64(Value: integer, Order: byte::order): buffer`
   Puts :mini:`Value` in :mini:`Buffer` as an 64-bit signed value. Uses :mini:`Order` byte order.

   .. code-block:: mini

      buffer(8):put64(12345, address::LE)
      :> <8:3930000000000000>
      buffer(8):put64(12345, address::BE)
      :> <8:0000000000003039>


:mini:`meth (Buffer: buffer):put8(Value: integer): buffer`
   Puts :mini:`Value` in :mini:`Buffer` as an 8-bit signed value.

   .. code-block:: mini

      buffer(1):put8(64) :> <1:40>


:mini:`meth (Buffer: buffer):putf32(Value: real): buffer`
   Puts :mini:`Value` in :mini:`Buffer` as a 32-bit floating point value. Uses the platform byte order.

   .. code-block:: mini

      buffer(4):putf32(1.23456789) :> <4:52069E3F>


:mini:`meth (Buffer: buffer):putf32(Value: real, Order: byte::order): buffer`
   Puts :mini:`Value` in :mini:`Buffer` as a 32-bit floating point value. Uses little endian byte order.

   .. code-block:: mini

      buffer(4):putf32(1.23456789, address::LE) :> <4:52069E3F>
      buffer(4):putf32(1.23456789, address::BE) :> <4:3F9E0652>


:mini:`meth (Buffer: buffer):putf64(Value: real): buffer`
   Puts :mini:`Value` in :mini:`Buffer` as a 64-bit floating point value. Uses the platform byte order.

   .. code-block:: mini

      buffer(8):putf64(1.23456789) :> <8:1BDE8342CAC0F33F>


:mini:`meth (Buffer: buffer):putf64(Value: real, Order: byte::order): buffer`
   Puts :mini:`Value` in :mini:`Buffer` as a 64-bit floating point value. Uses little endian byte order.

   .. code-block:: mini

      buffer(8):putf64(1.23456789, address::LE)
      :> <8:1BDE8342CAC0F33F>
      buffer(8):putf64(1.23456789, address::BE)
      :> <8:3FF3C0CA4283DE1B>


:mini:`meth (Buffer: buffer):putu16(Value: integer): buffer`
   Puts :mini:`Value` in :mini:`Buffer` as an 16-bit unsigned value. Uses the platform byte order.

   .. code-block:: mini

      buffer(2):putu16(12345) :> <2:3930>


:mini:`meth (Buffer: buffer):putu16(Value: integer, Order: byte::order): buffer`
   Puts :mini:`Value` in :mini:`Buffer` as an 16-bit unsigned value. Uses :mini:`Order` byte order.

   .. code-block:: mini

      buffer(2):putu16(12345, address::LE) :> <2:3930>
      buffer(2):putu16(12345, address::BE) :> <2:3039>


:mini:`meth (Buffer: buffer):putu32(Value: integer): buffer`
   Puts :mini:`Value` in :mini:`Buffer` as an 32-bit unsigned value. Uses the platform byte order.

   .. code-block:: mini

      buffer(4):putu32(12345) :> <4:39300000>


:mini:`meth (Buffer: buffer):putu32(Value: integer, Order: byte::order): buffer`
   Puts :mini:`Value` in :mini:`Buffer` as an 32-bit unsigned value. Uses :mini:`Order` byte order.

   .. code-block:: mini

      buffer(4):putu32(12345, address::LE) :> <4:39300000>
      buffer(4):putu32(12345, address::BE) :> <4:00003039>


:mini:`meth (Buffer: buffer):putu64(Value: integer): buffer`
   Puts :mini:`Value` in :mini:`Buffer` as an 64-bit unsigned value. Uses the platform byte order.

   .. code-block:: mini

      buffer(8):putu64(12345) :> <8:3930000000000000>


:mini:`meth (Buffer: buffer):putu64(Value: integer, Order: byte::order): buffer`
   Puts :mini:`Value` in :mini:`Buffer` as an 64-bit unsigned value. Uses :mini:`Order` byte order.

   .. code-block:: mini

      buffer(8):putu64(12345, address::LE)
      :> <8:3930000000000000>
      buffer(8):putu64(12345, address::BE)
      :> <8:0000000000003039>


:mini:`meth (Buffer: buffer):putu8(Value: integer): buffer`
   Puts :mini:`Value` in :mini:`Buffer` as an 8-bit unsigned value.

   .. code-block:: mini

      buffer(1):put8(64) :> <1:40>


:mini:`meth (Length: integer):buffer: buffer`
   Allocates a new buffer with :mini:`Length` bytes.

   .. code-block:: mini

      buffer(16) :> <16:00895D6E537F000046756E6374696F6E>


