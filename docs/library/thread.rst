.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

thread
======

.. rst-class:: mini-api

:mini:`fun mlthreadport(Arg₁: any)`
   *TBD*


:mini:`meth (Args...: any):thread(Fn: function, Arg₃: any): thread`
   Creates a new thread and calls :mini:`Fn(Args...)` in the new thread.
   All arguments must be thread-safe.


:mini:`fun thread::sleep(Duration: number): nil`
   Causes the current thread to sleep for :mini:`Duration` microseconds.


:mini:`type thread`
   A thread.


:mini:`meth thread()`
   *TBD*


:mini:`meth (Thread: thread):join: any`
   Waits until the thread :mini:`Thread` completes and returns its result.


:mini:`type thread::channel`
   A channel for thread communication.


:mini:`fun thread::channel(Capacity: integer): thread::channel`
   Creates a new channel with capacity :mini:`Capacity`.


:mini:`meth (Channel₁: thread::channel):recv(: thread::channel, ...): tuple[integer, any]`
   Gets the next available message on any of :mini:`Channel₁,  ...,  Channelₙ`,  blocking if :mini:`Channel` is empty. Returns :mini:`(Index,  Message)` where :mini:`Index = 1,  ...,  n`.


:mini:`meth (Channel: thread::channel):send(Message: any): thread::channel`
   Adds :mini:`Message` to :mini:`Channel`. :mini:`Message` must be thread-safe.
   Blocks if :mini:`Channel` is currently full.


:mini:`type thread::condition`
   A condition.


:mini:`fun thread::condition(): thread::condition`
   Creates a new condition.


:mini:`meth (Condition: thread::condition):broadcast: thread::condition`
   Signals all threads waiting on :mini:`Condition`.


:mini:`meth (Condition: thread::condition):signal: thread::condition`
   Signals a single thread waiting on :mini:`Condition`.


:mini:`meth (Condition: thread::condition):wait(Mutex: thread::mutex): thread::condition`
   Waits for a signal on :mini:`Condition`,  using :mini:`Mutex` for synchronization.


:mini:`type thread::mutex`
   A mutex.


:mini:`fun thread::mutex(): thread::mutex`
   Creates a new mutex.


:mini:`meth (Mutex: thread::mutex):lock: thread::mutex`
   Locks :mini:`Mutex`.


:mini:`meth (Mutex: thread::mutex):protect(Value: any): thread::protected`
   Creates a thread-safe (protected) wrapper for :mini:`Value`.


:mini:`meth (Mutex: thread::mutex):unlock: thread::mutex`
   Unlocks :mini:`Mutex`.


:mini:`type thread::port < function`
   *TBD*


:mini:`type thread::protected`
   A thread-safe (protected) wrapper for another value.


:mini:`meth (Protected₁: thread::protected):use(: thread::protected, ..., Function: function): any`
   Locks :mini:`Protected₁:mutex`,  then calls :mini:`Function(Value₁,  ...,  Valueₙ)` where :mini:`Valueᵢ` is the value protected by :mini:`Protectedᵢ`. All :mini:`Protectedᵢ` must be protected by the same :mini:`thread::mutex`.


