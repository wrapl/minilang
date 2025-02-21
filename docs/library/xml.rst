.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

xml
===

.. rst-class:: mini-api

:mini:`meth xml::parse(String: address): xml`
   Returns :mini:`String` parsed into an XML node.


:mini:`meth xml::parse(Stream: stream): xml`
   Returns the contents of :mini:`Stream` parsed into an XML node.


:mini:`fun xml::escape(String: string): string`
   Escapes characters in :mini:`String`.

   .. code-block:: mini

      import: xml("fmt/xml")
      xml::escape("\'1 + 2 > 3 & 2 < 4\'")
      :> "\'1 + 2 &gt; 3 &amp; 2 &lt; 4\'"


:mini:`meth (Arg₁: xml::element):append(Arg₂: string, ...)`
   *TBD*


:mini:`meth (Arg₁: xml::element):append(Arg₂: string, Arg₃: map, ...)`
   *TBD*


:mini:`type xml`
   An XML node.


:mini:`meth xml(Stream: stream): xml`
   Returns the contents of :mini:`Stream` parsed into an XML node.


:mini:`meth xml(String: string): xml`
   Returns :mini:`String` parsed into an XML node.


:mini:`meth xml(Tag: symbol, ...): xml`
   Returns a new xml element with tag :mini:`Tag`,  adding attributes and children as :mini:`xml::element(...)`.


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


:mini:`meth <(Xml: xml): xml | nil`
   Returns the previous sibling of :mini:`Xml` or :mini:`nil`.


:mini:`meth (Xml: xml) < (N: integer): xml | nil`
   Returns the :mini:`N`-th prev sibling of :mini:`Xml` or :mini:`nil`.


:mini:`meth (Xml: xml) < (Attribute₁ is Value₁, ...): xml | nil`
   Returns the prev sibling of :mini:`Xml` with :mini:`Attribute₁ = Value₁`,  etc.,  if one exists,  otherwise :mini:`nil`.


:mini:`meth (Xml: xml) < (Tag: string): xml | nil`
   Returns the prev sibling of :mini:`Xml` with tag :mini:`Tag` if one exists,  otherwise :mini:`nil`.


:mini:`meth (Xml: xml) < (Tag: string, Attribute₁ is Value₁, ...): xml | nil`
   Returns the prev sibling of :mini:`Xml` with tag :mini:`Tag` and :mini:`Attribute₁ = Value₁`,  etc.,  if one exists,  otherwise :mini:`nil`.


:mini:`meth (Xml: xml) << (Fn: function): sequence`
   Returns a sequence of the previous siblings of :mini:`Xml` for which :mini:`Fn(Child)` is non-nil.


:mini:`meth (Xml: xml) << (Attribute₁ is Value₁, ...): sequence`
   Returns a sequence of the previous siblings of :mini:`Xml` with :mini:`Attribute₁ = Value₁`,  etc.


:mini:`meth (Xml: xml) << (Tag: string): sequence`
   Returns a sequence of the previous siblings of :mini:`Xml` with tag :mini:`Tag`.


:mini:`meth (Xml: xml) << (Tag: string, Attribute₁ is Value₁, ...): sequence`
   Returns a sequence of the previous siblings of :mini:`Xml` with tag :mini:`Tag` and :mini:`Attribute₁ = Value₁`,  etc.


:mini:`meth >(Xml: xml): xml | nil`
   Returns the next sibling of :mini:`Xml` or :mini:`nil`.


:mini:`meth (Xml: xml) > (N: integer): xml | nil`
   Returns the :mini:`N`-th next sibling of :mini:`Xml` or :mini:`nil`.


:mini:`meth (Xml: xml) > (Attribute₁ is Value₁, ...): xml | nil`
   Returns the next sibling of :mini:`Xml` with :mini:`Attribute₁ = Value₁`,  etc.,  if one exists,  otherwise :mini:`nil`.


:mini:`meth (Xml: xml) > (Tag: string): xml | nil`
   Returns the next sibling of :mini:`Xml` with tag :mini:`Tag` if one exists,  otherwise :mini:`nil`.


:mini:`meth (Xml: xml) > (Tag: string, Attribute₁ is Value₁, ...): xml | nil`
   Returns the next sibling of :mini:`Xml` with tag :mini:`Tag` and :mini:`Attribute₁ = Value₁`,  etc.,  if one exists,  otherwise :mini:`nil`.


:mini:`meth (Xml: xml) >> (Fn: function): sequence`
   Returns a sequence of the next siblings of :mini:`Xml` for which :mini:`Fn(Child)` is non-nil.


:mini:`meth (Xml: xml) >> (Attribute₁ is Value₁, ...): sequence`
   Returns a sequence of the next siblings of :mini:`Xml` with :mini:`Attribute₁ = Value₁`,  etc.


:mini:`meth (Xml: xml) >> (Tag: string): sequence`
   Returns a sequence of the next siblings of :mini:`Xml` with tag :mini:`Tag`.


:mini:`meth (Xml: xml) >> (Tag: string, Attribute₁ is Value₁, ...): sequence`
   Returns a sequence of the next siblings of :mini:`Xml` with tag :mini:`Tag` and :mini:`Attribute₁ = Value₁`,  etc.


:mini:`meth ^(Xml: xml): xml | nil`
   Returns the parent of :mini:`Xml` or :mini:`nil`.


:mini:`meth (Xml: xml) ^ (N: integer): xml | nil`
   Returns the :mini:`N`-th parent of :mini:`Xml` or :mini:`nil`.


:mini:`meth (Xml: xml) ^ (Attribute₁ is Value₁, ...): xml | nil`
   Returns the parent of :mini:`Xml` with :mini:`Attribute₁ = Value₁`,  etc.,  if one exists,  otherwise :mini:`nil`.


:mini:`meth (Xml: xml) ^ (Tag: string): xml | nil`
   Returns the parent of :mini:`Xml` with tag :mini:`Tag` if one exists,  otherwise :mini:`nil`.


:mini:`meth (Xml: xml) ^ (Tag: string, Attribute₁ is Value₁, ...): xml | nil`
   Returns the parent of :mini:`Xml` with tag :mini:`Tag` and :mini:`Attribute₁ = Value₁`,  etc.,  if one exists,  otherwise :mini:`nil`.


:mini:`meth (Node: xml):add_next(Other: any, ...): xml`
   Inserts :mini:`Other` directly after :mini:`Node`.


:mini:`meth (Node: xml):add_prev(Other: any, ...): xml`
   Inserts :mini:`Other` directly before :mini:`Node`.


:mini:`meth (Node: xml):index: integer | nil`
   Returns the index of :mini:`Node` in its parent or :mini:`nil`.


:mini:`meth (Node: xml):index(Text: boolean): integer | nil`
   Returns the index of :mini:`Node` in its parent including or excluding text nodes.


:mini:`meth (Xml: xml):next: xml | nil`
   Returns the next sibling of :mini:`Xml` or :mini:`nil`.


:mini:`meth (Xml: xml):next(N: integer): xml | nil`
   Returns the :mini:`N`-th next sibling of :mini:`Xml` or :mini:`nil`.


:mini:`meth (Xml: xml):next(Attribute₁ is Value₁, ...): xml | nil`
   Returns the next sibling of :mini:`Xml` with :mini:`Attribute₁ = Value₁`,  etc.,  if one exists,  otherwise :mini:`nil`.


:mini:`meth (Xml: xml):next(Tag: string): xml | nil`
   Returns the next sibling of :mini:`Xml` with tag :mini:`Tag` if one exists,  otherwise :mini:`nil`.


:mini:`meth (Xml: xml):next(Tag: string, Attribute₁ is Value₁, ...): xml | nil`
   Returns the next sibling of :mini:`Xml` with tag :mini:`Tag` and :mini:`Attribute₁ = Value₁`,  etc.,  if one exists,  otherwise :mini:`nil`.


:mini:`meth (Xml: xml):parent: xml | nil`
   Returns the parent of :mini:`Xml` or :mini:`nil`.


:mini:`meth (Xml: xml):parent(N: integer): xml | nil`
   Returns the :mini:`N`-th parent of :mini:`Xml` or :mini:`nil`.


:mini:`meth (Xml: xml):parent(Attribute₁ is Value₁, ...): xml | nil`
   Returns the parent of :mini:`Xml` with :mini:`Attribute₁ = Value₁`,  etc.,  if one exists,  otherwise :mini:`nil`.


:mini:`meth (Xml: xml):parent(Tag: string): xml | nil`
   Returns the parent of :mini:`Xml` with tag :mini:`Tag` if one exists,  otherwise :mini:`nil`.


:mini:`meth (Xml: xml):parent(Tag: string, Attribute₁ is Value₁, ...): xml | nil`
   Returns the parent of :mini:`Xml` with tag :mini:`Tag` and :mini:`Attribute₁ = Value₁`,  etc.,  if one exists,  otherwise :mini:`nil`.


:mini:`meth (Xml: xml):prev: xml | nil`
   Returns the previous sibling of :mini:`Xml` or :mini:`nil`.


:mini:`meth (Xml: xml):prev(N: integer): xml | nil`
   Returns the :mini:`N`-th prev sibling of :mini:`Xml` or :mini:`nil`.


:mini:`meth (Xml: xml):prev(Attribute₁ is Value₁, ...): xml | nil`
   Returns the prev sibling of :mini:`Xml` with :mini:`Attribute₁ = Value₁`,  etc.,  if one exists,  otherwise :mini:`nil`.


:mini:`meth (Xml: xml):prev(Tag: string): xml | nil`
   Returns the prev sibling of :mini:`Xml` with tag :mini:`Tag` if one exists,  otherwise :mini:`nil`.


:mini:`meth (Xml: xml):prev(Tag: string, Attribute₁ is Value₁, ...): xml | nil`
   Returns the prev sibling of :mini:`Xml` with tag :mini:`Tag` and :mini:`Attribute₁ = Value₁`,  etc.,  if one exists,  otherwise :mini:`nil`.


:mini:`meth (Node: xml):remove: xml`
   Removes :mini:`Node` from its parent.


:mini:`meth (Node₁: xml):replace(Node₂: xml): xml`
   Removes :mini:`Node₁` from its parent and replaces it with :mini:`Node₂`.


:mini:`meth (Arg₁: xml::element):append(Arg₂: xml, ...)`
   *TBD*


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
      :> error("XMLError", "Attribute values must be strings")


:mini:`meth (Parent: xml::element) :: (Attribute: string): string | nil`
   Returns the value of the :mini:`Attribute` attribute of :mini:`Parent`.


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


:mini:`meth (Arg₁: xml::element)[Arg₂: integer, Arg₃₁ is Value₁, ...]`
   *TBD*


:mini:`meth (Parent: xml::element)[Index: integer, Tag: string]: xml | nil`
   Returns the :mini:`Index`-th child of :mini:`Parent` with tag :mini:`Tag` or :mini:`nil`.


:mini:`meth (Arg₁: xml::element)[Arg₂: integer, Arg₃: string, Arg₄₁ is Value₁, ...]`
   *TBD*


:mini:`meth (Parent: xml::element)[Attribute: string]: string | nil`
   Returns the value of the :mini:`Attribute` attribute of :mini:`Parent`.


:mini:`meth (Xml: xml::element):attributes: map`
   Returns the attributes of :mini:`Xml`.


:mini:`meth (Parent: xml::element):empty: xml`
   Removes the contents of :mini:`Parent`.


:mini:`meth (Parent: xml::element):grow(Children: sequence, ...): xml`
   Adds each node generated by :mini:`Children` to :mini:`Parent` and returns :mini:`Parent`.


:mini:`meth (Parent: xml::element):put(Child: any, ...): xml`
   Adds :mini:`Child` to :mini:`Parent`.


:mini:`meth (Xml: xml::element):set(Attribute: string, Value: string): xml`
   Sets the value of attribute :mini:`Attribute` in :mini:`Xml` to :mini:`Value` and returns :mini:`Xml`.


:mini:`meth (Xml: xml::element):tag: string`
   Returns the tag of :mini:`Xml`.


:mini:`meth (Xml: xml::element):text: string`
   Returns the (recursive) text content of :mini:`Xml`.


:mini:`meth (Xml: xml::element):text(Sep: string): string`
   Returns the (recursive) text content of :mini:`Xml`,  adding :mini:`Sep` between the contents of adjacent nodes.


:mini:`meth (Buffer: string::buffer):append(Xml: xml::element)`
   Appends a string representation of :mini:`Xml` to :mini:`Buffer`.


:mini:`type xml::filter < function`
   An XML filter.


:mini:`meth xml::filter(Attr₁ is Value₁, ...): xml::filter`
   Returns an XML filter that checks if a node has attributes :mini:`Attrᵢ = Valueᵢ`.


:mini:`meth xml::filter(Tag: string, Attr₁ is Value₁, ...): xml::filter`
   Returns an XML filter that checks if a node has tag :mini:`Tag` and attributes :mini:`Attrᵢ = Valueᵢ`.


:mini:`type xml::parser < stream`
   A callback based streaming XML parser.


:mini:`fun xml::parser(Callback: any): xml::parser`
   Returns a new parser that calls :mini:`Callback(Xml)` each time a complete XML document is parsed.


:mini:`meth (Sequence: xml::sequence) / (Args: any, ...): sequence`
   Generates the sequence :mini:`Nodeᵢ / Args` where :mini:`Nodeᵢ` are the nodes generated by :mini:`Sequence`.


:mini:`meth (Sequence: xml::sequence) // (Args: any, ...): sequence`
   Generates the sequence :mini:`Nodeᵢ // Args` where :mini:`Nodeᵢ` are the nodes generated by :mini:`Sequence`.


:mini:`meth (Sequence: xml::sequence) < (Args: any, ...): sequence`
   Generates the sequence :mini:`Nodeᵢ < Args` where :mini:`Nodeᵢ` are the nodes generated by :mini:`Sequence`.


:mini:`meth (Sequence: xml::sequence) << (Args: any, ...): sequence`
   Generates the sequence :mini:`Nodeᵢ << Args` where :mini:`Nodeᵢ` are the nodes generated by :mini:`Sequence`.


:mini:`meth (Sequence: xml::sequence) > (Args: any, ...): sequence`
   Generates the sequence :mini:`Nodeᵢ > Args` where :mini:`Nodeᵢ` are the nodes generated by :mini:`Sequence`.


:mini:`meth (Sequence: xml::sequence) >> (Args: any, ...): sequence`
   Generates the sequence :mini:`Nodeᵢ >> Args` where :mini:`Nodeᵢ` are the nodes generated by :mini:`Sequence`.


:mini:`meth (Sequence: xml::sequence) ^ (Args: any, ...): sequence`
   Generates the sequence :mini:`Nodeᵢ ^ Args` where :mini:`Nodeᵢ` are the nodes generated by :mini:`Sequence`.


:mini:`meth (Sequence: xml::sequence):contains(Regex: regex): sequence`
   Equivalent to :mini:`Sequence ->? fun(X) X:text:find(Regex)`.


:mini:`meth (Sequence: xml::sequence):contains(String: string): sequence`
   Equivalent to :mini:`Sequence ->? fun(X) X:text:find(String)`.


:mini:`meth (Sequence: xml::sequence):has(Fn: function): sequence`
   Equivalent to :mini:`Sequence ->? fun(X) some(Fn(X))`.


:mini:`meth (Sequence: xml::sequence):next(Args: any, ...): sequence`
   Generates the sequence :mini:`Nodeᵢ > Args` where :mini:`Nodeᵢ` are the nodes generated by :mini:`Sequence`.


:mini:`meth (Sequence: xml::sequence):parent(Args: any, ...): sequence`
   Generates the sequence :mini:`Nodeᵢ ^ Args` where :mini:`Nodeᵢ` are the nodes generated by :mini:`Sequence`.


:mini:`meth (Sequence: xml::sequence):prev(Args: any, ...): sequence`
   Generates the sequence :mini:`Nodeᵢ < Args` where :mini:`Nodeᵢ` are the nodes generated by :mini:`Sequence`.


:mini:`type xml::text < xml, string`
   An XML text node.


:mini:`meth (Xml: xml::text):text: string`
   Returns the text content of :mini:`Xml`.


