.. include:: <isonum.txt>

xml
===

:mini:`type xml`
   An XML node.


:mini:`meth (Xml: xml):parent: xml | nil`
   Returnst the parent of :mini:`Xml` or :mini:`nil`.


:mini:`meth (Xml: xml):prev: xml | nil`
   Returnst the previous sibling of :mini:`Xml` or :mini:`nil`.


:mini:`meth (Xml: xml):next: xml | nil`
   Returnst the next sibling of :mini:`Xml` or :mini:`nil`.


:mini:`type xml::text < xml, string`
   A XML text node.


:mini:`meth (Xml: xml::text):text: string`
   Returns the text content of :mini:`Xml`.


:mini:`type xml::element < xml, sequence`
   An XML element node.


:mini:`meth xml::element(Tag: string, Children...: string|xml, Attributes?: names|map): xml::element`
   *TBD*

:mini:`meth (Xml: xml::element):tag: string`
   Returns the tag of :mini:`Xml`.


:mini:`meth (Xml: xml::element):attributes: map`
   Returns the attributes of :mini:`Xml`.


:mini:`meth (Xml: xml::element):text: string`
   Returns the (recursive) text content of :mini:`Xml`.


:mini:`meth (Parent: xml::element):put(String: string): xml`
   Adds a new text node containing :mini:`String` to :mini:`Parent`.


:mini:`meth (Parent: xml::element):put(Child: xml::element): xml`
   Adds :mini:`Child` to :mini:`Parent`.


:mini:`meth (Arg₁: xml::element):grow(Arg₂: sequence, ...)`
   *TBD*

:mini:`meth (Parent: xml::element)[Index: integer]: xml | nil`
   Returns the :mini:`Index`-th child of :mini:`Parent` or :mini:`nil`.


:mini:`meth (Parent: xml::element)[Attribute: string]: string | nil`
   Returns the value of the :mini:`Attribute` attribute of :mini:`Parent`.


:mini:`type xml::filter < function`
   An XML filter.


:mini:`meth xml::filter(Arg₁₁ is Value₁, ...): xml::filter`
   *TBD*

:mini:`meth xml::filter(Arg₁: string, Arg₂₁ is Value₁, ...): xml::filter`
   *TBD*

:mini:`meth /(Xml: xml::element): sequence`
   Returns a sequence of the children of :mini:`Xml`.


:mini:`meth (Xml: xml) / (Tag: string): sequence`
   Returns a sequence of the children of :mini:`Xml` with tag :mini:`Tag`.


:mini:`meth (Xml: xml) / (Attribute₁ is Value₁, ...): sequence`
   Returns a sequence of the children of :mini:`Xml` with :mini:`Attribute₁ = Value₁`,  etc.


:mini:`meth (Xml: xml) / (Tag: string, Attribute₁ is Value₁, ...): sequence`
   Returns a sequence of the children of :mini:`Xml` with tag :mini:`Tag` and :mini:`Attribute₁ = Value₁`,  etc.


:mini:`meth (Xml: xml) / (Fn: function): sequence`
   Returns a sequence of the children of :mini:`Xml` for which :mini:`Fn(Child)` is non-nil.


:mini:`meth >>(Xml: xml::element): sequence`
   Returns a sequence of the next siblings of :mini:`Xml`.


:mini:`meth (Xml: xml) >> (Tag: string): sequence`
   Returns a sequence of the next siblings of :mini:`Xml` with tag :mini:`Tag`.


:mini:`meth (Xml: xml) >> (Attribute₁ is Value₁, ...): sequence`
   Returns a sequence of the next siblings of :mini:`Xml` with :mini:`Attribute₁ = Value₁`,  etc.


:mini:`meth (Xml: xml) >> (Tag: string, Attribute₁ is Value₁, ...): sequence`
   Returns a sequence of the next siblings of :mini:`Xml` with tag :mini:`Tag` and :mini:`Attribute₁ = Value₁`,  etc.


:mini:`meth (Xml: xml) >> (Fn: function): sequence`
   Returns a sequence of the next siblings of :mini:`Xml` for which :mini:`Fn(Child)` is non-nil.


:mini:`meth <<(Xml: xml::element): sequence`
   Returns a sequence of the previous siblings of :mini:`Xml`.


:mini:`meth (Xml: xml) << (Tag: string): sequence`
   Returns a sequence of the previous siblings of :mini:`Xml` with tag :mini:`Tag`.


:mini:`meth (Xml: xml) << (Attribute₁ is Value₁, ...): sequence`
   Returns a sequence of the previous siblings of :mini:`Xml` with :mini:`Attribute₁ = Value₁`,  etc.


:mini:`meth (Xml: xml) << (Tag: string, Attribute₁ is Value₁, ...): sequence`
   Returns a sequence of the previous siblings of :mini:`Xml` with tag :mini:`Tag` and :mini:`Attribute₁ = Value₁`,  etc.


:mini:`meth (Xml: xml) << (Fn: function): sequence`
   Returns a sequence of the previous siblings of :mini:`Xml` for which :mini:`Fn(Child)` is non-nil.


:mini:`meth (Xml: xml):parent(Tag: string): xml | nil`
   Returns the parent of :mini:`Xml` if it has tag :mini:`Tag`,  otherwise :mini:`nil`.


:mini:`meth (Arg₁: xml):parent(Arg₂: integer)`
   *TBD*

:mini:`meth (Arg₁: xml):next(Arg₂: integer)`
   *TBD*

:mini:`meth (Arg₁: xml):prev(Arg₂: integer)`
   *TBD*

:mini:`meth //(Xml: xml::element): sequence`
   Returns a sequence of the recursive children of :mini:`Xml`,  including :mini:`Xml`.


:mini:`meth (Xml: xml) // (Tag: string): sequence`
   Returns a sequence of the recursive children of :mini:`Xml` with tag :mini:`Tag`.


:mini:`meth (Xml: xml) // (Attribute₁ is Value₁, ...): sequence`
   Returns a sequence of the recursive children of :mini:`Xml` with :mini:`Attribute₁ = Value₁`,  etc.


:mini:`meth (Xml: xml) // (Tag: string, Attribute₁ is Value₁, ...): sequence`
   Returns a sequence of the recursive children of :mini:`Xml` with tag :mini:`Tag` and :mini:`Attribute₁ = Value₁`,  etc.


:mini:`meth (Xml: xml) // (Fn: function): sequence`
   Returns a sequence of the recursive children of :mini:`Xml` for which :mini:`Fn(Child)` is non-nil.


:mini:`meth (Sequence: xml::sequence) / (Args: any, ...): sequence`
   Generates the sequence :mini:`Nodeᵢ / Args` where :mini:`Nodeᵢ` are the nodes generated by :mini:`Sequence`.


:mini:`meth (Sequence: xml::sequence) // (Args: any, ...): sequence`
   Generates the sequence :mini:`Nodeᵢ // Args` where :mini:`Nodeᵢ` are the nodes generated by :mini:`Sequence`.


:mini:`meth (Sequence: xml::sequence) >> (Args: any, ...): sequence`
   Generates the sequence :mini:`Nodeᵢ >> Args` where :mini:`Nodeᵢ` are the nodes generated by :mini:`Sequence`.


:mini:`meth (Sequence: xml::sequence) << (Args: any, ...): sequence`
   Generates the sequence :mini:`Nodeᵢ << Args` where :mini:`Nodeᵢ` are the nodes generated by :mini:`Sequence`.


:mini:`meth (Sequence: xml::sequence):parent(Args: any, ...): sequence`
   Generates the sequence :mini:`Nodeᵢ ^ Args` where :mini:`Nodeᵢ` are the nodes generated by :mini:`Sequence`.


:mini:`meth (Sequence: xml::sequence):next(Args: any, ...): sequence`
   Generates the sequence :mini:`Nodeᵢ + Args` where :mini:`Nodeᵢ` are the nodes generated by :mini:`Sequence`.


:mini:`meth (Sequence: xml::sequence):prev(Args: any, ...): sequence`
   Generates the sequence :mini:`Nodeᵢ - Args` where :mini:`Nodeᵢ` are the nodes generated by :mini:`Sequence`.


:mini:`meth (Sequence: xml::sequence):contains(String: string): sequence`
   Equivalent to :mini:`Sequence ->? fun(X) X:text:find(String)`.


:mini:`meth (Sequence: xml::sequence):contains(Regex: regex): sequence`
   Equivalent to :mini:`Sequence ->? fun(X) X:text:find(Regex)`.


:mini:`meth (Sequence: xml::sequence):has(Fn: function): sequence`
   Equivalent to :mini:`Sequence ->? fun(X) first(Fn(X))`.


:mini:`meth (Arg₁: string::buffer):append(Arg₂: xml::element)`
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

