.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

address
=======

.. rst-class:: mini-api

:mini:`type address`
   An address represents a read-only bounded section of memory.


:mini:`meth address(String: string): address`
   Returns an address view of :mini:`String`.

   .. code-block:: mini

      address("Hello world!\n")
      :> <13:48656C6C6F20776F726C64210A>


:mini:`meth (Arg₁: address) != (Arg₂: address): address | nil`
   Returns :mini:`Arg₂` if the bytes at :mini:`Arg₁` != the bytes at :mini:`Arg₂` and :mini:`nil` otherwise.

   .. code-block:: mini

      "Hello" != "World" :> "World"
      "World" != "Hello" :> "Hello"
      "Hello" != "Hello" :> nil
      "abcd" != "abc" :> "abc"
      "abc" != "abcd" :> "abcd"


:mini:`meth (Address: address) + (Offset: integer): address`
   Returns the address at offset :mini:`Offset` from :mini:`Address`.

   .. code-block:: mini

      let A := address("Hello world!\n")
      :> <13:48656C6C6F20776F726C64210A>
      A + 4 :> <9:6F20776F726C64210A>


:mini:`meth (Address₁: address) - (Address₂: address): integer`
   Returns the offset from :mini:`Address₂` to :mini:`Address₁`,  provided :mini:`Address₂` is visible to :mini:`Address₁`.

   .. code-block:: mini

      let A := address("Hello world!\n")
      let B := A + 4
      B - A :> 4
      address("world!\n") - A
      :> error("ValueError", "Addresses are not from same base")


:mini:`meth (Arg₁: address) < (Arg₂: address): address | nil`
   Returns :mini:`Arg₂` if the bytes at :mini:`Arg₁` < the bytes at :mini:`Arg₂` and :mini:`nil` otherwise.

   .. code-block:: mini

      "Hello" < "World" :> "World"
      "World" < "Hello" :> nil
      "Hello" < "Hello" :> nil
      "abcd" < "abc" :> nil
      "abc" < "abcd" :> "abcd"


:mini:`meth (Arg₁: address) <= (Arg₂: address): address | nil`
   Returns :mini:`Arg₂` if the bytes at :mini:`Arg₁` <= the bytes at :mini:`Arg₂` and :mini:`nil` otherwise.

   .. code-block:: mini

      "Hello" <= "World" :> "World"
      "World" <= "Hello" :> nil
      "Hello" <= "Hello" :> "Hello"
      "abcd" <= "abc" :> nil
      "abc" <= "abcd" :> "abcd"


:mini:`meth (A: address) <> (B: address): integer`
   Compares the bytes at :mini:`A` and :mini:`B` lexicographically and returns :mini:`-1`,  :mini:`0` or :mini:`1` respectively.

   .. code-block:: mini

      "Hello" <> "World" :> -1
      "World" <> "Hello" :> 1
      "Hello" <> "Hello" :> 0
      "abcd" <> "abc" :> 1
      "abc" <> "abcd" :> -1


:mini:`meth (Arg₁: address) = (Arg₂: address): address | nil`
   Returns :mini:`Arg₂` if the bytes at :mini:`Arg₁` = the bytes at :mini:`Arg₂` and :mini:`nil` otherwise.

   .. code-block:: mini

      "Hello" = "World" :> nil
      "World" = "Hello" :> nil
      "Hello" = "Hello" :> "Hello"
      "abcd" = "abc" :> nil
      "abc" = "abcd" :> nil


:mini:`meth (Arg₁: address) > (Arg₂: address): address | nil`
   Returns :mini:`Arg₂` if the bytes at :mini:`Arg₁` > the bytes at :mini:`Arg₂` and :mini:`nil` otherwise.

   .. code-block:: mini

      "Hello" > "World" :> nil
      "World" > "Hello" :> "Hello"
      "Hello" > "Hello" :> nil
      "abcd" > "abc" :> "abc"
      "abc" > "abcd" :> nil


:mini:`meth (Arg₁: address) >= (Arg₂: address): address | nil`
   Returns :mini:`Arg₂` if the bytes at :mini:`Arg₁` >= the bytes at :mini:`Arg₂` and :mini:`nil` otherwise.

   .. code-block:: mini

      "Hello" >= "World" :> nil
      "World" >= "Hello" :> "Hello"
      "Hello" >= "Hello" :> "Hello"
      "abcd" >= "abc" :> "abc"
      "abc" >= "abcd" :> nil


:mini:`meth (Address: address) @ (Length: integer): address`
   Returns the same address as :mini:`Address`,  limited to :mini:`Length` bytes.

   .. code-block:: mini

      let A := address("Hello world!\n")
      :> <13:48656C6C6F20776F726C64210A>
      A @ 5 :> <5:48656C6C6F>


:mini:`meth (Address: address) @ (Offset: integer, Length: integer): address`
   Returns the address at offset :mini:`Offset` from :mini:`Address` limited to :mini:`Length` bytes.

   .. code-block:: mini

      let A := address("Hello world!\n")
      :> <13:48656C6C6F20776F726C64210A>
      A @ (4, 4) :> <4:6F20776F>


:mini:`meth (Haystack: address):find(Needle: address): integer | nil`
   Returns the offset of the first occurence of the bytes of :mini:`Needle` in :mini:`Haystack` or :mini:`nil` is no occurence is found.

   .. code-block:: mini

      let A := address("Hello world!\n")
      :> <13:48656C6C6F20776F726C64210A>
      A:find("world") :> 6
      A:find("other") :> nil


:mini:`meth (Address: address):get16: integer`
   Returns the signed 16-bit value at :mini:`Address`. Currently follows the platform endiness.

   .. code-block:: mini

      let A := address("Hello world!\n")
      :> <13:48656C6C6F20776F726C64210A>
      A:get16 :> 25928


:mini:`meth (Address: address):get32: integer`
   Returns the signed 32-bit value at :mini:`Address`. Currently follows the platform endiness.

   .. code-block:: mini

      let A := address("Hello world!\n")
      :> <13:48656C6C6F20776F726C64210A>
      A:get32 :> 1819043144


:mini:`meth (Address: address):get64: integer`
   Returns the signed 64-bit value at :mini:`Address`. Currently follows the platform endiness.

   .. code-block:: mini

      let A := address("Hello world!\n")
      :> <13:48656C6C6F20776F726C64210A>
      A:get64 :> 8031924123371070792


:mini:`meth (Address: address):get8: integer`
   Returns the signed 8-bit value at :mini:`Address`.

   .. code-block:: mini

      let A := address("Hello world!\n")
      :> <13:48656C6C6F20776F726C64210A>
      A:get8 :> 72


:mini:`meth (Address: address):getf32: real`
   Returns the single precision floating point value at :mini:`Address`. Currently follows the platform endiness.

   .. code-block:: mini

      let A := address("Hello world!\n")
      :> <13:48656C6C6F20776F726C64210A>
      A:getf32 :> 1.14313912243758e+27


:mini:`meth (Address: address):getf64: real`
   Returns the double precision floating point value at :mini:`Address`. Currently follows the platform endiness.

   .. code-block:: mini

      let A := address("Hello world!\n")
      :> <13:48656C6C6F20776F726C64210A>
      A:getf64 :> 8.76577647882785e+228


:mini:`meth (Address: address):gets: string`
   Returns the string consisting of the bytes at :mini:`Address`.

   .. code-block:: mini

      let A := address("Hello world!\n")
      :> <13:48656C6C6F20776F726C64210A>
      A:gets :> "Hello world!\n"


:mini:`meth (Address: address):gets(Size: integer): string`
   Returns the string consisting of the first :mini:`Size` bytes at :mini:`Address`.

   .. code-block:: mini

      let A := address("Hello world!\n")
      :> <13:48656C6C6F20776F726C64210A>
      A:gets(5) :> "Hello"


:mini:`meth (Address: address):getu16: integer`
   Returns the unsigned 16-bit value at :mini:`Address`. Currently follows the platform endiness.

   .. code-block:: mini

      let A := address("Hello world!\n")
      :> <13:48656C6C6F20776F726C64210A>
      A:getu16 :> 25928


:mini:`meth (Address: address):getu32: integer`
   Returns the unsigned 32-bit value at :mini:`Address`. Currently follows the platform endiness.

   .. code-block:: mini

      let A := address("Hello world!\n")
      :> <13:48656C6C6F20776F726C64210A>
      A:getu32 :> 1819043144


:mini:`meth (Address: address):getu64: integer`
   Returns the unsigned 64-bit value at :mini:`Address`. Currently follows the platform endiness.
   .. warning::
   
      Minilang currently uses signed 64-bit integers so this method will produce incorrect results if the actual value is too large to fit. This may change in future implementations or if arbitrary precision integers are added to the runtime.

   .. code-block:: mini

      let A := address("Hello world!\n")
      :> <13:48656C6C6F20776F726C64210A>
      A:getu64 :> 8031924123371070792


:mini:`meth (Address: address):getu8: integer`
   Returns the unsigned 8-bit value at :mini:`Address`.

   .. code-block:: mini

      let A := address("Hello world!\n")
      :> <13:48656C6C6F20776F726C64210A>
      A:getu8 :> 72


:mini:`meth (Address: address):length: integer`
   Returns the number of bytes visible at :mini:`Address`.

   .. code-block:: mini

      let A := address("Hello world!\n")
      :> <13:48656C6C6F20776F726C64210A>
      A:length :> 13


:mini:`meth (Address: address):size: integer`
   Returns the number of bytes visible at :mini:`Address`.

   .. code-block:: mini

      let A := address("Hello world!\n")
      :> <13:48656C6C6F20776F726C64210A>
      A:size :> 13


:mini:`meth (Buffer: string::buffer):append(Value: address)`
   Appends the contents of :mini:`Value` to :mini:`Buffer`.


