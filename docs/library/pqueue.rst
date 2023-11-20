.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

pqueue
======

.. rst-class:: mini-api

:mini:`type pqueue < sequence`
   A priority queue with values and associated priorities.


:mini:`meth pqueue(Greater: function): pqueue`
   Returns a new priority queue using :mini:`Greater` to compare priorities.


:mini:`meth pqueue(): pqueue`
   Returns a new priority queue using :mini:`>` to compare priorities.


:mini:`meth (Queue: pqueue):count: integer`
   Returns the number of entries in :mini:`Queue`.


:mini:`meth (Queue: pqueue):insert(Value: any, Priority: any): pqueue::entry`
   Creates and returns a new entry in :mini:`Queue` with value :mini:`Value` and priority :mini:`Priority`.


:mini:`meth (Queue: pqueue):keep(Target: integer, Value: any, Priority: any): pqueue::entry | nil`
   Creates and returns a new entry in :mini:`Queue` with value :mini:`Value` and priority :mini:`Priority` if either :mini:`Queue`
   has fewer than :mini:`Target` entries or :mini:`Priority` is lower than the current highest priority entry in :mini:`Queue`
   (removing the current highest priority entry in this case).
   
   Returns the entry removed from :mini:`Queue` or :mini:`nil` if no entry was removed.


:mini:`meth (Queue: pqueue):next: pqueue::entry | nil`
   Removes and returns the highest priority entry in :mini:`Queue`,  or :mini:`nil` if :mini:`Queue` is empty.


:mini:`meth (Queue: pqueue):peek: pqueue::entry | nil`
   Returns the highest priority entry in :mini:`Queue` without removing it,  or :mini:`nil` if :mini:`Queue` is empty.


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


