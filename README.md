# Minilang is a small but extensible scripting language for embedding in C/C++ applications

Minilang is a simple language designed to be embedded into C/C++ applications with minimal fuss and maximum effect.

## Key Features

Minilang was designed from the ground up to be embedded into C/C++ applications. As a result, it has a rich API for embedding that support a wide range of use cases.

### Minimal Dependencies

Minilang only has one required dependency, the [Hans-Boehm conservative garbage collector](https://github.com/ivmai/bdwgc), which is commonly available in the standard repositories of most Linux distributions, in Homebrew on macOS and easily built from source.

### Code Safety

Minilang's VM is safe, there is no way to access raw memory to perform unsafe type-casting, indexing operations are bounds checked, arguments to internal functions are type-checked, etc. Minilang only provides core functionality by default, and does not provide file or network functions, module loading, or any other potentially unsafe functions.

### Extensible

Although Minilang's core functionality is intentionally restricted, functionality can be easily added by the host application, either globally or each time Minilang code is loaded. For example, there is no built-in `print()` function, one must be added by the host application. This extra step means that the `print()` function can write to a file or log instead if desired, or even send text across a websocket to display in a browser.

### Flexible Code Loading

The C API provides functions for incremental source code loading, allowing Minilang to be used for interactive sessions such as REPL shells and notebooks as well as loading complete Minilang code from files, databases, etc.

### Source Level Debugging

The C API also provides features for source level debugging of Minilang code. The host application can specify a debugger when running any function and the VM will invoke the debugger at breakpoints, errors, etc. The debugger API is flexible, debugging can be implementing in a console, desktop GUI or even across a network.

### Stackless Execution and One-Shot Continuations

Minilang uses a stackless VM for execution. Moreover, every function call passes the calling state to the called function, returns are implemented by resuming the calling state. This provides a number of additional features:

* **Inline Asynchronous Calls** Any Minilang function call can be asynchronous without any change in the calling code.
* **Parallel Asynchronous Calls** Multiple asynchronous calls can also be launched in parallel, with the calling state resumed automatically when all the calls have completed. Minilang is thus suitable for scripting complex workflows in distributed applications.
* **Preemptive Multitasking** Minilang can optionally use a scheduler to swap the running VM state.

### Per-Context Settings

Every Minilang state has an associated context which is inherited by default from the calling state but can be explicitly set by the host application. The context stores local settings such as the current debugger, preemptive scheduler, etc. The host application can register additional context settings.

### Multi-tenant Support

Combining all of the above features, Minilang is ideal for multi-tenant distributed applications.

## History

Minilang was originally designed for [Rabs](https://github.com/wrapl/rabs), an imperative 
parallel build system. As result, it was designed with the following requirements:

* **Minimal dependencies:** Minilang only has one required dependency, the [Hans-Boehm conservative garbage collector](https://github.com/ivmai/bdwgc), which is commonly available in the standard repositories of most Linux distributions, in Homebrew on macOS and easily built from source.
* **Allow callbacks for identifier lookups:** This allows build functions in Rabs to use dynamic (context aware) scoping.
* **Easy to add functions in C:** It is easy to add new functions to Minilang as required.
* **Easy to store Minilang objects in C:** References to build functions can be easily stored and called when required.
* **Full support for closures:** Closures (functions which capture their surrounding scope) make complex build functions simpler to write and store.
* **Closures are hashable:** Checksums can be computed for Minilang functions (using SHA256) in order to check if a target needs to be rebuilt.

Additional features have been added since the original use in Rabs.

* **Configurable builds** Minilang can be built with Rabs (as well as Make) enabling several optional features as desired.
**Module system** Minilang files can be loaded as modules, with imports and exports.
**Gnome introspection suppport** Minilang can be built with support for Gnome introspection, providing automatic bindings for a wide range of libraries including GUI, networking, etc.
**Continuation based implementation** Function calls are implemented using one-short continuation based approach. This adds (optional) support for the following:
  - **Asynchronous calls**
  - **Cooperative multitasking**
  - **Preemptive multitasking**

## Simple example

> example1.c

```c
#include <stdio.h>
#include <minilang/minilang.h>

static ml_value_t *print(void *Data, int Count, ml_value_t **Args) {
	ml_value_t *StringMethod = ml_method("string");
	for (int I = 0; I < Count; ++I) {
		ml_value_t *Result = Args[I];
		if (Result->Type != MLStringT) {
			Result = ml_simple_call(StringMethod, 1, &Result);
			if (Result->Type == MLErrorT) return Result;
			if (Result->Type != MLStringT) return ml_error("ResultError", "string method did not return string");
		}
		fwrite(ml_string_value(Result), 1, ml_string_length(Result), stdout);
	}
	fflush(stdout);
	return MLNil;
}

int main(int Argc, char **Argv) {
	ml_init();
	stringmap_t *Globals = stringmap_new();
	stringmap_insert(Globals, "print", ml_cfunction(NULL, print));
	ml_value_state_t *State = ml_value_state_new(NULL);
	ml_load_file((ml_state_t *)State, (ml_getter_t)stringmap_search, Globals, "example1.mini", NULL);
	if (State->Value->Type == MLErrorT) {
		ml_error_print(State->Value);
		exit(1);
	}
	ml_call((ml_state_t *)State, State->Value, 0, NULL);
	if (State->Value->Type == MLErrorT) {
		ml_error_print(State->Value);
		exit(1);
	}
}
```

> example1.mini

```lua
for X in 1 .. 10 do
	print('{X} ')
end
print('done.\n')
```

> output
```sh
$ gcc -o example1 example1.c -lminilang -lgc
$ ./example1
1 2 3 4 5 6 7 8 9 10 done.
$
```

## Building

### Using make

```console
$ git clone https://github.com/wrapl/minilang
$ cd minilang
$ make -j4
$ make install PREFIX=/usr/local
```

This will build and install a vanilla version of Minilang in `PREFIX`.

### Using Rabs

```console
$ git clone https://github.com/wrapl/minilang
$ cd minilang
$ rabs -p4 -DPREFIX=/usr/local install
```

This will build and install a vanilla version of Minilang in `PREFIX`.

Additional options can be enabled when building Minilang with Rabs. For example, to enable GTK+ support, pass `-DGTK` when building.

```console
$ rabs -p4 -DPREFIX=/usr/local -DGTK install
```

Currently the following optional features are available:

| Build Flags | Description |
| --- | --- |
| `-DMATH` | Adds additional maths functions, including multi-dimensional numeric arrays |
| `-DGTK` | Adds Gnome introspection support |
| `-DCBOR` | Adds support for serializing and deserializing Minilang values to/from CBOR |
| `-DSCHEDULER` | Adds support for preemptive multitasking |
| `-DMODULES` | Adds support for loading Minilang files as modules. Enables `-DSCHEDULER` |
| `-DASM` | Uses assembly code implementations for certain features on supported platforms |
| `-DTABLES` | Adds a table type (similar to a dataframe, datatable, etc). Enables `-DMATH` |
| `-DQUEUES` | Adds a priority queue type |

## Documentation

Full documentation can be found [here](https://minilang.readthedocs.io).