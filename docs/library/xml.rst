.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

xml
===

.. rst-class:: mini-api

:mini:`meth xml::parse(Stream: stream): xml`
   Returns the contents of :mini:`Stream` parsed into an XML node.


:mini:`meth xml::parse(String: string): xml`
   Returns :mini:`String` parsed into an XML node.


.. _type-xml:

:mini:`type xml`
   An XML node.


:mini:`meth xml(String: string): xml`
   Returns :mini:`String` parsed into an XML node.


:mini:`meth xml(Stream: stream): xml`
   Returns the contents of :mini:`Stream` parsed into an XML node.


:mini:`meth (Xml: xml) / (Fn: function): sequence`
   Returns a sequence of the children of :mini:`Xml` for which :mini:`Fn(Child)` is non-nil.


:mini:`meth (Xml: xml) / (Attribute₁ is Value₁, ...): sequence`
   Returns a sequence of the children of :mini:`Xml` with :mini:`Attribute₁ = Value₁`,  etc.


:mini:`meth (Xml: xml) / (Tag: string): sequence`
   Returns a sequence of the children of :mini:`Xml` with tag :mini:`Tag`.


:mini:`meth (Xml: xml) / (Tag: string, Attribute₁ is Value₁, ...): sequence`
   Returns a sequence of the children of :mini:`Xml` with tag :mini:`Tag` and :mini:`Attribute₁ = Value₁`,  etc.


:mini:`meth (Xml: xml) // (Fn: function): sequence`
   Returns a sequence of the recursive children of :mini:`Xml` for which :mini:`Fn(Child)` is non-nil.


:mini:`meth (Xml: xml) // (Attribute₁ is Value₁, ...): sequence`
   Returns a sequence of the recursive children of :mini:`Xml` with :mini:`Attribute₁ = Value₁`,  etc.


:mini:`meth (Xml: xml) // (Tag: string): sequence`
   Returns a sequence of the recursive children of :mini:`Xml` with tag :mini:`Tag`.


:mini:`meth (Xml: xml) // (Tag: string, Attribute₁ is Value₁, ...): sequence`
   Returns a sequence of the recursive children of :mini:`Xml` with tag :mini:`Tag` and :mini:`Attribute₁ = Value₁`,  etc.


:mini:`meth (Xml: xml) << (Fn: function): sequence`
   Returns a sequence of the previous siblings of :mini:`Xml` for which :mini:`Fn(Child)` is non-nil.


:mini:`meth (Xml: xml) << (Attribute₁ is Value₁, ...): sequence`
   Returns a sequence of the previous siblings of :mini:`Xml` with :mini:`Attribute₁ = Value₁`,  etc.


:mini:`meth (Xml: xml) << (Tag: string): sequence`
   Returns a sequence of the previous siblings of :mini:`Xml` with tag :mini:`Tag`.


:mini:`meth (Xml: xml) << (Tag: string, Attribute₁ is Value₁, ...): sequence`
   Returns a sequence of the previous siblings of :mini:`Xml` with tag :mini:`Tag` and :mini:`Attribute₁ = Value₁`,  etc.


:mini:`meth (Xml: xml) >> (Fn: function): sequence`
   Returns a sequence of the next siblings of :mini:`Xml` for which :mini:`Fn(Child)` is non-nil.


:mini:`meth (Xml: xml) >> (Attribute₁ is Value₁, ...): sequence`
   Returns a sequence of the next siblings of :mini:`Xml` with :mini:`Attribute₁ = Value₁`,  etc.


:mini:`meth (Xml: xml) >> (Tag: string): sequence`
   Returns a sequence of the next siblings of :mini:`Xml` with tag :mini:`Tag`.


:mini:`meth (Xml: xml) >> (Tag: string, Attribute₁ is Value₁, ...): sequence`
   Returns a sequence of the next siblings of :mini:`Xml` with tag :mini:`Tag` and :mini:`Attribute₁ = Value₁`,  etc.


:mini:`meth (Xml: xml):next: xml | nil`
   Returns the next sibling of :mini:`Xml` or :mini:`nil`.


:mini:`meth (Xml: xml):next(N: integer): xml | nil`
   Returns the :mini:`N`-th next sibling of :mini:`Xml` or :mini:`nil`.


:mini:`meth (Xml: xml):parent: xml | nil`
   Returnst the parent of :mini:`Xml` or :mini:`nil`.


:mini:`meth (Xml: xml):parent(N: integer): xml | nil`
   Returns the :mini:`N`-th parent of :mini:`Xml` or :mini:`nil`.


:mini:`meth (Xml: xml):parent(Tag: string): xml | nil`
   Returns the ancestor of :mini:`Xml` with tag :mini:`Tag` if one exists,  otherwise :mini:`nil`.


:mini:`meth (Xml: xml):prev: xml | nil`
   Returnst the previous sibling of :mini:`Xml` or :mini:`nil`.


:mini:`meth (Xml: xml):prev(N: integer): xml | nil`
   Returns the :mini:`N`-th previous sibling of :mini:`Xml` or :mini:`nil`.


.. _type-xml-decoder:

:mini:`type xml::decoder < stream`
   A callback based streaming XML decoder.


.. _fun-xml-decoder:

:mini:`fun xml::decoder(Callback: any): xml::decoder`
   Returns a new decoder that calls :mini:`Callback(Xml)` each time a complete XML document is parsed.


.. _type-xml-element:

:mini:`type xml::element < xml, sequence`
   An XML element node.


:mini:`meth xml::element(Tag: string, Arg₁, : any, ...): xml::element`
   Returns a new XML node with tag :mini:`Tag` and optional children and attributes depending on the types of each :mini:`Argᵢ`:
   
   * :mini:`string`: added as child text node. Consecutive strings are added a single node.
   * :mini:`xml`: added as a child node.
   * :mini:`list`: each value must be a :mini:`string` or :mini:`xml` and is added as above.
   * :mini:`map`: keys and values must be strings,  set as attributes.
   * :mini:`name is value`: values must be strings,  set as attributes.

   .. code-block:: mini

      import: xml("fmt/xml")
      xml::element("test", "Text", type is "example")
      :> error("ModuleError", "Module fmt not found in <library>")


:mini:`meth /(Xml: xml::element): sequence`
   Returns a sequence of the children of :mini:`Xml`.


:mini:`meth //(Xml: xml::element): sequence`
   Returns a sequence of the recursive children of :mini:`Xml`,  including :mini:`Xml`.


:mini:`meth <<(Xml: xml::element): sequence`
   Returns a sequence of the previous siblings of :mini:`Xml`.


:mini:`meth >>(Xml: xml::element): sequence`
   Returns a sequence of the next siblings of :mini:`Xml`.


:mini:`meth (Parent: xml::element)[Index: integer]: xml | nil`
   Returns the :mini:`Index`-th child of :mini:`Parent` or :mini:`nil`.


:mini:`meth (Parent: xml::element)[Attribute: string]: string | nil`
   Returns the value of the :mini:`Attribute` attribute of :mini:`Parent`.


:mini:`meth (Xml: xml::element):attributes: map`
   Returns the attributes of :mini:`Xml`.


:mini:`meth (Arg₁: xml::element):grow(Arg₂: sequence, ...)`
   *TBD*


:mini:`meth (Parent: xml::element):put(String: string): xml`
   Adds a new text node containing :mini:`String` to :mini:`Parent`.


:mini:`meth (Parent: xml::element):put(Child: xml::element): xml`
   Adds :mini:`Child` to :mini:`Parent`.


:mini:`meth (Xml: xml::element):tag: string`
   Returns the tag of :mini:`Xml`.


:mini:`meth (Xml: xml::element):text: string`
   Returns the (recursive) text content of :mini:`Xml`.


:mini:`meth (Xml: xml::element):text(Sep: string): string`
   Returns the (recursive) text content of :mini:`Xml`,  adding :mini:`Sep` between the contents of adjacent nodes.


:mini:`meth (Buffer: string::buffer):append(Xml: xml::element)`
   Appends a string representation of :mini:`Xml` to :mini:`Buffer`.


.. _type-xml-filter:

:mini:`type xml::filter < function`
   An XML filter.


:mini:`meth xml::filter(Attr₁ is Value₁, ...): xml::filter`
   Returns an XML filter that checks if a node has attributes :mini:`Attrᵢ = Valueᵢ`.


:mini:`meth xml::filter(Tag: string, Attr₁ is Value₁, ...): xml::filter`
   Returns an XML filter that checks if a node has tag :mini:`Tag` and attributes :mini:`Attrᵢ = Valueᵢ`.


:mini:`meth (Sequence: xml::sequence) / (Args: any, ...): sequence`
   Generates the sequence :mini:`Nodeᵢ / Args` where :mini:`Nodeᵢ` are the nodes generated by :mini:`Sequence`.


:mini:`meth (Sequence: xml::sequence) // (Args: any, ...): sequence`
   Generates the sequence :mini:`Nodeᵢ // Args` where :mini:`Nodeᵢ` are the nodes generated by :mini:`Sequence`.


:mini:`meth (Sequence: xml::sequence) << (Args: any, ...): sequence`
   Generates the sequence :mini:`Nodeᵢ << Args` where :mini:`Nodeᵢ` are the nodes generated by :mini:`Sequence`.


:mini:`meth (Sequence: xml::sequence) >> (Args: any, ...): sequence`
   Generates the sequence :mini:`Nodeᵢ >> Args` where :mini:`Nodeᵢ` are the nodes generated by :mini:`Sequence`.


:mini:`meth (Sequence: xml::sequence):contains(Regex: regex): sequence`
   Equivalent to :mini:`Sequence ->? fun(X) X:text:find(Regex)`.


:mini:`meth (Sequence: xml::sequence):contains(String: string): sequence`
   Equivalent to :mini:`Sequence ->? fun(X) X:text:find(String)`.


:mini:`meth (Sequence: xml::sequence):has(Fn: function): sequence`
   Equivalent to :mini:`Sequence ->? fun(X) some(Fn(X))`.


:mini:`meth (Sequence: xml::sequence):next(Args: any, ...): sequence`
   Generates the sequence :mini:`Nodeᵢ + Args` where :mini:`Nodeᵢ` are the nodes generated by :mini:`Sequence`.


:mini:`meth (Sequence: xml::sequence):parent(Args: any, ...): sequence`
   Generates the sequence :mini:`Nodeᵢ ^ Args` where :mini:`Nodeᵢ` are the nodes generated by :mini:`Sequence`.


:mini:`meth (Sequence: xml::sequence):prev(Args: any, ...): sequence`
   Generates the sequence :mini:`Nodeᵢ - Args` where :mini:`Nodeᵢ` are the nodes generated by :mini:`Sequence`.


.. _type-xml-text:

:mini:`type xml::text < xml, string`
   A XML text node.


:mini:`meth (Xml: xml::text):text: string`
   Returns the text content of :mini:`Xml`.


