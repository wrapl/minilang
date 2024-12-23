.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

object
======

.. rst-class:: mini-api

:mini:`type class < type`
   Type of all object classes.


:mini:`fun class(Parents: class, ..., Fields: method, ..., Exports: names, ...): class`
   Returns a new class inheriting from :mini:`Parents`,  with fields :mini:`Fields` and exports :mini:`Exports`. The special exports :mini:`::of` and :mini:`::init` can be set to override the default conversion and initialization behaviour. The :mini:`::new` export will *always* be set to the original constructor for this class.


:mini:`meth (Arg₁: class):fields`
   *TBD*


:mini:`type field`
   *TBD*


:mini:`type field::mutable < field`
   *TBD*


:mini:`meth (Arg₁: method):MLMethodDefault(Arg₂: field::modifier)`
   *TBD*


:mini:`meth (Arg₁: method):MLMethodDefault(Arg₂: watched)`
   *TBD*


:mini:`type object`
   Parent type of all object classes.


:mini:`meth (Object: object) :: (Field: string): field`
   Retrieves the field :mini:`Field` from :mini:`Object`. Mainly intended for unpacking objects.


:mini:`meth (Arg₁: string::buffer):append(Arg₂: object)`
   *TBD*


:mini:`type property`
   A value with an associated setter function.


:mini:`fun property(Value: any, Set: function): property`
   Returns a new property which dereferences to :mini:`Value`. Assigning to the property will call :mini:`Set(NewValue)`.


