.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

cbor
====

.. rst-class:: mini-api

:mini:`meth cbor::decode(Bytes: address): any | error`
   Decode :mini:`Bytes` into a Minilang value,  or return an error if :mini:`Bytes` contains invalid CBOR or cannot be decoded into a Minilang value.


:mini:`meth cbor::decode(Bytes: address, Externals: external::set): any | error`
   Decode :mini:`Bytes` into a Minilang value,  or return an error if :mini:`Bytes` contains invalid CBOR or cannot be decoded into a Minilang value.


:mini:`meth cbor::decode(Bytes: address, Globals: function): any | error`
   Decode :mini:`Bytes` into a Minilang value,  or return an error if :mini:`Bytes` contains invalid CBOR or cannot be decoded into a Minilang value.


:mini:`meth cbor::decode(Bytes: address, Globals: map): any | error`
   Decode :mini:`Bytes` into a Minilang value,  or return an error if :mini:`Bytes` contains invalid CBOR or cannot be decoded into a Minilang value.


:mini:`meth cbor::encode(Value: any): address | error`
   Encode :mini:`Value` into CBOR or return an error if :mini:`Value` cannot be encoded.


:mini:`meth cbor::encode(Value: any, Externals: external::set): address | error`
   Encode :mini:`Value` into CBOR or return an error if :mini:`Value` cannot be encoded.


:mini:`meth cbor::encode(Value: any, Buffer: string::buffer): address | error`
   Encode :mini:`Value` into CBOR or return an error if :mini:`Value` cannot be encoded.


:mini:`meth cbor::encode(Value: any, Buffer: string::buffer, Externals: external::set): address | error`
   Encode :mini:`Value` into CBOR or return an error if :mini:`Value` cannot be encoded.


:mini:`type cbor::decoder < stream`
   A CBOR decoder that can be written to as a stream and calls a user-supplied callback whenever a complete value is decoded.


:mini:`fun cbor::decoder(Callback: function): cbor::decoder`
   Returns a new CBOR decoder that calls :mini:`Callback(Value)` whenever a complete CBOR value is written to the decoder.


:mini:`type cbor::tag`
   *TBD*


:mini:`fun cbor::tag(Tag: integer, Arg₂: any): cbor::tag`
   *TBD*


:mini:`meth cbor::decode(Stream: stream): any | error`
   *TBD*


:mini:`fun cbor::write_array(Buffer: string::buffer, Value: integer): Buffer`
   *TBD*


:mini:`fun cbor::write_break(Buffer: string::buffer): Buffer`
   *TBD*


:mini:`fun cbor::write_bytes(Buffer: string::buffer, Value: integer): Buffer`
   *TBD*


:mini:`fun cbor::write_float2(Buffer: string::buffer, Value: integer): Buffer`
   *TBD*


:mini:`fun cbor::write_float4(Buffer: string::buffer, Value: integer): Buffer`
   *TBD*


:mini:`fun cbor::write_float8(Buffer: string::buffer, Value: integer): Buffer`
   *TBD*


:mini:`fun cbor::write_indef_array(Buffer: string::buffer): Buffer`
   *TBD*


:mini:`fun cbor::write_indef_bytes(Buffer: string::buffer): Buffer`
   *TBD*


:mini:`fun cbor::write_indef_map(Buffer: string::buffer): Buffer`
   *TBD*


:mini:`fun cbor::write_indef_string(Buffer: string::buffer): Buffer`
   *TBD*


:mini:`fun cbor::write_integer(Buffer: string::buffer, Value: integer): Buffer`
   *TBD*


:mini:`fun cbor::write_map(Buffer: string::buffer, Value: integer): Buffer`
   *TBD*


:mini:`fun cbor::write_negative(Buffer: string::buffer, Value: integer): Buffer`
   *TBD*


:mini:`fun cbor::write_positive(Buffer: string::buffer, Value: integer): Buffer`
   *TBD*


:mini:`fun cbor::write_simple(Buffer: string::buffer, Value: boolean|nil): Buffer`
   *TBD*


:mini:`fun cbor::write_string(Buffer: string::buffer, Value: integer): Buffer`
   *TBD*


:mini:`fun cbor::write_tag(Buffer: string::buffer, Value: integer): Buffer`
   *TBD*


