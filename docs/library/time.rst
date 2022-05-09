.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

time
====

Provides time and date operations. Depending on how Minilang was built,  may need to be imported using :mini:`import: time("std/time")`.

.. _fun-time-zone-list:

:mini:`fun time::zone::list(): list[string]`
   Returns a list of available time zone names.


.. _fun-time-mdays:

:mini:`fun time::mdays(Year: integer, Month: integer): integer`
   *TBD*


.. _type-time:

:mini:`type time`
   An instant in time with nanosecond resolution.


.. _value-time-zones:

:mini:`def time::zones: time::zones`
   The set of available time zones.


:mini:`meth time(Year: integer, Month: integer, Day: integer, Hour: integer, Minute: integer, Second: integer, TimeZone: nil): time`
   Returns the time specified by the provided components in UTC.


:mini:`meth time(String: string): time`
   Parses the :mini:`String` as a time according to ISO 8601.


:mini:`meth time(Year: integer, Month: integer, Day: integer): time`
   Returns the time specified by the provided components in the local time.


:mini:`meth time(String: string, Format: string): time`
   Parses the :mini:`String` as a time according to specified format. The time is assumed to be in local time.


:mini:`meth time(Year: integer, Month: integer, Day: integer, TimeZone: nil): time`
   Returns the time specified by the provided components in UTC.


:mini:`meth time(Year: integer, Month: integer, Day: integer, Hour: integer, Minute: integer, Second: integer, Arg₇: time::zone): time`
   Returns the time specified by the provided components in the specified time zone.


:mini:`meth time(Year: integer, Month: integer, Day: integer, Arg₄: time::zone): time`
   Returns the time specified by the provided components in the specified time zone.


:mini:`meth time(): time`
   Returns the current time.


:mini:`meth time(String: string, Format: string, TimeZone: nil): time`
   Parses the :mini:`String` as a time according to specified format. The time is assumed to be in UTC.


:mini:`meth time(Year: integer, Month: integer, Day: integer, Hour: integer, Minute: integer, Second: integer): time`
   Returns the time specified by the provided components in the local time.


:mini:`meth (Start: time) + (Duration: number): time`
   Returns the time :mini:`Duration` seconds after :mini:`Start`.

   .. code-block:: mini

      import: time("std/time")
      time("2022-04-01 12:00:00") + 3600 :> 2022-04-01T13:00:00


:mini:`meth (Start: time) - (Duration: number): time`
   Returns the time :mini:`Duration` seconds before :mini:`Start`.

   .. code-block:: mini

      import: time("std/time")
      time("2022-04-01 12:00:00") - 3600 :> 2022-04-01T11:00:00


:mini:`meth (End: time) - (Start: time): real`
   Returns the time elasped betwen :mini:`Start` and :mini:`End` in seconds.

   .. code-block:: mini

      import: time("std/time")
      time("2022-04-01 12:00:00") - time("2022-04-01 11:00:00")
      :> 3600


:mini:`meth (A: time) <> (B: time): integer`
   Compares the times :mini:`A` and :mini:`B` and returns :mini:`-1`,  :mini:`0` or :mini:`1` respectively.


:mini:`meth (Arg₁: time) @ (Arg₂: time::zone)`
   *TBD*


:mini:`meth (Time: time):day: integer`
   Returns the date from :mini:`Time` in local time.


:mini:`meth (Time: time):day(TimeZone: nil): integer`
   Returns the date from :mini:`Time` in UTC.


:mini:`meth (Time: time):day(TimeZone: time::zone): integer`
   Returns the date from :mini:`Time` in :mini:`TimeZone`.


:mini:`meth (Time: time):hour: integer`
   Returns the hour from :mini:`Time` in local time.


:mini:`meth (Time: time):hour(TimeZone: nil): integer`
   Returns the hour from :mini:`Time` in UTC.


:mini:`meth (Time: time):hour(TimeZone: time::zone): integer`
   Returns the hour from :mini:`Time` in :mini:`TimeZone`.


:mini:`meth (Time: time):minute: integer`
   Returns the minute from :mini:`Time` in local time.


:mini:`meth (Time: time):minute(TimeZone: nil): integer`
   Returns the minute from :mini:`Time` in UTC.


:mini:`meth (Time: time):minute(TimeZone: time::zone): integer`
   Returns the minute from :mini:`Time` in :mini:`TimeZone`.


:mini:`meth (Time: time):month: integer`
   Returns the month from :mini:`Time` in local time.


:mini:`meth (Time: time):month(TimeZone: nil): integer`
   Returns the month from :mini:`Time` in UTC.


:mini:`meth (Time: time):month(TimeZone: time::zone): integer`
   Returns the month from :mini:`Time` in :mini:`TimeZone`.


:mini:`meth (Time: time):nsec: integer`
   Returns the nanoseconds component of :mini:`Time`.


:mini:`meth (Time: time):second: integer`
   Returns the second from :mini:`Time` in local time.


:mini:`meth (Time: time):second(TimeZone: nil): integer`
   Returns the second from :mini:`Time` in UTC.


:mini:`meth (Time: time):second(TimeZone: time::zone): integer`
   Returns the second from :mini:`Time` in :mini:`TimeZone`.


:mini:`meth (Time: time):wday: integer`
   Returns the day of the week from :mini:`Time` in local time.


:mini:`meth (Time: time):wday(TimeZone: nil): integer`
   Returns the day of the week from :mini:`Time` in UTC.


:mini:`meth (Time: time):wday(TimeZone: time::zone): integer`
   Returns the day of the week from :mini:`Time` in :mini:`TimeZone`.


:mini:`meth (Time: time):yday: integer`
   Returns the number of days from the start of the year from :mini:`Time` in local time.


:mini:`meth (Time: time):yday(TimeZone: nil): integer`
   Returns the number of days from the start of the year from :mini:`Time` in UTC.


:mini:`meth (Time: time):yday(TimeZone: time::zone): integer`
   Returns the number of days from the start of the year from :mini:`Time` in :mini:`TimeZone`.


:mini:`meth (Time: time):year: integer`
   Returns the year from :mini:`Time` in local time.


:mini:`meth (Time: time):year(TimeZone: nil): integer`
   Returns the year from :mini:`Time` in UTC.


:mini:`meth (Time: time):year(TimeZone: time::zone): integer`
   Returns the year from :mini:`Time` in :mini:`TimeZone`.


:mini:`meth (Buffer: string::buffer):append(Time: time): string`
   Formats :mini:`Time` as a local time.


:mini:`meth (Buffer: string::buffer):append(Time: time, TimeZone: nil): string`
   Formats :mini:`Time` as a UTC time according to ISO 8601.


:mini:`meth (Buffer: string::buffer):append(Time: time, Format: string): string`
   Formats :mini:`Time` as a local time according to the specified format.


:mini:`meth (Buffer: string::buffer):append(Time: time, Format: string, TimeZone: nil): string`
   Formats :mini:`Time` as a UTC time according to the specified format.


:mini:`meth (Buffer: string::buffer):append(Time: time, Format: string, TimeZone: time::zone): string`
   Formats :mini:`Time` as a time in :mini:`TimeZone` according to the specified format.


:mini:`meth (Buffer: string::buffer):append(Time: time, TimeZone: time::zone): string`
   Formats :mini:`Time` as a time in :mini:`TimeZone`.


.. _type-time-zone:

:mini:`type time::zone`
   A time zone.


:mini:`meth (Buffer: string::buffer):append(TimeZone: time::zone)`
   Appends the name of :mini:`TimeZone` to :mini:`Buffer`.


.. _type-time-zoned:

:mini:`type time::zoned`
   *TBD*


:mini:`meth (Name: time::zones) :: (Arg₂: string): time::zone | error`
   Returns the time zone identified by :mini:`Name` or an error if no time zone is found.


