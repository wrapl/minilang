.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

flags
=====

.. rst-class:: mini-api

.. _type-flags:

:mini:`type flags < type`
   The base type of flag types.


:mini:`meth flags(Name₁ is Value₁, ...): flags`
   Returns a new flags type
   Returns a new flags type,  where :mini:`Nameᵢ` has value :mini:`Valueᵢ`.

   .. code-block:: mini

      let mode := flags(Read is 1, Write is 4, Execute is 32)
      :> <<mode>>
      mode::Read :> Read
      mode::Read + mode::Write :> Write,Read


:mini:`meth flags(Name₁: string, ...): flags`
   Returns a new flags type,  where :mini:`Nameᵢ` has value :math:`2^{i-1}`.

   .. code-block:: mini

      let mode := flags("Read", "Write", "Execute") :> <<mode>>
      mode::Read :> Read
      mode::Read + mode::Write :> Write,Read


.. _type-flags-spec:

:mini:`type flags::spec`
   A pair of flag sets for including and excluding flags.


:mini:`meth (Arg₁: string::buffer):append(Arg₂: flags::spec)`
   *TBD*


.. _type-flags-value:

:mini:`type flags::value`
   An instance of a flags type.


:mini:`meth (Flags₁: flags::value) + (Flags₂: flags::value): flags::value`
   Returns the union of :mini:`Flags₁` and :mini:`Flags₂`. :mini:`Flags₁` and :mini:`Flags₂` must have the same flags type.

   .. code-block:: mini

      let mode := flags("Read", "Write", "Execute") :> <<mode>>
      mode::Read + mode::Write :> Write,Read


:mini:`meth (Flags₁: flags::value) - (Flags₂: flags::value): flags::value`
   Returns the difference of :mini:`Flags₁` and :mini:`Flags₂`. :mini:`Flags₁` and :mini:`Flags₂` must have the same flags type.

   .. code-block:: mini

      let mode := flags("Read", "Write", "Execute") :> <<mode>>
      mode("Read", "Write") - mode::Write :> Read


:mini:`meth /(Flags: flags::value): flags::spec`
   *TBD*


:mini:`meth (Flags₁: flags::value) / (Flags₂: flags::value): flags::spec`
   *TBD*


:mini:`meth (Flags₁: flags::value) < (Flags₂: flags::value): flags::value`
   Returns the :mini:`Flags₂` if it contains all of :mini:`Flags₁`. :mini:`Flags₁` and :mini:`Flags₂` must have the same flags type.

   .. code-block:: mini

      let mode := flags("Read", "Write", "Execute") :> <<mode>>
      mode("Read", "Write") < mode("Read", "Write", "Execute")
      :> Write,Read,Execute
      mode("Read", "Write", "Execute") < mode("Read", "Write")
      :> nil


:mini:`meth (Flags₁: flags::value) <= (Flags₂: flags::value): flags::value`
   Returns the :mini:`Flags₂` if it contains all of :mini:`Flags₁`. :mini:`Flags₁` and :mini:`Flags₂` must have the same flags type.

   .. code-block:: mini

      let mode := flags("Read", "Write", "Execute") :> <<mode>>
      mode("Read", "Write") <= mode("Read", "Write", "Execute")
      :> Write,Read,Execute
      mode("Read", "Write", "Execute") <= mode("Read", "Write")
      :> nil


:mini:`meth (Flags₁: flags::value) > (Flags₂: flags::value): flags::value`
   Returns the :mini:`Flags₂` if it is contained in :mini:`Flags₁`. :mini:`Flags₁` and :mini:`Flags₂` must have the same flags type.

   .. code-block:: mini

      let mode := flags("Read", "Write", "Execute") :> <<mode>>
      mode("Read", "Write") > mode("Read", "Write", "Execute")
      :> nil
      mode("Read", "Write", "Execute") > mode("Read", "Write")
      :> Write,Read


:mini:`meth (Flags₁: flags::value) >= (Flags₂: flags::value): flags::value`
   Returns the :mini:`Flags₂` if it is contained in :mini:`Flags₁`. :mini:`Flags₁` and :mini:`Flags₂` must have the same flags type.

   .. code-block:: mini

      let mode := flags("Read", "Write", "Execute") :> <<mode>>
      mode("Read", "Write") >= mode("Read", "Write", "Execute")
      :> nil
      mode("Read", "Write", "Execute") >= mode("Read", "Write")
      :> Write,Read


:mini:`meth (Flags: flags::value):in(Spec: flags::spec)`
   *TBD*


:mini:`meth list(Arg₁: flags::value)`
   *TBD*


:mini:`meth (Arg₁: string::buffer):append(Arg₂: flags::value)`
   *TBD*


