.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

logging
=======

.. rst-class:: mini-api

:mini:`fun logger::level(Level: :string): string`
   Gets or sets the logging level for default logging. Returns the log level.


:mini:`type log::macro < (MLFunction`
   *TBD*


:mini:`type logger`
   A logger.


:mini:`fun logger(Category: string): logger`
   Returns a new logger with levels :mini:`::error`,  :mini:`::warn`,  :mini:`::info` and :mini:`::debug`.


:mini:`meth (Logger: logger) :: (Level: string): logger::fn`
   *TBD*


