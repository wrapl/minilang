.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

pqueue
======

.. _type-pqueue:

:mini:`type pqueue < sequence`
   A priority queue with values and associated priorities.


:mini:`meth pqueue(Greater: function): pqueue`
   Returns a new priority queue using :mini:`Greater` to compare priorities.


:mini:`meth pqueue(): pqueue`
   Returns a new priority queue using :mini:`>` to compare priorities.


:mini:`meth (PQueue: pqueue):count: integer`
   Returns the number of entries in :mini:`PQueue`.


:mini:`meth (PQueue: pqueue):insert(Value: any, Priority: any): pqueue::entry`
   Creates and returns a new entry in :mini:`PQueue` with value :mini:`Value` and priority :mini:`Priority`.


:mini:`meth (PQueue: pqueue):next: pqueue::entry | nil`
   Removes and returns the highest priority entry in :mini:`PQueue`,  or :mini:`nil` if :mini:`PQueue` is empty.


:mini:`meth (PQueue: pqueue):peek: pqueue::entry | nil`
   Returns the highest priority entry in :mini:`PQueue` without removing it,  or :mini:`nil` if :mini:`PQueue` is empty.


.. _type-pqueue-entry:

:mini:`type pqueue::entry`
   A entry in a priority queue.


:mini:`meth (Entry: pqueue::entry):adjust(Priority: any): pqueue::entry`
   Changes the priority of :mini:`Entry` to :mini:`Priority`.


:mini:`meth (Entry: pqueue::entry):lower(Priority: any): pqueue::entry`
   Changes the priority of :mini:`Entry` to :mini:`Priority` only if its current priority is greater than :mini:`Priority`.


:mini:`meth (Entry: pqueue::entry):priority: any`
   Returns the priority associated with :mini:`Entry`.


:mini:`meth (Entry: pqueue::entry):queued: pqueue::entry | nil`
   Returns :mini:`Entry` if it is currently in the priority queue,  otherwise returns :mini:`nil`.


:mini:`meth (Entry: pqueue::entry):raise(Priority: any): pqueue::entry`
   Changes the priority of :mini:`Entry` to :mini:`Priority` only if its current priority is less than :mini:`Priority`.


:mini:`meth (Entry: pqueue::entry):remove: pqueue::entry`
   Removes :mini:`Entry` from its priority queue.


:mini:`meth (Entry: pqueue::entry):requeue: pqueue::entry`
   Adds :mini:`Entry` back into its priority queue if it is not currently in the queue.


:mini:`meth (Entry: pqueue::entry):value: any`
   Returns the value associated with :mini:`Entry`.


