time
====

.. include:: <isonum.txt>

:mini:`type time`
   A time in UTC with nanosecond resolution.


:mini:`meth time()` |rarr| :mini:`time`
   Returns the current UTC time.


:mini:`meth time(String: string)` |rarr| :mini:`time`
   Parses the :mini:`String` as a time according to ISO 8601.


:mini:`meth time(String: string, Format: string)` |rarr| :mini:`time`
   Parses the :mini:`String` as a time according to specified format. The time is assumed to be in local time.


:mini:`meth time(String: string, Format: string, UTC: boolean)` |rarr| :mini:`time`
   Parses the :mini:`String` as a time according to specified format. The time is assumed to be in local time unless UTC is :mini:`true`.


:mini:`meth :nsec(Time: time)` |rarr| :mini:`integer`
   Returns the nanoseconds component of :mini:`Time`.


:mini:`meth string(Time: time)` |rarr| :mini:`string`
   Formats :mini:`Time` as a local time.


:mini:`meth string(Time: time, TimeZone: nil)` |rarr| :mini:`string`
   Formats :mini:`Time` as a UTC time according to ISO 8601.


:mini:`meth string(Time: time, Format: string)` |rarr| :mini:`string`
   Formats :mini:`Time` as a local time according to the specified format.


:mini:`meth string(Time: time, Format: string, TimeZone: nil)` |rarr| :mini:`string`
   Formats :mini:`Time` as a UTC time according to the specified format.


:mini:`meth <>(Arg₁: time, Arg₂: time)`

:mini:`meth -(Arg₁: time, Arg₂: time)`

:mini:`meth +(Arg₁: time, Arg₂: number)`

:mini:`meth -(Arg₁: time, Arg₂: number)`

