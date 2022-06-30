.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

cbor
====

:mini:`meth cbor::decode(Bytes: address): any | error`
   Decode :mini:`Bytes` into a Minilang value,  or return an error if :mini:`Bytes` contains invalid CBOR or cannot be decoded into a Minilang value.


:mini:`meth cbor::encode(Value: any): address | error`
   Encode :mini:`Value` into CBOR or return an error if :mini:`Value` cannot be encoded.


.. _value-cbor-Objects:

:mini:`def cbor::Objects: map[string,function]`
   Constructors to call for tag 27 (objects).


.. _fun-decodecomplex:

:mini:`fun decodecomplex(Arg₁: real, Arg₂: real)`
   *TBD*


.. _fun-mlcborglobal:

:mini:`fun mlcborglobal(Name: string, Value: any)`
   *TBD*


:mini:`meth cbor::encode(Value: string::buffer, Arg₂: any): address | error`
   Encode :mini:`Value` into CBOR or return an error if :mini:`Value` cannot be encoded.


