Chapter 3
=========

Sequences
---------

Given a *sequence* of values, we can evaluate a block of code for each value in the sequence. There are a number of different sequence types in Minilang including lists, maps, sets, are more. Minilang also provides methods for forming new sequences from existing sequences by transforming or filtering values. This will described in more detail later, for now we will consider ranges, lists and maps.

Ranges
......

They are several types of ranges in Minilang, in general they all have a starting value and either an ending value or number of steps. Numeric ranges can optionally have a step size, if omitted this defaults to :mini:`1`.

.. tryit::

	for X in 1 .. 10 do
		print('X = {X}\n')
	end

	for X in 1 .. 10 by 2 do
		print('X = {X}\n')
	end

	for X in 1 .. 10 in 8 do
		print('X = {X}\n')
	end

	for X in "A" .. "H" do
		print('X = {X}\n')
	end

Lists
.....

In Minilang, a list is simply a list of values, of any type. List are written between :mini:`[` and :mini:`]`. Lists can also be created using the :mini:`list()` constructor which takes a sequence and collects its values into a list.

.. tryit::

	let L := [1, 12, 4, 17, 23]
	L:length
	L:sort
	L[2]

	let L2 := list(1 .. 20 by 3)
	L2:length
	L2:reverse

	let L3 := list("the cat slept on the bed")

.. note::

	In Minilang, most methods that reorder the elements of a list (sorting, reversing, etc), are *in-place*, i.e. they modify the list itself instead of creating a new list. They also generally returned the list allowing them to be chained easily. If the original list is required unchanged, create a copy of the list first using :mini:`list()`.

.. tryit::

	let L := [1, 12, 4, 17, 23]
	let L2 := list(L):sort:reverse
	print('L = {L}\n')
	print('L2 = {L2}\n')

They are numerous other methods available for lists, they can be found here :doc:`/library/list`.

.. tryit::

	let L := []

	L:put(1)

	L:put(2)

	L:push(3)

	L:push(4)

	L:pop

	L:pull

	L:pop

	L:pull

	L:pop

Maps
....

A map is a collection of key-value pairs, where the keys are unique. Maps can be written between :mini:`{` and :mini:`}`, or created using the :mini:`map()` constructor which takes the keys and values from a sequence and collects them into a map.

.. note::

	Sequences in Minilang are always expected to contain or generate *keys* and *values*. Usually the values of a sequence are obvious. On the other hand, although some sequences have obvious keys (e.g. maps), often the keys are implicit. The keys of numeric ranges, strings and lists are :mini:`1, 2, ...`.

.. tryit::

	let M := {"A" is 100, "B" is [1, 2, 3], "C" is "Yes"}
	M["A"]

	M["B"]

	M["C"]

	M["D"]

	let M2 := map(M)

	let M3 := map(1 .. 10 in 7)

	let M4 := map("the cat slept on the bed")

.. note::

	Maps return :mini:`nil` when indexed with a key that is not present in the map. It is possible (but not recommended) to store :mini:`nil` as the value in a map and to use :mini:`Key in Map` to distinguish between a missing key and a key associated with :mini:`nil`.

Strings
.......

Strings in Minilang are also sequences. When used as a sequence, they generate the individual characters as values, with :mini:`1, 2, ...` as keys.

:mini:`for`-expressions
-----------------------

It is possible to iterate over any sequence in Minilang using a :mini:`for`-expression. If a single variable is declared in the :mini:`for`-declaration, it will be assigned the values of the sequence at each iteration. If two variables are declared, then the first will be assigned the keys and the second the values of the sequence.

.. tryit::

	let M := {}
	for I, C in "the cat snored as she slept" do
		print('I = {I}, C = {C}\n')
		M[C] := I
	end

	for C, I in M do
		print('I = {I}, C = {C}\n')
	end

As mentioned before, nearly every construct in Minilang is an expression with a vaule; a :mini:`for`-expression evaluates to :mini:`nil` when it completes normally. A :mini:`for`-expression can be exited at any step using an :mini:`exit`-expression. An :mini:`exit`-expression can optionally take another expression, the result of which will be the result of the :mini:`for`-expression. It is also possible to skip to the next iteration using :mini:`next`.

.. tryit::

	let X := for I in 1 .. 1000000 do
		if I * I > 50 do
			exit I
		end
	end
	print('{X} * {X} > 50\n')
