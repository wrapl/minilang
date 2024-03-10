.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

uuid
====

.. rst-class:: mini-api

.. note::
   Depending on how *Minilang* is built,  :mini:`uuid` might need to be imported using :mini:`import: uuid("util/uuid")`.

:mini:`meth address(UUID: uuid): address`
   Returns an address view of :mini:`UUID`.

   .. code-block:: mini

      import: uuid("util/uuid")
      address(uuid()) :> <16:AD4FDB78C297400F8BF29B7816C10650>


:mini:`type uuid`
   A UUID.


:mini:`meth uuid(Address: address): uuid | error`
   Returns a new UUID with the bytes of :mini:`Address`.


:mini:`meth uuid(String: string): uuid | error`
   Parses :mini:`String` as a UUID,  returning an error if :mini:`String` does not have the correct format.

   .. code-block:: mini

      import: uuid("util/uuid")
      uuid("5fe1af82-02f9-429a-8787-4a7c16628a02")
      :> 5fe1af82-02f9-429a-8787-4a7c16628a02
      uuid("test") :> error("UUIDError", "Invalid UUID string")


:mini:`meth uuid(): uuid`
   Returns a new random UUID.

   .. code-block:: mini

      import: uuid("util/uuid")
      uuid() :> 713af698-5f8c-49f0-8ce3-8fb8a85e2df9


:mini:`meth (Arg₁: uuid) <> (Arg₂: uuid)`
   *TBD*


:mini:`meth (Buffer: string::buffer):append(UUID: uuid)`
   Appends a representation of :mini:`UUID` to :mini:`Buffer`.


