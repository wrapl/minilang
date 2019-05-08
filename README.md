# Minilang

Minilang is a simple language designed to be embedded into C/C++ applications with 
minimal fuss. It is intentionally lacking in many built in features, but is easy to extend 
with new functionality.

<aside class="warning">
    Minilang uses the <a href="https://github.com/ivmai/bdwgc">Hans-Boehm conservative
    garbage collector</a>. This simplifies memory management in most cases, but may not
    be compatible with all use cases. In the future, the option to use a different form
    of memory management may be added to Minilang. 
</aside>

## Introduction

Minilang was originally designed for [Rabs](https://github.com/wrapl/rabs), an imperative 
parallel build system. As result, it was designed with the following requirements:

* **Minimal dependencies:** Minilang only has one dependency, the [Hans-Boehm conservative
  garbage collector](https://github.com/ivmai/bdwgc), which is commonly available in 
  the standard repositories of most Linux distributions, and in Homebrew on macOS.
* **Allow callbacks for identifier lookups:** This allows build functions in Rabs to 
  use dynamic (context aware) scoping.
* **Easy to add functions in C:** It is easy to add new functions to Minilang as required.
* **Easy to store Minilang objects in C:** References to build functions can be easily 
  stored and called when required.
* **Full support for closures:** Closures (functions which capture their surrounding 
  scope) make complex build functions simpler to write and store.
* **Closures are hashable:** Checksums can be computed for Minilang functions (using 
  sha256) in order to check if a target needs to be rebuilt.

## Simple example

> example1.c

```c
#include <minilang.h>

static ml_value_t *print(void *Data, int Count, ml_value_t **Args) {
	ml_value_t *StringMethod = ml_method("string");
	for (int I = 0; I < Count; ++I) {
		ml_value_t *Result = Args[I];
		if (Result->Type != MLStringT) {
			Result = ml_call(StringMethod, 1, &Result);
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
	stringmap_insert(Globals, "print", ml_function(NULL, print));
	ml_value_t *Value = ml_load(stringmap_search, Globals, "example1.mini");
	if (ml_is_error(Value)) {
		ml_error_print(Value);
		exit(1);
	}
	Value = ml_call(Value, 0, NULL);
	if (ml_is_error(Value)) {
		ml_error_print(Value);
		exit(1);
	}
}
```

> example1.mini

```lua
for X in 1 .. 10 do
	print('X ')
end
print('done.\n')
```

> output
```sh
$ gcc -o example1 example1.c
$ ./example1
1 2 3 4 5 6 7 8 9 10 done.
$
```