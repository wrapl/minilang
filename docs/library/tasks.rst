.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

tasks
=====

.. rst-class:: mini-api

.. _fun-parallel:

:mini:`fun parallel(Sequence: any, Max?: integer, Min?: integer, Fn: function): nil | error`
   Iterates through :mini:`Sequence` and calls :mini:`Fn(Key,  Value)` for each :mini:`Key,  Value` pair produced **without** waiting for the call to return.
   The call to :mini:`parallel` returns when all calls to :mini:`Fn` return,  or an error occurs.
   If :mini:`Max` is given,  at most :mini:`Max` calls to :mini:`Fn` will run at a time by pausing iteration through :mini:`Sequence`.
   If :mini:`Min` is also given then iteration will be resumed only when the number of calls to :mini:`Fn` drops to :mini:`Min`.


:mini:`meth (Fn: function):else(Else: function): task`
   *TBD*


:mini:`meth (Fn: function):on(On: function): task`
   *TBD*


:mini:`meth (Fn: function):then(Then: function): task`
   Equivalent to :mini:`task(Fn,  call -> Then)`.


:mini:`meth (Fn: function):then(Then: function, Else: function): task`
   *TBD*


.. _fun-buffered:

:mini:`fun buffered(Sequence: sequence, Size: integer, Fn: function): sequence`
   Returns the sequence :mini:`(Kᵢ,  Fn(Kᵢ,  Vᵢ))` where :mini:`Kᵢ,  Vᵢ` are the keys and values produced by :mini:`Sequence`. The calls to :mini:`Fn` are done in parallel,  with at most :mini:`Size` calls at a time. The original sequence order is preserved (using an internal buffer).

   .. code-block:: mini

      list(buffered(1 .. 10, 5, tuple))
      :> [(1, 1), (2, 2), (3, 3), (4, 4), (5, 5), (6, 6), (7, 7), (8, 8), (9, 9), (10, 10)]


.. _type-task:

:mini:`type task < function`
   A task representing a value that will eventually be completed.


:mini:`meth task(): task`
   Returns a task. The task should eventually be completed with :mini:`Task:done()` or :mini:`Task:error()`.


:mini:`meth task(Arg₁: any, ..., Argₙ: any, Fn: function): task`
   Returns a task which calls :mini:`Fn(Arg₁,  ...,  Argₙ)`.


:mini:`meth (Task: task):done(Result: any): any | error`
   Completes :mini:`Task` with :mini:`Result`,  resuming any waiting code. Raises an error if :mini:`Task` is already complete.


:mini:`meth (Task: task):error(Type: string, Message: string): any | error`
   Completes :mini:`Task` with an :mini:`error(Type,  Message)`,  resuming any waiting code. Raises an error if :mini:`Task` is already complete.


:mini:`meth (Task: task):wait: any | error`
   Waits until :mini:`Task` is completed and returns its result.


.. _type-tasks:

:mini:`type tasks < function`
   A dynamic set of tasks (function calls). Multiple tasks can run in parallel (depending on the availability of a scheduler and/or asynchronous function calls).


:mini:`meth tasks(Main: function): tasks`
   Creates a new :mini:`tasks` set with no limits.


:mini:`meth tasks(MaxRunning: integer, Main: function): tasks`
   Creates a new :mini:`tasks` set which will run at most :mini:`MaxRunning` child tasks in parallel.


:mini:`meth tasks(MaxRunning: integer, MaxPending: integer, Main: function): tasks`
   Creates a new :mini:`tasks` set which will run at most :mini:`MaxRunning` child tasks in parallel.
   
   At most :mini:`MaxPending` child tasks will be queued. Calls to add child tasks will wait until there some tasks are cleared.


