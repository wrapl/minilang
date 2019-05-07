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
