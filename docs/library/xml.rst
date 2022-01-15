xml
===

:mini:`type xml`
   *TBD*

:mini:`meth ^(Xml: xml): xml | nil`
   *TBD*

:mini:`meth <(Arg₁: xml)`
   *TBD*

:mini:`meth >(Arg₁: xml)`
   *TBD*

:mini:`type xml::text < xml, string`
   *TBD*

:mini:`type xml::element < xml, sequence`
   *TBD*

:mini:`meth (Xml: xml::element):tag: method`
   *TBD*

:mini:`meth (Xml: xml::element):attributes: map`
   *TBD*

:mini:`meth (Parent: xml::element):put(String: string): xml`
   *TBD*

:mini:`meth (Parent: xml::element):put(Child: xml): xml`
   *TBD*

:mini:`meth (Arg₁: xml::element)[Arg₂: integer]`
   *TBD*

:mini:`meth (Arg₁: xml::element)[Arg₂: string]`
   *TBD*

:mini:`meth (Arg₁: xml) < (Arg₂: method)`
   *TBD*

:mini:`meth (Arg₁: xml) < (Arg₂: method)`
   *TBD*

:mini:`meth (Arg₁: xml) > (Arg₂: method)`
   *TBD*

:mini:`type xml::children < sequence`
   *TBD*

:mini:`meth /(Arg₁: xml::element)`
   *TBD*

:mini:`meth (Arg₁: xml) / (Arg₂: method)`
   *TBD*

:mini:`type xml::recursive < sequence`
   *TBD*

:mini:`meth //(Arg₁: xml::element)`
   *TBD*

:mini:`meth (Arg₁: xml) // (Arg₂: method)`
   *TBD*

:mini:`meth (Arg₁: string::buffer):append(Arg₂: xml::element)`
   *TBD*

:mini:`meth (Tag: method):xml(Children...: string|xml, Attributes?: names|map): xml`
   *TBD*

:mini:`meth (Xml: string):xml: xml`
   *TBD*

:mini:`meth xml(Arg₁: stream)`
   *TBD*

:mini:`fun xml::decoder(Callback: any): xml::decoder`
   *TBD*

:mini:`type xml::decoder`
   *TBD*

:mini:`meth (Decoder: xml::decoder):decode(Xml: address): Decoder`
   *TBD*

:mini:`meth (Decoder: xml::decoder):decode(Xml: address, Size: integer): Decoder`
   *TBD*

:mini:`meth (Decoder: xml::decoder):finish: Decoder`
   *TBD*

