Types
=====

.. c:type:: struct ml_value_t

   Represents a Minilang value.
   Every Minilang value must contain this struct at the start.
   
   .. c:member:: const ml_type_t *Type
   
   Type of this value.


.. c:type:: struct ml_type_t

   Represents a Minilang type.
   
   Although this is not a completely opaque type, avoid creating instances directly and instead use the :c:macro:`ML_TYPE` macro to declare and initialize a :c:type:`ml_type_t`.
   
   .. c:member:: const ml_type_t *Type
   
      Type of this value. Will be :c:data:`MLTypeT` or a sub type of it.
   
   .. c:member:: const ml_type_t **Types
   
      :c:`NULL`-terminated array of parent types used for method resolution.
      
   .. c:member:: const char *Name
   
      A short string name for the type, used for the default string conversion and error messages.
      
   .. c:member:: long (*hash)(ml_value_t *, ml_hash_chain_t *)
   
      Function for hashing instances of this type.
   
   .. c:member:: void (*call)(ml_state_t *, ml_value_t *, int, ml_value_t **)
   
      Function for calling instances of this type.
   
   .. c:member:: ml_value_t *(*deref)(ml_value_t *)
   
      Function for dereferencing instances of this type.
   
   .. c:member:: ml_value_t *(*assign)(ml_value_t *, ml_value_t *)
   
      Function for assigning values to instances of this type.
      
   .. c:member:: ml_value_t *Constructor
   
      Minilang function to call when calling this type as a constructor.


.. c:macro:: ML_TYPE(TYPE, PARENTS, NAME, ...)

   Declares and initializes a new type.

   :c:`PARENTS` must be a list of parent types written in parentheses :c:`(Type₁, Type₂, ...)`. It is not necessary to include :c:data:`MLAnyT` (the base type of all types). The list of parent types can be empty.
   
   The callback functions are initialized to defaults and should be overridden if required using designated initializers: e.g. :c:`.deref = my_deref`.


.. c:macro:: ML_INTERFACE(TYPE, PARENTS, NAME, ...)

   Same as :c:macro:`ML_TYPE` for types with empty instances (and thus behave as an interface).

.. c:var:: ml_type_t MLTypeT[]

   The type of Minilang types.


.. c:var:: ml_type_t MLAnyT[]

   The root type of all Minilang values.


.. c:var:: ml_value_t MLNil[]

   The :mini:`nil` value.


.. c:var:: ml_value_t MLSome[]

   A non-:mini:`nil` value. Has no other use.


.. c:function:: int ml_is(const ml_value_t *Value, const ml_type_t *Type)

   Returns 1 if :c:`Value` is an instance of :c:`Type` or a subtype, returns 0 otherwise.

.. c:function:: long ml_hash_chain(ml_value_t *Value, ml_hash_chain_t *Chain)

   Returns a hash value for :c:`Value`. Uses :c:`Chain` to break cycles while hashing.
   
   :c:`Chain` can be :c:`NULL` for the top-level call to :c:func:`ml_hash_chain()`, hash functions for structures which may introduce cycles should create a new :c:struct:`ml_hash_chain_t` and pass that to their calls to :c:func:`ml_hash_chain()`.

.. c:function:: long ml_hash(ml_value_t *Value)

   Equivalent to :c:func:`ml_hash_chain(Value, NULL)`.

.. c:type:: struct ml_hash_chain_t

   If :c:func:`ml_hash_chain()` is called with a value already in the chain, the previous index is returned, preventing cycles.

   .. c:member:: ml_hash_chain_t *Previous

   Previous link in the hash chain.

   .. c:member:: ml_value_t *Value

   The value that was encountered.

   .. c:member:: long Index

   The previous index when the value was encountered.


.. c:var:: ml_type_t MLIteratableT[]

   An interface for all iteratable types.

.. c:function:: void ml_iterate(ml_state_t *Caller, ml_value_t *Value)

   Start iterating over :c:`Value`. Resumes :c:`Caller` with an iterator (or :mini:`nil` if :c:`Value` is empty, or an :mini:`error` if :c:`Value` is not iteratable or another error occurs).

.. c:function::  void ml_iter_value(ml_state_t *Caller, ml_value_t *Iter)

.. c:function::  void ml_iter_key(ml_state_t *Caller, ml_value_t *Iter)

.. c:function::  void ml_iter_next(ml_state_t *Caller, ml_value_t *Iter)