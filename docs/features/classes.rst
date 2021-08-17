Classes
=======

:mini:`class(Arg₁, Arg₂, ...)`
   Creates a new class with additional properties based on the types of :mini:`Arg₁, Arg₂, ...`:

   :mini:`class`
      Adds a parent class. Multiple parent classes are allowed.
   
   :mini:`method`
      Adds a field. Instances of this class will have space allocated for all fields, fields cannot be added or removed from instances later. Fields are accessed using the associated method.
      
   :mini:`Name is Value`
      Named arguments add shared values to the class. If :mini:`Class` is a class, then :mini:`Class::Name` will return the shared value called *Name*.

Certain shared values have special meaning. If :mini:`c` is a class, then:

* The name :mini:`c::new` is always set to a function equivalent to the following:
  
  .. code-block:: mini
  
   fun(Arg₁, Arg₂, ...) do
      let Instance := new instance of c
      c::init(Instance, Arg₁, Arg₂, ...)
      ret Instance
   end
  
This cannot be overridden, if *new* is passed as a named argument to :mini:`class()`, it is ignored.

* The value of :mini:`c::init` is used as the initializer and should be a callable value (function, method, etc). This value is called by :mini:`c::new` to initialize a new instance :mini:`c` with the given arguments.

  If *init* is not set, a default initializer is set which assigns positional arguments to the instance fields in order. Any named arguments are assigned to the corresponding field by name. 

* The value of :mini:`c::of` is used as the constructor and should be a callable value (function, method, etc). This value is called when the class is called as a function, i.e. :mini:`c(...)` is equivalent to :mini:`c::of(...)`. 

  If *of* is not set, a default constructor is set which simply calls :mini:`c::new`.