.. include:: <isonum.txt>

queue
=====

:mini:`type queue::entry`
   A entry in a priority queue.


:mini:`type queue < sequence`
   A priority queue with values and associated priorities.


:mini:`meth queue(): queue`
   Returns a new queue using :mini:`<>` to compare priorities.


:mini:`meth (Queue: queue):insert(Value: any, Priority: any): queue::entry`
   Creates and returns a new entry in :mini:`Queue` with value :mini:`Value` and priority :mini:`Priority`.


:mini:`meth (Queue: queue):next: queue::entry | nil`
   Removes and returns the next entry in :mini:`Queue`,  or :mini:`nil` if :mini:`Queue` is empty.


:mini:`meth (Queue: queue):count: integer`
   Returns the number of entries in :mini:`Queue`.


:mini:`meth (Entry: queue::entry):requeue: queue::entry`
   Adds :mini:`Entry` back into its queue if it is not currently in the queue.


:mini:`meth (Entry: queue::entry):adjust(Priority: any): queue::entry`
   Changes the priority of :mini:`Entry` to :mini:`Priority`.


:mini:`meth (Entry: queue::entry):raise(Priority: any): queue::entry`
   Changes the priority of :mini:`Entry` to :mini:`Priority` only if its current priority is less than :mini:`Priority`.


:mini:`meth (Entry: queue::entry):lower(Priority: any): queue::entry`
   Changes the priority of :mini:`Entry` to :mini:`Priority` only if its current priority is greater than :mini:`Priority`.


:mini:`meth (Entry: queue::entry):remove: queue::entry`
   Removes :mini:`Entry` from its queue.


:mini:`meth (Entry: queue::entry):value: any`
   Returns the value associated with :mini:`Entry`.


:mini:`meth (Entry: queue::entry):priority: any`
   Returns the priority associated with :mini:`Entry`.


:mini:`meth (Entry: queue::entry):queued: queue::entry | nil`
   Returns :mini:`Entry` if it is currently in the queue,  otherwise returns :mini:`nil`.


