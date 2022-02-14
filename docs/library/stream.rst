.. include:: <isonum.txt>

stream
======

:mini:`type stream`
   Base type of readable and writable byte streams.


:mini:`meth (Stream: stream):read(Buffer: buffer): integer`
   Reads bytes from :mini:`Stream` into :mini:`Buffer` to :mini:`Stream`. This method should be overridden for streams defined in Minilang.


:mini:`meth (Stream: stream):read(Delimiters: integer): string | nil`
   Returns the next text from :mini:`Stream`,  upto :mini:`Count` characters,  whichever comes first. Returns :mini:`nil` if :mini:`Stream` is empty.


:mini:`meth (Stream: stream):readx(Count: integer, Delimiters: string): string | nil`
   Returns the next text from :mini:`Stream`,  upto but excluding any character in :mini:`Delimiters` or :mini:`Count` characters,  whichever comes first. Returns :mini:`nil` if :mini:`Stream` is empty.


:mini:`meth (Stream: stream):readi(Count: integer, Delimiters: string): string | nil`
   Returns the next text from :mini:`Stream`,  upto and including any character in :mini:`Delimiters` or :mini:`Count` characters,  whichever comes first. Returns :mini:`nil` if :mini:`Stream` is empty.


:mini:`meth (Stream: stream):read: string | nil`
   Equivalent to :mini:`Stream:readi(SIZE_MAX,  '\n')`.


:mini:`meth (Stream: stream):rest: string | nil`
   Returns the remainder of :mini:`Stream` or :mini:`nil` if :mini:`Stream` is empty.


:mini:`meth (Stream: stream):write(Address: address): integer`
   Writes the bytes at :mini:`Address` to :mini:`Stream`. This method should be overridden for streams defined in Minilang.


:mini:`meth (Stream: stream):write(Value₁, ..., Valueₙ: any): integer`
   Writes each :mini:`Valueᵢ` in turn to :mini:`Stream`.


:mini:`meth (Source: stream):copy(Destination: stream): integer`
   Copies the remaining bytes from :mini:`Source` to :mini:`Destination`.


:mini:`meth (Source: stream):copy(Destination: stream, Count: integer): integer`
   Copies upto :mini:`Count` bytes from :mini:`Source` to :mini:`Destination`.


:mini:`meth (Stream: stream):flush`
   Flushes :mini:`Stream`. This method should be overridden for streams defined in Minilang.


:mini:`type stream::buffered < stream`
   *TBD*

:mini:`meth stream::buffered(Arg₁: stream, Arg₂: integer)`
   *TBD*

:mini:`meth (Arg₁: stream::buffered):read(Arg₂: buffer)`
   *TBD*

:mini:`meth (Arg₁: stream::buffered):write(Arg₂: address)`
   *TBD*

:mini:`meth (Arg₁: stream::buffered):flush`
   *TBD*

:mini:`meth (Arg₁: string::buffer):read(Arg₂: buffer)`
   *TBD*

:mini:`type stream::fd < stream`
   A file-descriptor based stream.


:mini:`meth (Stream: stream::fd):read(Dest: buffer): integer`
   Reads from :mini:`Stream` into :mini:`Dest` returning the actual number of bytes read.


:mini:`meth (Stream: stream::fd):write(Source: address): integer`
   Writes from :mini:`Source` to :mini:`Stream` returning the actual number of bytes written.


