Compiler
========

.. c:type:: struct ml_compiler_t ml_compiler_t;

   Represents a Minilang compiler.
   This is an opaque structure with no user accessible fields.

.. c:type:: ml_value_t *(*ml_getter_t)(void *Globals, const char *Name);

   Signature for callbacks for looking up global identifiers. If :c:var:`Name` is not found, :c:expr:`NULL` should be returned.
   
.. c:type:: const char *(*ml_reader_t)(void *);
   
   Signature for callbacks for reading input for a compiler. If the end of input is reached, :c:expr:`NULL` should be returned.
   
.. c:function:: ml_compiler_t *ml_compiler(ml_getter_t GlobalGet, void *Globals, ml_reader_t Read, void *Input);

   Creates a new :c:type:`ml_compiler_t`.

.. c:function:: const char *ml_compiler_name(ml_compiler_t *Compiler);

   Returns the name of the current source location of the compiler.

.. c:function:: ml_source_t ml_compiler_source(ml_compiler_t *Compiler, ml_source_t Source);

   Sets the source location of the compiler and returns the previous source location.

.. c:function:: void ml_compiler_reset(ml_compiler_t *Compiler);

   Resets the compiler by clearing any cached input and pending tasks.

.. c:function:: void ml_compiler_input(ml_compiler_t *Compiler, const char *Text);

   Sets the next input text in the compiler.

.. c:function:: const char *ml_compiler_clear(ml_compiler_t *Compiler);

   Returns any cached input and clears it in the compiler.

.. c:function:: void ml_compiler_error(ml_compiler_t *Compiler, const char *Error, const char *Format, ...) __attribute__((noreturn));

   Raises a compiler error to the latest call to the compiler.

.. c:function:: void ml_function_compile(ml_state_t *Caller, ml_compiler_t *Compiler, const char **Parameters);

   Compiles a function from the source code available to the compiler. This may be asynchronous, when complete :c:var:`Caller` is run with the compiled function or an error value.

.. c:function:: void ml_command_evaluate(ml_state_t *Caller, ml_compiler_t *Compiler);

   Compiles and executes a single command from the source code available to the compiler. This may be asynchronous, when complete :c:var:`Caller` is run with the result of the command.

.. c:function:: void ml_load_file(ml_state_t *Caller, ml_getter_t GlobalGet, void *Globals, const char *FileName, const char *Parameters[]);

   Creates a compiler with specified globals that reads and compiles a function from the source code in :c:var:`FileName`. This may be asynchronous, when complete :c:var:`Caller` is run with the compiled function or an error value.

.. c:var:: ml_value_t MLEndOfInput[];

   :c:type:`ml_value_t` returned by a compiler when the end of input is reached.

.. c:var:: ml_value_t MLNotFound[];

   :c:type:`ml_value_t` expected from a :c:type:`ml_getter_t` when the given name is not found.

.. c:var:: ml_type_t MLCompilerT[];

   Compiler type. Usually made available to Minilang code as :mini:`compiler`.

.. c:function:: ml_value_t *ml_stringmap_globals(stringmap_t *Globals);

   Creates a Minilang function that looks up names in :c:var:`Globals` and returns :c:var:`MLNotFound` if not present.
