.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

semaphore
=========

.. rst-class:: mini-api

:mini:`type semaphore`
   A semaphore for synchronizing concurrent code.


:mini:`fun semaphore(Initial?: integer): semaphore`
   Returns a new semaphore with initial value :mini:`Initial` or :mini:`1` if no initial value is specified.
   :integer Initial?: : integer


:mini:`meth (Semaphore: semaphore):signal: integer`
   Increments the internal value in :mini:`Semaphore`,  resuming any waiters. Returns the new value.


:mini:`meth (Semaphore: semaphore):value: integer`
   Returns the internal value in :mini:`Semaphore`.


:mini:`meth (Semaphore: semaphore):wait: integer`
   Waits until the internal value in :mini:`Semaphore` is postive,  then decrements it and returns the new value.


