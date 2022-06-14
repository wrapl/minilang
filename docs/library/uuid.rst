.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

uuid
====

.. note::
   Depending on how *Minilang* is built,  :mini:`uuid` might need to be imported using :mini:`import: uuid("std/uuid")`.

:mini:`meth address(UUID: uuid): address`
   Returns an address view of :mini:`UUID`.

   .. code-block:: mini

      import: uuid("std/uuid")
      address(uuid()) :> <16:45EFB039FA9343FBA7DE18B0E8A4AF5C>


.. _type-uuid:

:mini:`type uuid`
   A UUID.


:mini:`meth uuid(String: string): uuid | error`
   Parses :mini:`String` as a UUID,  returning an error if :mini:`String` does not have the correct format.

   .. code-block:: mini

      import: uuid("std/uuid")
      uuid("5fe1af82-02f9-429a-8787-4a7c16628a02")
      :> 5fe1af82-02f9-429a-8787-4a7c16628a02
      uuid("test") :> error("UUIDError", "Invalid UUID string")


:mini:`meth uuid(): uuid`
   Returns a new random UUID.

   .. code-block:: mini

      import: uuid("std/uuid")
      uuid() :> 0eed4b73-1390-4be6-9c77-5afc103c53ca


:mini:`meth (Arg₁: uuid) <> (Arg₂: uuid)`
   *TBD*


:mini:`meth (Buffer: string::buffer):append(UUID: uuid)`
   Appends a representation of :mini:`UUID` to :mini:`Buffer`.


