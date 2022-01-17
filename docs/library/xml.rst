xml
===

:mini:`type xml`
   An XML node.


:mini:`meth ^(Xml: xml): xml | nil`
   Returnst the parent of :mini:`Xml` or :mini:`nil`.


:mini:`meth <(Xml: xml): xml | nil`
   Returnst the previous sibling of :mini:`Xml` or :mini:`nil`.


:mini:`meth >(Xml: xml): xml | nil`
   Returnst the next sibling of :mini:`Xml` or :mini:`nil`.


:mini:`type xml::text < xml, string`
   A XML text node.


:mini:`meth (Xml: xml::text):text: string`
   Returns the text content of :mini:`Xml`.


:mini:`type xml::element < xml, sequence`
   An XML element node.


:mini:`meth (Xml: xml::element):tag: string`
   Returns the tag of :mini:`Xml`.


:mini:`meth (Xml: xml::element):attributes: map`
   Returns the attributes of :mini:`Xml`.


:mini:`meth (Xml: xml::element):text: string`
   Returns the (recursive) text content of :mini:`Xml`.


:mini:`meth (Parent: xml::element):put(String: string): xml`
   Adds a new text node containing :mini:`String` to :mini:`Parent`.


:mini:`meth (Parent: xml::element):put(Child: xml): xml`
   Adds :mini:`Child` to :mini:`Parent`.


:mini:`meth (Parent: xml::element)[Index: integer]: xml | nil`
   Returns the :mini:`Index`-th child of :mini:`Parent` or :mini:`nil`.


:mini:`meth (Parent: xml::element)[Attribute: string]: string | nil`
   Returns the value of the :mini:`Attribute` attribute of :mini:`Parent`.


:mini:`meth (Xml: xml) ^ (Tag: string): xml | nil`
   Returns the parent of :mini:`Xml` if it has tag :mini:`Tag`,  otherwise :mini:`nil`.


:mini:`meth (Xml: xml) < (Tag: string): xml | nil`
   Returns the previous sibling of :mini:`Xml` with tag :mini:`Tag`,  otherwise :mini:`nil`.


:mini:`meth (Xml: xml) > (Tag: string): xml | nil`
   Returns the next sibling of :mini:`Xml` with tag :mini:`Tag`,  otherwise :mini:`nil`.


:mini:`meth /(Xml: xml::element): sequence`
   Returns a sequence of the children of :mini:`Xml`.


:mini:`meth (Xml: xml) / (Tag: string): sequence`
   Returns a sequence of the children of :mini:`Xml` with tag :mini:`Tag`.


:mini:`meth (Xml: xml) / (Fn: function): sequence`
   Returns a sequence of the children of :mini:`Xml` for which :mini:`Fn(Child)` is non-nil.


:mini:`meth //(Xml: xml::element): sequence`
   Returns a sequence of the recursive children of :mini:`Xml`,  including :mini:`Xml`.


:mini:`meth (Xml: xml) // (Tag: string): sequence`
   Returns a sequence of the recursive children of :mini:`Xml` with tag :mini:`Tag`.


:mini:`meth (Xml: xml) // (Fn: function): sequence`
   Returns a sequence of the recursive children of :mini:`Xml` for which :mini:`Fn(Child)` is non-nil.


:mini:`type xml::filter < function`
   An XML filter.


:mini:`meth xml::filter(Arg₁: names, ...): xml::filter`
   *TBD*

:mini:`meth xml::filter(Arg₁: string, Arg₂: names, ...): xml::filter`
   *TBD*

:mini:`meth (Arg₁: xml) // (Arg₂: names, ...)`
   *TBD*

:mini:`meth (Arg₁: xml) // (Arg₂: string, Arg₃: names, ...)`
   *TBD*

:mini:`meth (Arg₁: xml::sequence) / (...)`
   *TBD*

:mini:`meth (Arg₁: xml::sequence) // (...)`
   *TBD*

:mini:`meth (Arg₁: string::buffer):append(Arg₂: xml::element)`
   *TBD*

:mini:`meth (Tag: string):xml(Children...: string|xml, Attributes?: names|map): xml`
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

