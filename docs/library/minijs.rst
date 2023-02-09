.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

minijs
======

.. rst-class:: mini-api

Provides a specialized encoding of Minilang values to and from JSON with support for complex or cyclic data structures.

* :json:`null` |harr| :mini:`nil`
* :json:`true` |harr| :mini:`true`
* :json:`false` |harr| :mini:`false`
* *integer* |harr| :mini:`integer`
* *real* |harr| :mini:`real`
* *string* |harr| :mini:`string`
* ``[type,  ...]`` |harr| *other*

:mini:`meth minijs::encode(Value: any): string | error`
   *TBD*


:mini:`meth minijs::encode(Value: any, Externals: external::set): string | error`
   *TBD*


.. _type-minijs:

:mini:`type minijs < string`
   *TBD*


.. _fun-minijs:

:mini:`fun minijs(Value: any): minijs`
   *TBD*


:mini:`meth minijs::decode(Json: string): any | error`
   *TBD*


:mini:`meth minijs::decode(Json: string, Externals: external::set): any | error`
   *TBD*


