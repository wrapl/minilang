.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

object
======

.. _type-class:

:mini:`type class < type`
   Type of all object classes.


.. _fun-class:

:mini:`fun class(Parents: class, ..., Fields: method, ..., Exports: names, ...): class`
   Returns a new class inheriting from :mini:`Parents`,  with fields :mini:`Fields` and exports :mini:`Exports`. The special exports :mini:`::of` and :mini:`::init` can be set to override the default conversion and initialization behaviour. The :mini:`::new` export will *always* be set to the original constructor for this class.


.. _type-enum:

:mini:`type enum < type, sequence`
   The base type of enumeration types.


.. _fun-enum:

:mini:`fun enum(Values: string, ...): enum`
   Returns a new enumeration type.

   .. code-block:: mini

      let day := enum("Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun")
      :> <<day>>
      day::Wed :> Wed
      day::Fri + 0 :> 5


:mini:`meth (Enum: enum):count: integer`
   Returns the size of the enumeration :mini:`Enum`.

   .. code-block:: mini

      let day := enum("Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun")
      :> <<day>>
      day:count :> 7


.. _type-enum-range:

:mini:`type enum::range < sequence`
   A range of enum values.


.. _type-enum-value:

:mini:`type enum::value < integer`
   An instance of an enumeration type.


:mini:`meth (Min: enum::value) .. (Max: enum::value): enum::range`
   Returns a range of enum values. :mini:`Min` and :mini:`Max` must belong to the same enumeration.

   .. code-block:: mini

      let day := enum("Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun")
      :> <<day>>
      day::Mon .. day::Fri :> <enum-range>


:mini:`meth (Arg₁: string::buffer):append(Arg₂: enum::value)`
   *TBD*


.. _type-flags:

:mini:`type flags < type`
   The base type of flag types.


:mini:`meth flags(Name₁: string, ...): flags`
   Returns a new flags type,  where :mini:`Nameᵢ` has value $2^{i-1}$.

   .. code-block:: mini

      let mode := flags("Read", "Write", "Execute") :> <<mode>>
      mode::Read :> Read
      mode::Read + mode::Write :> Write|Read


:mini:`meth flags(Name₁ is Value₁, ...): flags`
   Returns a new flags type
   Returns a new flags type,  where :mini:`Nameᵢ` has value :mini:`Valueᵢ`.

   .. code-block:: mini

      let mode := flags(Read is 1, Write is 4, Execute is 32)
      :> <<mode>>
      mode::Read :> Read
      mode::Read + mode::Write :> Write|Read


.. _type-flags-value:

:mini:`type flags::value < integer`
   An instance of a flags type.


:mini:`meth (Flags₁: flags::value) + (Flags₂: flags::value): flags::value`
   Returns the union of :mini:`Flags₁` and :mini:`Flags₂`. :mini:`Flags₁` and :mini:`Flags₂` must have the same flags type.

   .. code-block:: mini

      let mode := flags("Read", "Write", "Execute") :> <<mode>>
      mode::Read + mode::Write :> Write|Read


:mini:`meth (Flags₁: flags::value) - (Flags₂: flags::value): flags::value`
   Returns the difference of :mini:`Flags₁` and :mini:`Flags₂`. :mini:`Flags₁` and :mini:`Flags₂` must have the same flags type.

   .. code-block:: mini

      let mode := flags("Read", "Write", "Execute") :> <<mode>>
      mode("Read", "Write") - mode::Write :> Read


:mini:`meth (Flags₁: flags::value) < (Flags₂: flags::value): flags::value`
   Returns the :mini:`Flags₂` if it contains all of :mini:`Flags₁`. :mini:`Flags₁` and :mini:`Flags₂` must have the same flags type.

   .. code-block:: mini

      let mode := flags("Read", "Write", "Execute") :> <<mode>>
      mode("Read", "Write") < mode("Read", "Write", "Execute")
      :> Write|Read|Execute
      mode("Read", "Write", "Execute") < mode("Read", "Write")
      :> nil


:mini:`meth (Flags₁: flags::value) <= (Flags₂: flags::value): flags::value`
   Returns the :mini:`Flags₂` if it contains all of :mini:`Flags₁`. :mini:`Flags₁` and :mini:`Flags₂` must have the same flags type.

   .. code-block:: mini

      let mode := flags("Read", "Write", "Execute") :> <<mode>>
      mode("Read", "Write") <= mode("Read", "Write", "Execute")
      :> Write|Read|Execute
      mode("Read", "Write", "Execute") <= mode("Read", "Write")
      :> nil


:mini:`meth (Flags₁: flags::value) > (Flags₂: flags::value): flags::value`
   Returns the :mini:`Flags₂` if it is contained in :mini:`Flags₁`. :mini:`Flags₁` and :mini:`Flags₂` must have the same flags type.

   .. code-block:: mini

      let mode := flags("Read", "Write", "Execute") :> <<mode>>
      mode("Read", "Write") > mode("Read", "Write", "Execute")
      :> nil
      mode("Read", "Write", "Execute") > mode("Read", "Write")
      :> Write|Read


:mini:`meth (Flags₁: flags::value) >= (Flags₂: flags::value): flags::value`
   Returns the :mini:`Flags₂` if it is contained in :mini:`Flags₁`. :mini:`Flags₁` and :mini:`Flags₂` must have the same flags type.

   .. code-block:: mini

      let mode := flags("Read", "Write", "Execute") :> <<mode>>
      mode("Read", "Write") >= mode("Read", "Write", "Execute")
      :> nil
      mode("Read", "Write", "Execute") >= mode("Read", "Write")
      :> Write|Read


:mini:`meth list(Arg₁: flags::value)`
   *TBD*


:mini:`meth (Arg₁: string::buffer):append(Arg₂: flags::value)`
   *TBD*


.. _type-object:

:mini:`type object`
   Parent type of all object classes.


:mini:`meth (Object: object) :: (Field: string): field`
   Retrieves the field :mini:`Field` from :mini:`Object`. Mainly intended for unpacking objects.


:mini:`meth (Arg₁: string::buffer):append(Arg₂: object)`
   *TBD*


.. _type-property:

:mini:`type property`
   A value with an associated setter function.


.. _fun-property:

:mini:`fun property(Value: any, Set: function): property`
   Returns a new property which dereferences to :mini:`Value`. Assigning to the property will call :mini:`Set(NewValue)`.


