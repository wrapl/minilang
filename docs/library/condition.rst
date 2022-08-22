.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

condition
=========

.. _type-condition:

:mini:`type condition`
   A condition for synchronizing concurrent code.


.. _fun-condition:

:mini:`fun condition(): condition`
   Returns a new condition.


:mini:`meth (Condition: condition):broadcast`
   Signals :mini:`Condition`,  resuming all waiters.


:mini:`meth (Condition: condition):signal`
   Signals :mini:`Condition`,  resuming a single waiter.


:mini:`meth (Condition: condition):wait(Semaphore: semaphore): integer`
   Increments :mini:`Semaphore`,  waits until :mini:`Condition` is signalled,  then decrements :mini:`Semaphore` (waiting if necessary) and returns its value.


