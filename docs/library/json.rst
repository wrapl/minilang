.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

json
====

.. rst-class:: mini-api

JSON values are mapped to Minilang as follows:

* :json:`null` |harr| :mini:`nil`
* :json:`true` |harr| :mini:`true`
* :json:`false` |harr| :mini:`false`
* *integer* |harr| :mini:`integer`
* *real* |harr| :mini:`real`
* *string* |harr| :mini:`string`
* *array* |harr| :mini:`list`
* *object* |harr| :mini:`map`

.. _fun-json-encode:

:mini:`fun json::encode(Value: any): string | error`
   Encodes :mini:`Value` into JSON,  raising an error if :mini:`Value` cannot be represented as JSON.


.. _type-json:

:mini:`type json < string`
   Contains a JSON encoded value. Primarily used to distinguish strings containing JSON from other strings (e.g. for CBOR encoding).


.. _fun-json:

:mini:`fun json(Value: any): json`
   Encodes :mini:`Value` into JSON.


:mini:`meth (Json: json):decode: any | error`
   Decodes the JSON string in :mini:`Json` into a Minilang value.


.. _type-json-decoder:

:mini:`type json::decoder < stream`
   A JSON decoder that can be written to as a stream and calls a user-supplied callback whenever a complete value is parsed.


.. _fun-json-decoder:

:mini:`fun json::decoder(Callback: any): json::decoder`
   Returns a new JSON decoder that calls :mini:`Callback(Value)` whenever a complete JSON value is written to the decoder.


.. _fun-json-decode:

:mini:`fun json::decode(Json: string): any`
   Decodes :mini:`Json` into a Minilang value.


