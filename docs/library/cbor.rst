.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

cbor
====

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


