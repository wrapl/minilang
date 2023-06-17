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
      address(uuid()) :> <16:692250FDBE364A90BCA06FF07D782427>


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
      uuid() :> e26a6636-51d1-48bf-9473-48804d270bf7


:mini:`meth (Arg₁: uuid) <> (Arg₂: uuid)`
   *TBD*


:mini:`meth (Buffer: string::buffer):append(UUID: uuid)`
   Appends a representation of :mini:`UUID` to :mini:`Buffer`.


