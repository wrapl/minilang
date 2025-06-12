Chapter 2
=========

Conditional Evaluation
----------------------

.. note::

	For the purposes of conditional evaluation, Minilang only considers whether a value is :mini:`nil` or not. All other values (including :mini:`false`, :mini:`0`, :mini:`""`, :mini:`[]`, etc) are considered as not :mini:`nil`. 

Comparison Operations
---------------------

Like most languages, values in Minilang can be compared using infix comparison operators, :mini:`=` (a single ``=``), :mini:`!=`, :mini:`<`, :mini:`>`, :mini:`<=` and :mini:`>=`. Unlike most languages, comparison operators in Minilang do not return :mini:`true` or :mini:`false`, instead they return their 2nd argument if the comparison is satisfied, and :mini:`nil` is it is not.

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


:mini:`and`, :mini:`or` and :mini:`not`-expressions.
.................................................... 


