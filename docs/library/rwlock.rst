.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

rwlock
======

.. _type-rwlock:

:mini:`type rwlock`
   A read-write lock for synchronizing concurrent code.


.. _fun-rwlock:

:mini:`fun rwlock(): rwlock`
   Returns a new read-write lock.


:mini:`meth (Lock: rwlock):rdlock`
   Locks :mini:`Lock` for reading,  waiting if there are any writers using or waiting to use :mini:`Lock`.


:mini:`meth (Lock: rwlock):unlock`
   Unlocks :mini:`Lock`,  resuming any waiting writers or readers (giving preference to writers).


:mini:`meth (Lock: rwlock):wrlock`
   Locks :mini:`Lock` for reading,  waiting if there are any readers or other writers using :mini:`Lock`.


