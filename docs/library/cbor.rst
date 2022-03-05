.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

cbor
====

.. _value-cbor-Objects:

:mini:`def cbor::Objects: map[string,function]`
   Constructors to call for tag 27 (objects).



.. _fun-cbor-decode:

:mini:`fun cbor::decode(Bytes: address): any | error`
   Decode :mini:`Bytes` into a Minilang value,  or return an error if :mini:`Bytes` contains invalid CBOR or cannot be decoded into a Minilang value.



.. _fun-cbor-encode:

:mini:`fun cbor::encode(Value: any): address | error`
   Encode :mini:`Value` into CBOR or return an error if :mini:`Value` cannot be encoded.



