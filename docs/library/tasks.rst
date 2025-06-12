.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

tasks
=====

.. rst-class:: mini-api

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


:mini:`fun buffered(Sequence: sequence, Size: integer, Fn: function): sequence`
   Returns the sequence :mini:`(Kᵢ,  Fn(Kᵢ,  Vᵢ))` where :mini:`Kᵢ,  Vᵢ` are the keys and values produced by :mini:`Sequence`. The calls to :mini:`Fn` are done in parallel,  with at most :mini:`Size` calls at a time. The original sequence order is preserved (using an internal buffer).

   .. code-block:: mini

      list(buffered(1 .. 10, 5, tuple))
      :> [(1, 1), (2, 2), (3, 3), (4, 4), (5, 5), (6, 6), (7, 7), (8, 8), (9, 9), (10, 10)]


:mini:`fun diffused(Sequence: sequence, Size: integer, Fn: function): sequence`
   Returns the sequence :mini:`(Kᵢ,  Fn(Kᵢ,  Vᵢ))` where :mini:`Kᵢ,  Vᵢ` are the keys and values produced by :mini:`Sequence`. The calls to :mini:`Fn` are done in parallel,  with at most :mini:`Size` calls at a time. The original sequence order is not preserved.

   .. code-block:: mini

      list(diffused(1 .. 10, 5, tuple))
      :> [(1, 1), (2, 2), (3, 3), (4, 4), (5, 5), (6, 6), (7, 7), (8, 8), (9, 9), (10, 10)]


:mini:`type task < function`
   A task representing a value that will eventually be completed.


:mini:`meth task(): task`
   Returns a task. The task should eventually be completed with :mini:`Task:done()` or :mini:`Task:error()`.


:mini:`meth task(Arg₁: any, ..., Argₙ: any, Fn: function): task`
   Returns a task which calls :mini:`Fn(Arg₁,  ...,  Argₙ)`.


:mini:`meth (Task₁: task) * (Task₂: task): task::set`
   Returns a :mini:`task::set` that completes when all of its sub tasks complete,  or any raises an error.


:mini:`meth (Task₁: task) + (Task₂: task): task::set`
   Returns a :mini:`task::set` that completes when any of its sub tasks complete,  or any raises an error.


:mini:`meth (Task: task):done(Result: any): any | error`
   Completes :mini:`Task` with :mini:`Result`,  resuming any waiting code. Raises an error if :mini:`Task` is already complete.


:mini:`meth (Task: task):error(Type: string, Message: string): any | error`
   Completes :mini:`Task` with an :mini:`error(Type,  Message)`,  resuming any waiting code. Raises an error if :mini:`Task` is already complete.


:mini:`meth (Task: task):wait: any | error`
   Waits until :mini:`Task` is completed and returns its result.


:mini:`meth (Task: task):wait(Arg₂: real): any | error`
   Waits until :mini:`Task` is completed and returns its result.


:mini:`meth (Tasks: task::list):wait: any | error`
   Waits until all the tasks in :mini:`Tasks` are completed or any task returns an error.


:mini:`type task::queue < function`
   A queue of tasks that can run a limited number of tasks at once.
   
   :mini:`fun (Queue: task::queue)(Arg₁,  ...,  Argₙ,  Fn): task`
      Returns a new task that calls :mini:`Fn(Arg₁,  ...,  Argₙ)`. The task will be delayed if :mini:`Queue` has reached its limit.


:mini:`meth task::queue(MaxRunning: integer): task::queue`
   Returns a new task queue which runs at most :mini:`MaxRunning` tasks at a time.


:mini:`meth task::queue(MaxRunning: integer, Callback: function): task::queue`
   Returns a new task queue which runs at most :mini:`MaxRunning` tasks at a time.


:mini:`meth (Arg₁: task::queue):cancel`
   *TBD*


:mini:`type task::set < task`
   A task combining a set of sub tasks.


