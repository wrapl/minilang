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


:mini:`type cbor::decoder < (MLStream`
   A CBOR decoder that can be written to as a stream and calls a user-supplied callback whenever a complete value is decoded.


:mini:`fun cbor::decoder(Callback: function): cbor::decoder`
   Returns a new CBOR decoder that calls :mini:`Callback(Value)` whenever a complete CBOR value is written to the decoder.


:mini:`type cbortag`
   *TBD*


:mini:`fun cbortag(Arg₁: integer, Arg₂: any)`
   *TBD*


