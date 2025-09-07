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

In Minilang, a list is simply a list of values, of any type. List are written between :mini:`[` and :mini:`]`. Lists can also be constructed using the :mini:`list()` constructor which takes a sequence and collects its values into a list.

.. tryit::

	let L := [1, 12, 4, 17, 23]
	L:length
	L:sort
	L[2]

	let L2 := list(1 .. 20 by 3)
	L2:length
	L2:reverse

.. note::

	In Minilang, most methods that reorder the elements of a list (sorting, reversing, etc), are *in-place*, i.e. they modify the list itself instead of creating a new list. They also generally returned the list allowing them to be chained easily. If the original list is required unchanged, create a copy of the list first using :mini:`list()`.

.. tryit::

	let L := [1, 12, 4, 17, 23]
	let L2 := list(L):sort:reverse
	print('L = {L}\n')
	print('L2 = {L2}\n')
