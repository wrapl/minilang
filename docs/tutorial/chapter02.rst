Chapter 2
=========

Conditional Evaluation
----------------------

.. note::

	For the purposes of conditional evaluation, Minilang only considers whether a value is :mini:`nil` or not. All other values (including :mini:`false`, :mini:`0`, :mini:`""`, :mini:`[]`, etc) are considered as not :mini:`nil`.

Comparison Operations
---------------------

Like most languages, values in Minilang can be compared using infix comparison operators, :mini:`=` (a single ``=``), :mini:`!=`, :mini:`<`, :mini:`>`, :mini:`<=` and :mini:`>=`. Unlike many other languages, comparison operators in Minilang do not return :mini:`true` or :mini:`false`, instead they return their 2nd argument if the comparison is satisfied, and :mini:`nil` is it is not.

.. tryit::

	1 < 2
	1 > 2

	3 = 3
	3 != 3

	4 < 4
	4 <= 4

	5 >= 4
	5 >= 6

:mini:`if`-expressions
......................

Code can be evaluated conditionally in Minilang using :mini:`if`-expressions.

.. tryit::

	for X in [0, false, "", [], nil] do
		if X then
			print(X, " is considered not nil.\n")
		else
			print(X, " is considered nil.\n")
		end
	end

Each branch of an :mini:`if`-expression is a block, and can contain 0 or more variable declarations and expressions.

.. tryit::

	for X in [1, nil] do
		if X then
			let A := 1 + 2
			print("A = ", A, "\n")
		else
			let B := 4 * 5
			print("B = ", B, "\n")
		end
	end

Finally, nearly all constructs in Minilang are expressions, i.e. they result in a value. :mini:`if`-expressions evaluate to the last expression in their evaluated branch. The :mini:`else` branch of an :mini:`if`-expression can be omitted, in which case it is treated as :mini:`nil`.

.. tryit::

	for X in [1, nil] do
		print("Choosing the ", if X then "non-nil" else "nil" end, " branch.\n")
	end

	for X in [1, nil] do
		print("Choosing the ", if X then "non-nil" end, " branch.\n")
	end


:mini:`and`-expressions and :mini:`or`-expressions
..................................................

Code can also be evaluated conditionally in Minilang using :mini:`and`, :mini:`or`. An :mini:`and`-expression evaluates to its second argument if both arguments are not :mini:`nil`, and to :mini:`nil` otherwise. An :mini:`or`-expression evaluates to its first argument if it is not :mini:`nil`, otherwise it evaluates to its second argument. Both :mini:`and`-expressions and :mini:`or`-expressions only evaluate their second argument if required.

.. flat-table:: :mini:`X and Y`
	:header-rows: 1
	:stub-columns: 1

	* -
	  - :mini:`Y = nil`
	  - :mini:`Y != nil`

	* - :mini:`X = nil`
	  - :cspan:`2` :mini:`nil` (:mini:`Y` not evaluated)

	* - :mini:`X != nil`
	  - :mini:`nil`
	  - :mini:`Y`

.. flat-table:: :mini:`X or Y`
	:header-rows: 1
	:stub-columns: 1

	* -
	  - :mini:`Y = nil`
	  - :mini:`Y != nil`

	* - :mini:`X = nil`
	  - :mini:`nil`
	  - :mini:`Y`

	* - :mini:`X != nil`
	  - :cspan:`2` :mini:`X` (:mini:`Y` not evaluated)

.. tryit::

	fun test(V, X) do
		print('{V} = {X}\n')
		ret X
	end

	test("X", 10) and test("Y", nil)

	test("X", 10) and test("Y", 20)

	test("X", nil) and test("Y", nil)

	test("X", nil) and test("Y", 20)

	test("X", 10) or test("Y", nil)

	test("X", 10) or test("Y", 20)

	test("X", nil) or test("Y", nil)

	test("X", nil) or test("Y", 20)


:mini:`not`-expressions and :mini:`xor`-expressions
...................................................

A :mini:`not`-expression returns :mini:`some` if its argument returns :mini:`nil` and return :mini:`nil` otherwise. If exactly one argument of a :mini:`xor`-expression returns a non-:mini:`nil` value, that value is returned, otherwise it returns :mini:`nil`. Both :mini:`not`-expressions and :mini:`xor`-expressions always evaluate their arguments.

.. flat-table:: :mini:`not X`
	:header-rows: 1

	* - :mini:`X = nil`
	  - :mini:`some`

	* - Not :mini:`X != nil`
	  - :mini:`nil`

.. flat-table:: :mini:`X xor Y`
	:header-rows: 1
	:stub-columns: 1

	* -
	  - :mini:`Y = nil`
	  - :mini:`Y != nil`

	* - :mini:`X = nil`
	  - :mini:`nil`
	  - :mini:`Y`

	* - :mini:`X != nil`
	  - :mini:`X`
	  - :mini:`nil`

.. tryit::

	fun test(V, X) do
		print('{V} = {X}\n')
		ret X
	end

	not test("X", 10)

	not test("X", nil)

	test("X", 10) xor test("Y", nil)

	test("X", 10) xor test("Y", 20)

	test("X", nil) xor test("Y", nil)

	test("X", nil) xor test("Y", 20)

