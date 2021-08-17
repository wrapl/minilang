tasks
=====

.. include:: <isonum.txt>

:mini:`fun tasks(Max?: integer, Min?: integer)` |rarr| :mini:`tasks`
   Creates a new :mini:`tasks` set.

   If specified, at most :mini:`Max` functions will be called in parallel (the default is unlimited).

   If :mini:`Min` is also specified then the number of running tasks must drop below :mini:`Min` before more tasks are launched.


:mini:`type tasks < function`
   A dynamic set of tasks (function calls). Multiple tasks can run in parallel (depending on the availability of a scheduler and/or asynchronous function calls).


:mini:`meth :add(Tasks: tasks, Args...: any, Function: any)`
   Adds the function call :mini:`Function(Args...)` to a set of tasks.

   Adding a task to a completed tasks set returns an error.


:mini:`meth :wait(Tasks: tasks)` |rarr| :mini:`nil` or :mini:`error`
   Waits until all of the tasks in a tasks set have returned, or one of the tasks has returned an error (which is then returned from this call).


:mini:`fun parallel(Sequence: any, Max?: integer, Min?: integer, Function: function)` |rarr| :mini:`nil` or :mini:`error`
   Iterates through :mini:`Sequence` and calls :mini:`Function(Key, Value)` for each :mini:`Key, Value` pair produced **without** waiting for the call to return.

   The call to :mini:`parallel` returns when all calls to :mini:`Function` return, or an error occurs.

   If :mini:`Max` is given, at most :mini:`Max` calls to :mini:`Function` will run at a time by pausing iteration through :mini:`Sequence`.

   If :mini:`Min` is also given then iteration will be resumed only when the number of calls to :mini:`Function` drops to :mini:`Min`.


