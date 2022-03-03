.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

tasks
=====

.. _type-task:

:mini:`type task`
   A task representing a value that will eventually be completed.


:mini:`meth task(): task`
   Returns a task. The task should eventually be completed with :mini:`Task:done()` or :mini:`Task:error()`.


:mini:`meth task(Fn: function, Args: any, ...): task`
   Calls :mini:`Fn(Args)` and immediately returns a task that will eventually be completed with the result of the call.


:mini:`meth (Task: task):wait: any | error`
   Waits until :mini:`Task` is completed and returns its result.


:mini:`meth (Task: task):done(Result: any): any | error`
   Completes :mini:`Task` with :mini:`Result`,  resuming any waiting code. Raises an error if :mini:`Task` is already complete.


:mini:`meth (Task: task):error(Type: string, Message: string): any | error`
   Completes :mini:`Task` with an :mini:`error(Type,  Message)`,  resuming any waiting code. Raises an error if :mini:`Task` is already complete.


.. _fun-tasks:

:mini:`fun tasks(Max?: integer, Min?: integer): tasks`
   Creates a new :mini:`tasks` set.

   If specified,  at most :mini:`Max` functions will be called in parallel (the default is unlimited).

   If :mini:`Min` is also specified then the number of running tasks must drop below :mini:`Min` before more tasks are launched.


.. _type-tasks:

:mini:`type tasks < function`
   A dynamic set of tasks (function calls). Multiple tasks can run in parallel (depending on the availability of a scheduler and/or asynchronous function calls).


:mini:`meth (Tasks: tasks):add(Fn: function, Args: any, ...)`
   Adds the function call :mini:`Fn(Args...)` to a set of tasks. Raises an error if :mini:`Tasks` is already complete.


:mini:`meth (Tasks: tasks):wait: nil | error`
   Waits until all of the tasks in a tasks set have returned,  or one of the tasks has returned an error (which is then returned from this call).


.. _fun-parallel:

:mini:`fun parallel(Sequence: any, Max?: integer, Min?: integer, Function: function): nil | error`
   Iterates through :mini:`Sequence` and calls :mini:`Function(Key,  Value)` for each :mini:`Key,  Value` pair produced **without** waiting for the call to return.

   The call to :mini:`parallel` returns when all calls to :mini:`Function` return,  or an error occurs.

   If :mini:`Max` is given,  at most :mini:`Max` calls to :mini:`Function` will run at a time by pausing iteration through :mini:`Sequence`.

   If :mini:`Min` is also given then iteration will be resumed only when the number of calls to :mini:`Function` drops to :mini:`Min`.


