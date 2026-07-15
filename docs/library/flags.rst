.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

flags
=====

.. rst-class:: mini-api

:mini:`fun mlflags()`
   *TBD*


:mini:`type flags < type`
   The base type of flag types.


:mini:`meth (Arg₁: flags) :: (Arg₂: list)`
   *TBD*


:mini:`meth (Flags: flags::value):in(Spec: flags::spec)`
   *TBD*


:mini:`meth ~(Flags: flags::value): flags::spec`
   *TBD*


:mini:`meth !(Flags: flags::value): flags::spec`
   *TBD*


:mini:`meth (Flags₁: flags::value) / (Flags₂: flags::value): flags::spec`
   *TBD*


:mini:`meth (Flags₁: flags::value) >= (Flags₂: flags::value): flags::value`
   Returns the :mini:`Flags₂` if it is contained in :mini:`Flags₁`. :mini:`Flags₁` and :mini:`Flags₂` must have the same flags type.

   .. code-block:: mini

      let mode := flags("Read", "Write", "Execute") :> <<mode>>
      mode("Read", "Write") >= mode("Read", "Write", "Execute")
      :> nil
      mode("Read", "Write", "Execute") >= mode("Read", "Write")
      :> Read|Write


:mini:`meth (Flags₁: flags::value) > (Flags₂: flags::value): flags::value`
   Returns the :mini:`Flags₂` if it is contained in :mini:`Flags₁`. :mini:`Flags₁` and :mini:`Flags₂` must have the same flags type.

   .. code-block:: mini

      let mode := flags("Read", "Write", "Execute") :> <<mode>>
      mode("Read", "Write") > mode("Read", "Write", "Execute")
      :> nil
      mode("Read", "Write", "Execute") > mode("Read", "Write")
      :> Read|Write


:mini:`meth (Flags₁: flags::value) <= (Flags₂: flags::value): flags::value`
   Returns the :mini:`Flags₂` if it contains all of :mini:`Flags₁`. :mini:`Flags₁` and :mini:`Flags₂` must have the same flags type.

   .. code-block:: mini

      let mode := flags("Read", "Write", "Execute") :> <<mode>>
      mode("Read", "Write") <= mode("Read", "Write", "Execute")
      :> Read|Write|Execute
      mode("Read", "Write", "Execute") <= mode("Read", "Write")
      :> nil


:mini:`meth (Flags₁: flags::value) < (Flags₂: flags::value): flags::value`
   Returns the :mini:`Flags₂` if it contains all of :mini:`Flags₁`. :mini:`Flags₁` and :mini:`Flags₂` must have the same flags type.

   .. code-block:: mini

      let mode := flags("Read", "Write", "Execute") :> <<mode>>
      mode("Read", "Write") < mode("Read", "Write", "Execute")
      :> Read|Write|Execute
      mode("Read", "Write", "Execute") < mode("Read", "Write")
      :> nil


:mini:`meth (Flags₁: flags::value) - (Flags₂: flags::value): flags::value`
   Returns the difference of :mini:`Flags₁` and :mini:`Flags₂`. :mini:`Flags₁` and :mini:`Flags₂` must have the same flags type.

   .. code-block:: mini

      let mode := flags("Read", "Write", "Execute") :> <<mode>>
      mode("Read", "Write") - mode::Write :> Read


:mini:`meth (Flags₁: flags::value) + (Flags₂: flags::value): flags::value`
   Returns the union of :mini:`Flags₁` and :mini:`Flags₂`. :mini:`Flags₁` and :mini:`Flags₂` must have the same flags type.

   .. code-block:: mini

      let mode := flags("Read", "Write", "Execute") :> <<mode>>
      mode::Read + mode::Write :> Read|Write


:mini:`meth (Arg₁: string::buffer):append(Arg₂: flags::spec)`
   *TBD*


:mini:`type flags::value`
   An instance of a flags type.


:mini:`meth (Arg₁: string::buffer):append(Arg₂: flags::value)`
   *TBD*


:mini:`meth list(Arg₁: flags::value)`
   *TBD*


:mini:`type flags::spec`
   A pair of flag sets for including and excluding flags.


:mini:`meth integer(Arg₁: flags::value)`
   *TBD*


