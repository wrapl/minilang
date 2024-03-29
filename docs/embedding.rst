Embedding
=========

Since Minilang has been designed to support a wide range of use cases, it provides a rich API for embedding into C or C++ applications. In general, an application only needs to include ``minilang/minilang.h`` and link to ``libminilang.a`` and ``libgc.a`` to use Minilang.

Memory Management
-----------------

Minilang uses the `Hans-Boehm conservative garbage collector <https://github.com/ivmai/bdwgc>`_. This simplifies memory management in most cases, but may not be compatible with all use cases. In the future, the option to use a different form of memory management may be added to Minilang.

All memory that is passed to Minilang or is used to store Minilang values should be allocated with the garbage collector in order to ensure that memory does not leak and that Minilang values are not prematurely collected. For more complex uses (e.g. storing Minilang values in non-gc allocated memory or using finalizers to clean up resources when Minilang values are collected), the application can use the garbage collector API directly.

A number of macros are defined in ``minilang/ml_macros.h`` which simplifies allocating memory for structs and arrays. This header file is not included automatically (in order to avoid conflicts with C++ code, and to keep application header files clean).

.. c:macro:: new(T)

   Returns a block of memory of :c:expr:`sizeof(T)` bytes, cast to :c:expr:`T *`. Used for allocating single values / structs.

.. c:macro:: anew(T, N)

   Returns a block of memory of :c:expr:`N * sizeof(T)` bytes, cast to :c:expr:`T *`. Used for allocating arrays of values / structs.

.. c:macro:: xnew(T, N, U)

   Returns a block of memory of :c:expr:`sizeof(T) + N * sizeof(U)` bytes, cast to :c:expr:`T *`. Used for allocating structs with variable-length arrays (*VLA*'s) as the last field.

.. c:macro:: snew(N)

   Returns a block of memory of :c:expr:`N` bytes that is expected to remain free of pointers. Used for allocating strings.

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
   ml_value_t *ml_cstring("STRING_LITERAL");
   ml_value_t *ml_string_format(const char *Format, ...); // Uses sprintf
   ml_value_t *ml_regex(const char *Value, int Length);
   ml_value_t *ml_method(const char *Name);
   ml_value_t *ml_method_anon(const char *Name);

Tuples, lists, maps and other aggregrate types are covered later.

Testing Types
~~~~~~~~~~~~~

The basic types are defined as :code:`extern ml_type_t[]` values. Although it is possible to use :c:expr:`ml_typeof(Value) == Type` directly, the preferred way to test the type of an :c:struct:`ml_value_t` is to use :c:func:`ml_is` which also checks for subtypes.

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
   const char *ml_method_name(const ml_value_t *Value);

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

Optional features should then be initialized, these optional initializers can be passed a :c:struct:`stringmap_t` to insert any global exports they provide. This globals can then be made available to the compiler API functions. The exports from each initializer is covered in their respective sections. Each optional :c:func:`ml_<name>_init` function is define in the corresponding :file:`ml_<name>.h` header file.

.. code-block:: c

   #include <minilang/minilang.h>
   #include <minilang/ml_sequence.h>
   #include <minilang/ml_object.h>
   #include <minilang/ml_tasks.h>
   #include <minilang/ml_file.h>
   #include <minilang/ml_math.h>
   #include <minilang/ml_array.h>
   #include <minilang/ml_json.h>
   #include <minilang/ml_xml.h>
   #include <minilang/ml_time.h>
   #include <minilang/ml_uuid.h>

   int main(int Argc, const char *Argv[]) {
      stringmap_t *Globals = stringmap_new();
      ml_init(Globals);
      ml_sequence_init(Globals); // Recommended for sequence functions
      ml_object_init(Globals); // Recommended for class, enum and flag types

      ml_tasks_init(Globals); // Optional tasks and parallel evaluation (if scheduler is enabled)
      ml_file_init(Globals); // Optional file and directory functions
      ml_math_init(Globals); // Optional maths functions
      ml_array_init(Globals); // Optional multidimensional arrays (requires maths)

      ml_json_init(Globals); // Optional JSON types and functions
      ml_xml_init(Globals); // Optional XML types and functions
      ml_time_init(Globals); // Optional time types and functions
      ml_uuid_init(Globals); // Optional UUID types and functions
      // etc ...
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

The function :c:func:`ml_state` can be used to create a new simple state (with no additional fields).

For example, if :c:expr:`Function` contains a callable :c:struct:`ml_value_t` then the following example can be used to call :mini:`Function(10, "Hello world")`.

.. code-block:: c

   static void ml_main_state_run(ml_state_t *State, ml_value_t *Value) {
      if (ml_is_error(Value)) {
         fprintf(stderr, "%s: %s\n", ml_error_type(Value), ml_error_message(Value));
         ml_source_t Source;
         int Level = 0;
         while (ml_error_source(Value, Level++, &Source)) {
            fprintf(stderr, "\t%s:%d\n", Source.Name, Source.Line);
         }
         exit(1);
      }
   }

   int main(int Argc, const char *Argv[]) {
      ml_state_t *Main = ml_state(NULL);
      Main->run = ml_main_state_run;
      ml_inline(Main, Function, 2, ml_integer(10), ml_cstring("Hello world"));
   }

Another useful state type is a :c:struct:`ml_call_state_t` which holds a number of :c:struct:`ml_value_t`'s. When run, it will call the result as a function, with the supplied :c:struct:`ml_value_t`'s as arguments. For example, the following will load code from the specified file, run it with the supplied arguments as a single list called :mini:`Args` and then exit silently or with an error.

.. code-block:: c

   static void ml_main_state_run(ml_state_t *State, ml_value_t *Value) {
      if (ml_is_error(Value)) {
         fprintf(stderr, "%s: %s\n", ml_error_type(Value), ml_error_message(Value));
         ml_source_t Source;
         int Level = 0;
         while (ml_error_source(Value, Level++, &Source)) {
            fprintf(stderr, "\t%s:%d\n", Source.Name, Source.Line);
         }
         exit(1);
      }
   }

   int main(int Argc, const char *Argv[]) {
      static const char *Parameters[] = {"Args", NULL};
      if (Argc < 2) {
         fprintf(stderr, "Usage: %s <filename> [<arguments> ...]\n", Argv[0]);
         exit(1);
      }
      ml_value_t *Args = ml_list();
      const char *FileName = Argv[1];
      for (int I = 2; I < Argc; ++I) ml_list_append(Args, ml_cstring(Argv[I]));
      ml_state_t *Main = ml_state(NULL);
      Main->run = ml_main_state_run;
      ml_call_state_t *State = ml_call_state(Main, 1);
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

Typically this function is used with :c:func:`ml_compiler_input` to set the input code at each evaluation. Since a single piece of code can contain multiple expressions or declarations, :c:func:`ml_command_evaluate` should be called again within :c:expr:`Caller`'s run function until the value passed to :c:expr:`Caller->run` is :c:var:`MLEndOfInput`.

The following (incomplete) code shows an outline of how to use a Minilang compiler in a read-eval-print-loop (*REPL*).

.. code-block:: c

   #include <minilang/minilang.h>
   #include <minilang/ml_macros.h>

   typedef struct {
      ml_state_t Base;
      ml_compiler_t *Compiler;
      // ...
   } repl_state_t;

   static void repl_state_run(repl_state_t *REPL, ml_value_t *Result) {
      if (Result == MLEndOfInput) return;
      // Do something with Result (e.g. print to console)
      ml_command_evaluate((ml_state_t *)REPL, REPL->Compiler);
   }

   const char *repl_read_line(repl_state_t *REPL) {
      // Return next line(s) of input
   }

   int main() {
      ml_init();
      stringmap_t *Globals = stringmap_new();
      ml_types_init(Globals);
      // Initialize other optional features
      // Add additional globals
      repl_state_t *REPL = new(repl_state_t);
      REPL->Base.run = (ml_state_fn)repl_state_run;
      REPL->Base.Context = &MLRootContext;
      REPL->Compiler = ml_compiler((ml_getter_t)stringmap_search, Globals, (ml_reader_t)repl_read_line, REPL);
      ml_command_evaluate((ml_state_t *)REPL, REPL->Compiler);
   }


