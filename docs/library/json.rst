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

:mini:`meth json::decode(Json: address): any`
   Decodes :mini:`Json` into a Minilang value.


:mini:`meth json::encode(Value: any): string | error`
   Encodes :mini:`Value` into JSON,  raising an error if :mini:`Value` cannot be represented as JSON.


:mini:`type json < string`
   Contains a JSON encoded value. Primarily used to distinguish strings containing JSON from other strings (e.g. for CBOR encoding).


:mini:`fun json(Value: any): json`
   Encodes :mini:`Value` into JSON.


:mini:`meth (Json: json):decode: any | error`
   Decodes the JSON string in :mini:`Json` into a Minilang value.


:mini:`meth (Json: json):value: any | error`
   Decodes the JSON string in :mini:`Json` into a Minilang value.


:mini:`type json::decoder < stream`
   A JSON decoder that can be written to as a stream and calls a user-supplied callback whenever a complete value is decoded.


:mini:`fun json::decoder(Callback: any): json::decoder`
   Returns a new JSON decoder that calls :mini:`Callback(Value)` whenever a complete JSON value is written to the decoder.


:mini:`meth json::decode(Stream: stream): any`
   Decodes the content of :mini:`Json` into a Minilang value.


:mini:`meth json::encode(Buffer: string::buffer, Value: any)`
   *TBD*


