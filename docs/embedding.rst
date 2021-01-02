Embedding
=========

.. warning::
   Minilang uses the `Hans-Boehm conservative garbage collector <https://github.com/ivmai/bdwgc>`_. This simplifies memory management in most cases, but may not be compatible with all use cases. In the future, the option to use a different form of memory management may be added to Minilang.

Since Minilang has been designed to support a wide range of use cases, it provides a rich API for embedding into C or C++ applications.

Values
------

All values in Minilang are represented with a :c:struct:`ml_value_t`. This has a single guaranteed field, :c:member:`ml_value_t.Type`, which is a pointer to the :c:struct:`ml_type_t` describing this type. Actual :c:struct:`ml_value_t` instances will have additional fields according to the actual type.

.. code-block:: c

   typedef struct ml_value_t ml_value_t;
   typedef struct ml_type_t ml_type_t;
   
   struct ml_value_t {
      ml_type_t *Type;
   }
   
   struct ml_type_t {
      ml_type_t *Type;
      // Additional fields
   }

The function :c:func:`ml_typeof` returns the type of :c:var:`Value`. This should be used rather than accessing :c:expr:`Value->Type` directly since Minilang can be compiled with NaN-boxing, which encodes numeric values directly as :c:struct:`ml_value_t`'s.

Constructing Values
~~~~~~~~~~~~~~~~~~~

To construct instances of the basic types, use the following constants and functions:

.. code-block:: c

   extern ml_value_t MLNil[];
   extern ml_value_t MLSome[];

   ml_value_t *ml_boolean(int Value);
   ml_value_t *ml_integer(long Value);
   ml_value_t *ml_real(double Value);
   ml_value_t *ml_string(const char *Value, int Length);
   ml_value_t *ml_cstring(const char *Value); // Uses strlen
   ml_value_t *ml_string_format(const char *Format, ...); // Uses sprintf
   ml_value_t *ml_regex(const char *Value, int Length);
   ml_value_t *ml_method(const char *Name);
   ml_value_t *ml_method_anon(const char *Name);

Tuples, lists, maps and other aggregrate types are covered later.

Testing Types
~~~~~~~~~~~~~

The basic types are defined as :c:expr:`extern ml_type_t[]` values. Although it is possible to use :c:expr:`ml_typeof(Value) == Type` directly, the preferred way to test the type of an :c:struct:`ml_value_t` is to use :c:func:`ml_is` which also checks for subtypes.

.. code-block:: c

   extern ml_type_t MLAnyT[];
   extern ml_type_t MLNilT[];
   extern ml_type_t MLBooleanT[];
   extern ml_type_t MLNumberT[];
   extern ml_type_t MLIntegerT[];
   extern ml_type_t MLRealT[];
   extern ml_type_t MLStringT[];
   extern ml_type_t MLRegexT[];
   extern ml_type_t MLMethodT[];

   int ml_is(ml_value_t *Value, ml_type_t *Type);
   int ml_is_error(ml_value_t *Value);

Extracting C Values
~~~~~~~~~~~~~~~~~~~

After checking the type of a :c:struct:`ml_value_t` using :c:func:`ml_is`, the following functions can be used to extract the corresponding values:

.. code-block:: c

   int ml_boolean_value(ml_value_t *Value);
   long ml_integer_value(const ml_value_t *Value);
   long ml_integer_value_fast(const ml_value_t *Value);
   double ml_real_value(const ml_value_t *Value);
   double ml_real_value_fast(const ml_value_t *Value);
   const char *ml_string_value(const ml_value_t *Value);
   size_t ml_string_length(const ml_value_t *Value);
   const char *ml_regex_pattern(const ml_value_t *Value);
   const char *ml_method_name(const ml_value_t *Value)

String values are not copied and should not be modified.

In general, these functions do not check the type of the value they are passed, this is up to the caller. However the numeric value functions, :c:func:`ml_integer_value` and :c:func:`ml_real_value` do check their value types and convert accordingly (or return :c:expr:`0`). This allows them to be used for :c:var:`MLNumberT` values. The `_fast` versions of these functions can be used when the exact type is known.

Initialization
--------------

.. note::

   Stringmaps are found throughout the Minilang API to store string-value entries.
   
   .. code-block:: c
   
      typedef struct stringmap_t stringmap_t;
      
      stringmap_t *stringmap_new();
      // Stringmaps can also be initialized by assigning STRINGMAP_INIT
      
      void *stringmap_search(const stringmap_t *Map, const char *Key);
      void *stringmap_insert(stringmap_t *Map, const char *Key, void *Value);
      void *stringmap_remove(stringmap_t *Map, const char *Key);
      void **stringmap_slot(stringmap_t *Map, const char *Key);
      int stringmap_foreach(stringmap_t *Map, void *Data, int (*callback)(const char *, void *, void *));
   
Before any other Minilang API function can be used, the runtime needs to be initialized using :c:func:`ml_init`.

Optional features should then be initialized, these optional initializers can be passed a :c:struct:`stringmap_t` to insert any global exports they provide. This globals can then be made available to the compiler API functions. The exports from each initializer is covered in their respective sections.

.. code-block:: c

   #include <minilang/minilang.h>

   int main(int Argc, const char *Argv[]) {
      stringmap_t *Globals = stringmap_new();
      ml_init();
      ml_types_init(Globals);
      ml_file_init(Globals);
      ml_object_init(Globals);
      ml_iterfns_init(Globals);
      // ...
   }

States
------

The other important type in the Minilang API is the :c:struct:`ml_state_t`. Minilang uses **Continuation Passing Style (CPS)** for function calls, and the :c:struct:`ml_state_t` type respresents the continuations. The callback in :c:member:`ml_state_t.run` is called when the state is ready to run with a result from some computation.

.. code-block:: c

   typedef struct ml_context_t ml_context_t;
   typedef struct ml_state_t ml_state_t;
   
   typedef void (*ml_state_fn)(ml_state_t *State, ml_value_t *Result);
   
   struct ml_state_t {
      ml_type_t *Type;
      ml_state_t *Caller;
      ml_state_fn run;
      ml_context_t *Context;
   }
   
Like :c:struct:`ml_value_t`, different types of :c:struct:`ml_state_t` may have additional fields.

For applications which launch Minilang scripts as their main operation, the predefined :c:var:`MLMain` state is provided. When run, this state will output an error message if the result was an error, otherwise it will silently do nothing.

For example, if :c:expr:`Function` contains a callable :c:struct:`ml_value_t` then the following example can be used to call :mini:`Function(10, "Hello world")`.

.. code-block:: c

   int main(int Argc, const char *Argv[]) {
      // ...
      ml_inline(MLMain, Function, 2, ml_integer(10), ml_cstring("Hello world"));
   }

Another useful state type is a :c:struct:`ml_call_state_t` which holds a number of :c:struct:`ml_value_t`'s. When run, it will call the result as a function, with the supplied :c:struct:`ml_value_t`'s as arguments. For example, the following will load code from the specified file, run it with the supplied arguments as a single list called :mini:`Args` and then exit silently or with an error.

.. code-block:: c

   int main(int Argc, const char *Argv[]) {
      static const char *Parameters[] = {"Args", NULL};
      if (Argc < 2) {
         fprintf(stderr, "Usage: %s <filename> [<arguments> ...]\n", Argv[0]);
         exit(1);
      }
      ml_value_t *Args = ml_list();
      const char *FileName = Argv[1];
      for (int I = 2; I < Argc; ++I) ml_list_append(Args, ml_cstring(Argv[I]));
      ml_call_state_t *State = ml_call_state_new(MLMain, 1);
      State->Args[0] = Args;
      ml_load_file((ml_state_t *)State, global_get, NULL, FileName, Parameters);
   } 

Loading Minilang Code
---------------------

After initializing the Minilang runtime and populating a :c:struct:`stringmap_t` with some global identifiers, Minilang code can be loaded using a :c:struct:`ml_compiler_t`.

Compiler Initialization
~~~~~~~~~~~~~~~~~~~~~~~

A compiler can be created using :c:func:`ml_compiler`.

.. code-block:: c

   typedef struct ml_compiler_t ml_compiler_t;

   typedef const char *(*ml_reader_t)(void *Input);
   typedef ml_value_t *(*ml_getter_t)(void *Globals, const char *Name);

   ml_compiler_t *ml_compiler(ml_getter_t GlobalGet, void *Globals, ml_reader_t Read, void *Input);
   
   ml_source_t ml_compiler_source(ml_compiler_t *Compiler, ml_source_t Source);
   void ml_compiler_input(ml_compiler_t *Compiler, const char *Text);
   void ml_compiler_reset(ml_compiler_t *Compiler);
   const char *ml_compiler_clear(ml_compiler_t *Compiler);

The :c:expr:`Read` function is responsible for reading in source code and is called whenever the compiler needs more input. It should return :c:expr:`NULL` when the end of input is reached. :c:expr:`Read` can be set to :c:expr:`NULL`, in which case a default function which always returns :c:expr:`NULL` is used. In that case, :c:func:`ml_compiler_input` should be used to set the source code and :c:func:`ml_compiler_source` to set the source location information.

.. note::

   Although the :c:expr:`Read` function can return multiple lines, currently the compiler does not support partial lines. I.e. if a token is split across separate results from :c:expr:`Read`, the compiler will not join the pieces into the original token. 

The :c:expr:`GlobalGet` function is responsible for supplying global identifiers to the compiler. Since there are no predefined functions, this function is always needed. :c:expr:`GlobalGet` should return :c:expr:`NULL` if the specified identifier is not defined. For convenience, the signature matches that of :c:func:`stringmap_search`, in which case :c:expr:`Globals` should be a :c:struct:`stringmap_t`.

Once created, there are two main ways to use a :c:struct:`ml_compiler_t`:

Function Compilation
~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   void ml_function_compile(ml_state_t *Caller, ml_compiler_t *Compiler, const char **Parameters);

This function reads in the body of a Minilang function and compiles it into a Minilang function with the provided parameter names. I.e. it compiles the following ``fun(<Parameter₁>, <Parameter₂>, ...) do <Source> end`` and runs :c:expr:`Caller` with the resulting function, or error if an error occurred during compilation.

For convenience, the :c:func:`ml_load_file` function wraps compiler creation and file reading into a single call:

.. code-block:: c

   void ml_load_file(ml_state_t *Caller, ml_getter_t GlobalGet, void *Globals, const char *FileName, const char *Parameters[]);

Incremental Evaluation
~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   extern ml_value_t MLEndOfInput[];

   void ml_command_evaluate(ml_state_t *Caller, ml_compiler_t *Compiler);

This functions reads in a single expression or declaration and evaluates it within the compiler, running :c:expr:`Caller` with the result. Each compiler maintains its own set of toplevel declarations.

Typically this function is used with :c:func:`ml_compiler_input` to set the input code at each evaluation. Since a single piece of code can contain multiple expressions or declarations, :c:func:`ml_command_evaluate` should be called again within :c:expr:`Caller`'s run function unless the value passed to :c:expr:`Caller->run` is :c:var:`MLEndOfInput`.