.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

time
====

Provides time and date operations. Depending on how Minilang was built,  may need to be imported using :mini:`import: time("std/time")`.

.. _type-time:

:mini:`type time`
   A time in UTC with nanosecond resolution.


:mini:`meth time(String: string): time`
   Parses the :mini:`String` as a time according to ISO 8601.


:mini:`meth time(String: string, Format: string): time`
   Parses the :mini:`String` as a time according to specified format. The time is assumed to be in local time.


:mini:`meth time(): time`
   Returns the current UTC time.


:mini:`meth time(String: string, Format: string, UTC: boolean): time`
   Parses the :mini:`String` as a time according to specified format. The time is assumed to be in local time unless UTC is :mini:`true`.


:mini:`meth (Start: time) + (Duration: number): time`
   Returns the time :mini:`Duration` seconds after :mini:`Start`.

   .. code-block:: mini

      import: time("std/time")
      time("2022-04-01 12:00:00") + 3600 :> 2022-04-01 13:00:00


:mini:`meth (Start: time) - (Duration: number): time`
   Returns the time :mini:`Duration` seconds before :mini:`Start`.

   .. code-block:: mini

      import: time("std/time")
      time("2022-04-01 12:00:00") - 3600 :> 2022-04-01 11:00:00


:mini:`meth (End: time) - (Start: time): real`
   Returns the time elasped betwen :mini:`Start` and :mini:`End` in seconds.

   .. code-block:: mini

      import: time("std/time")
      time("2022-04-01 12:00:00") - time("2022-04-01 11:00:00")
      :> 3600


:mini:`meth (A: time) <> (B: time): integer`
   Compares the times :mini:`A` and :mini:`B` and returns :mini:`-1`,  :mini:`0` or :mini:`1` respectively.


:mini:`meth (Time: time):nsec: integer`
   Returns the nanoseconds component of :mini:`Time`.


:mini:`meth (Buffer: string::buffer):append(Time: time): string`
   Formats :mini:`Time` as a local time.


:mini:`meth (Buffer: string::buffer):append(Time: time, TimeZone: nil): string`
   Formats :mini:`Time` as a UTC time according to ISO 8601.


:mini:`meth (Buffer: string::buffer):append(Time: time, Format: string): string`
   Formats :mini:`Time` as a local time according to the specified format.


:mini:`meth (Buffer: string::buffer):append(Time: time, Format: string, TimeZone: nil): string`
   Formats :mini:`Time` as a UTC time according to the specified format.


