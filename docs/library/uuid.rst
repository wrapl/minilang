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
      address(uuid()) :> <16:35745942BE11411BAC6FDBA0DC86303F>


.. _type-uuid:

:mini:`type uuid`
   A UUID.


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
      uuid() :> 080e53a0-ec8c-41da-8ae8-b94f911e9ee5


:mini:`meth (Arg₁: uuid) <> (Arg₂: uuid)`
   *TBD*


:mini:`meth (Buffer: string::buffer):append(UUID: uuid)`
   Appends a representation of :mini:`UUID` to :mini:`Buffer`.


