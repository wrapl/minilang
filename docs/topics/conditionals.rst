Conditionals
============

*Minilang* has several constructs for conditional evaluation of code.

.. important::

   All conditional evaluation constructs treat **only** :mini:`nil` as the false-like or negative value, and **all other values** as true-like or positive. Boolean values (:mini:`true` and :mini:`false`) are not :mini:`nil` and hence treated as true-like or positive.

And / or expressions
--------------------

These expressions check their left argument for :mini:`nil` and only evaluate their right argument if necessary.

.. parser-rule-diagram:: expression ( 'and' | 'or' ) expression

They operate as follows:

.. list-table::
   :header-rows: 1

   * - Expression
     - Result of :mini:`X`
     - :mini:`Y` evaluated
     - Result

   * - :mini:`X and Y`
     - :mini:`nil`
     - No
     - :mini:`X` (:mini:`nil`)

   * - :mini:`X and Y`
     - Not :mini:`nil`
     - Yes
     - :mini:`Y`

   * - :mini:`X or Y`
     - :mini:`nil`
     - Yes
     - :mini:`Y`

   * - :mini:`X or Y`
     - Not :mini:`nil`
     - No
     - :mini:`X`

If expressions
--------------

An :mini:`if`-expression evaluates its condition expressions and evaluates the :mini:`then`-block if the condition value is not :mini:`nil`. Otherwise it evaluates the :mini:`else`-block is present, or :mini:`nil` otherwise. Additional :mini:`elseif` branches can used to avoid nesting the :mini:`else` blocks.

The condition value itself can optionally contain a variable declaration using :mini:`let` or :mini:`var` (including an unpacking declaration) allowing a computed value such as a regular expression match to be used as the condition without needing an extra declaration.

.. parser-rule-diagram:: 'if' ( ( 'var' | 'let' ) ( identifier | ( '(' ( identifier | '_' ) ( ',' ( identifier | '_' ) )* ')' ) ) ':=' )? expression 'then' block ( 'elseif' ( ( 'var' | 'let' ) ( identifier | ( '(' ( identifier | '_' ) ( ',' ( identifier | '_' ) )* ')' ) ) ':=' )? expression 'then' block )* ( 'else' block )? 'end'

When expressions
----------------

A :mini:`when`-expression evaluates an expression and then evaluates a number of conditions in turn using the result, until one of the conditions returns non-:mini:`nil`. The corresponding block is then evaluated. If no condition returns non-:mini:`nil` then the :mini:`else`-block is evaluted if present, otherwise :mini:`nil` is returned.

.. parser-rule-diagram:: 'when' expression ( ( 'is' ( 'nil' | ( 'in' | operator )? expression ) ( ',' ( 'nil' | ( 'in' | operator )? expression ) )* ) 'do' block )+ ( 'else' block )? 'end'


