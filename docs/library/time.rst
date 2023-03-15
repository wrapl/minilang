.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

time
====

.. rst-class:: mini-api

Provides time and date operations.

.. _fun-time-mdays:

:mini:`fun time::mdays(Year: integer, Month: integer): integer`
   *TBD*


.. _type-time:

:mini:`type time`
   An instant in time with nanosecond resolution.


:mini:`meth time(Year: integer, Month: integer, Day: integer): time`
   Returns the time specified by the provided components in the local time.


:mini:`meth time(Year: integer, Month: integer, Day: integer, Hour: integer, Minute: integer, Second: integer, TimeZone: nil): time`
   Returns the time specified by the provided components in UTC.


:mini:`meth time(Year: integer, Month: integer, Day: integer, Hour: integer, Minute: integer, Second: integer, Arg₇: time::zone): time`
   Returns the time specified by the provided components in the specified time zone.


:mini:`meth time(Year: integer, Month: integer, Day: integer, TimeZone: nil): time`
   Returns the time specified by the provided components in UTC.


:mini:`meth time(Year: integer, Month: integer, Day: integer, Arg₄: time::zone): time`
   Returns the time specified by the provided components in the specified time zone.


:mini:`meth time(String: string): time`
   Parses the :mini:`String` as a time according to ISO 8601.

   .. code-block:: mini

      time("2023-02-09T21:19:33.196413266")
      :> 2023-02-09T20:19:33.196413266


:mini:`meth time(String: string, Format: string): time`
   Parses the :mini:`String` as a time according to specified format. The time is assumed to be in local time.


:mini:`meth time(): time`
   Returns the current time.

   .. code-block:: mini

      time() :> 2023-03-15T11:00:26.55536383


:mini:`meth time(Year: integer, Month: integer, Day: integer, Hour: integer, Minute: integer, Second: integer): time`
   Returns the time specified by the provided components in the local time.


:mini:`meth time(String: string, Format: string, TimeZone: nil): time`
   Parses the :mini:`String` as a time according to specified format. The time is assumed to be in UTC.


:mini:`meth (Start: time) + (Duration: number): time`
   Returns the time :mini:`Duration` seconds after :mini:`Start`.

   .. code-block:: mini

      time("2022-04-01 12:00:00") + 3600 :> 2022-04-01T13:00:00


:mini:`meth (Start: time) - (Duration: number): time`
   Returns the time :mini:`Duration` seconds before :mini:`Start`.

   .. code-block:: mini

      time("2022-04-01 12:00:00") - 3600 :> 2022-04-01T11:00:00


:mini:`meth (End: time) - (Start: time): real`
   Returns the time elasped betwen :mini:`Start` and :mini:`End` in seconds.

   .. code-block:: mini

      time("2022-04-01 12:00:00") - time("2022-04-01 11:00:00")
      :> 3600


:mini:`meth (A: time) <> (B: time): integer`
   Compares the times :mini:`A` and :mini:`B` and returns :mini:`-1`,  :mini:`0` or :mini:`1` respectively.


:mini:`meth (Time: time) @ (TimeZone: time::zone): time::zoned`
   Returns a *zoned* time,  that contains an instant of time and an associated time zone.


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


:mini:`meth (Arg₁: time):precision(Arg₂: integer)`
   *TBD*


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


:mini:`meth (Arg₁: time):with(Arg₂₁ is Value₁, ...)`
   *TBD*


:mini:`meth (Arg₁: time):with(Arg₂: nil, Arg₃₁ is Value₁, ...)`
   *TBD*


:mini:`meth (Arg₁: time):with(Arg₂: time::zone, Arg₃₁ is Value₁, ...)`
   *TBD*


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


.. _type-time-day:

:mini:`type time::day < enum`
   *TBD*


.. _type-time-month:

:mini:`type time::month < enum`
   *TBD*


.. _type-time-zone:

:mini:`type time::zone`
   A time zone.


:mini:`meth (Buffer: string::buffer):append(TimeZone: time::zone)`
   Appends the name of :mini:`TimeZone` to :mini:`Buffer`.


:mini:`meth (Name: time::zone::type) :: (Arg₂: string): time::zone | error`
   Returns the time zone identified by :mini:`Name` or an error if no time zone is found.


.. _type-time-zoned:

:mini:`type time::zoned`
   *TBD*


