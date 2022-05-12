Functions
=========

In general, a function in *Minilang* is any value that can be called with 0 or more arguments, causing some computation or evaluation resulting in a single value (or an error). Functions in *Minilang* are first-class values that can be assigned to variables, stored in lists / maps, etc and passed to other functions as arguments. They can also be composed using various infix operators. Some types of values such as integers, methods and types are also functions in *Minilang*.

Closures
--------

Functions are created in *Minilang* using a :mini:`fun`-expression.

.. parser-rule-diagram:: 'fun' ( '(' (
   ( ( 'ref' | 'var' )? identifier ( ':' expression )? ( ':=' expression )? ( ',' ( 'ref' | 'var' )? identifier ( ':' expression )? ( ':=' expression )? )* ( ',' '[' identifier ']' )? ( ',' '{' identifier '}' )? ) |
   ( '[' identifier ']' ( ',' '{' identifier '}' )? ) |
   ( '{' identifier '}' )
   )? ')' ( ':' expression )? )? expression

The final expression in a :mini:`fun`-expression is the *body* of the function. The body can reference variables declared outside the function; when a :mini:`fun`-expression is evaluated, it returns a *closure* which combines the function code with the current values of any such variables.

When a closure is called, the supplied arguments are bound to the function parameters before the body is evaluated. The following rules are applied when binding arguments to parameters, starting with the first argument and parameter.

* If a *list* parameter (written as :mini:`[Identifier]`) is present, it is assigned a new empty list
* If a *map* parameter (written as :mini:`{Identifier}`) is present, it is assigned a new empty map
* For each argument

  * Otherwise if the next argument is named then

    * if is a parameter with the same name then the argument is bound to the parameter,
    * otherwise if there is a *map* parameter, the named argument is added to that parameter's value,
    * otherwise an error is returned.

  * Otherwise if there is another normal (not a *map* or *list*) parameter, the argument is bound to the parameter

    * if a type was specified for the parameter (using :mini:`: expression`), the argument type is checked against the specified type and if it is not the same or a sub-type, an error is returned
    * if :mini:`ref` is present, the argument is not dereferenced first, allowing the original variable to be modified by assigning to the paramter,
    * otherwise if :mini:`var` is present, the argument is dereferenced and put into a new variable that is bound to the parameter, allowing the parameter to be reassigned but not affected the original value,
    * otherwise the argument is dereferenced and bound to the parameter

  * Otherwise if there is a *list* parameter, the argument is added to that parameter's value,
  * Otherwise the argument is ignored.

* All remaining normal parameters are set to :mini:`nil`
* Any parameters which have been bound to :mini:`nil` and have default expressions (written as :mini:`:= expression`) are assigned the value of their default expressions (evaluated each time the closure is called)
* Finally the closure body is evaluated
* If a type was specified for the closure (using :mini:`: expression`), the type of the result is checked and an error returned if it does not match
* Otherwise if the result types matches or no type was specified, the result is returned

Calling Functions
-----------------

Functions are called in *Minilang* using traditional postfix notation.

Partial Functions
-----------------

Simple functions can be written in *Minilang* using blank expressions, written as a single underscore :mini:`_`.