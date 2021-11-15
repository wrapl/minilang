thread
======

:mini:`fun thread(Args...: any, Fn: function): thread`
   Creates a new thread and calls :mini:`Fn(Args...)` in the new thread.

   All arguments must be thread-safe.


:mini:`type thread`
   A thread.


:mini:`meth (Thread: thread):join: any`
   Waits until the thread :mini:`Thread` completes and returns its result.


:mini:`fun thread::sleep(Duration: number): nil`
   Causes the current thread to sleep for :mini:`Duration` microseconds.


:mini:`fun thread::channel(Capacity: integer): thread::channel`
   Creates a new channel with capacity :mini:`Capacity`.


:mini:`type thread::channel`
   A channel for thread communication.


:mini:`meth (Channel: thread::channel):send(Message: any): thread::channel`
   Adds :mini:`Message` to :mini:`Channel`. :mini:`Message` must be thread-safe.

   Blocks if :mini:`Channel` is currently full.


:mini:`meth (Channel₁: thread::channel):recv(...): tuple[integer,any]`
   Gets the next available message on any of :mini:`Channel₁, ..., Channelₙ`, blocking if :mini:`Channel` is empty. Returns :mini:`(Index, Message)` where :mini:`Index = 1, ..., n`.


:mini:`fun thread::mutex(): thread::mutex`
   Creates a new mutex.


:mini:`type thread::mutex`
   A mutex.


:mini:`meth (Mutex: thread::mutex):lock: thread::mutex`
   Locks :mini:`Mutex`.


:mini:`meth (Mutex: thread::mutex):unlock: thread::mutex`
   Unlocks :mini:`Mutex`.


:mini:`fun thread::condition(): thread::condition`
   Creates a new condition.


:mini:`type thread::condition`
   A condition.


:mini:`meth (Condition: thread::condition):wait(Mutex: thread::mutex): thread::condition`
   Waits for a signal on :mini:`Condition`, using :mini:`Mutex` for synchronization.


:mini:`meth (Condition: thread::condition):signal: thread::condition`
   Signals a single thread waiting on :mini:`Condition`.


:mini:`meth (Condition: thread::condition):broadcast: thread::condition`
   Signals all threads waiting on :mini:`Condition`.


