Compiler
========

.. c:type:: ml_compiler_t

   Represents a Minilang compiler.
   This is an opaque structure with no user accessible fields.

.. c:function:: ml_compiler_t ml_compiler(const char *SourceName, void *Data, const char *(*read)(void *), ml_getter_t GlobalGet, void *Globals)

   Creates a new :c:type:`ml_compiler_t`.

