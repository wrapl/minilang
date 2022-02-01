.. include:: <isonum.txt>

stream
======

:mini:`type stream`
   Base type of readable and writable byte streams.


:mini:`meth (Arg₁: stream):read(Arg₂: integer)`
   *TBD*

:mini:`meth (Arg₁: stream):readx(Arg₂: integer, Arg₃: string)`
   *TBD*

:mini:`meth (Arg₁: stream):readi(Arg₂: integer, Arg₃: string)`
   *TBD*

:mini:`meth (Arg₁: stream):read`
   *TBD*

:mini:`meth (Arg₁: stream):rest`
   *TBD*

:mini:`meth (Arg₁: stream):write(Arg₂: any, ...)`
   *TBD*

:mini:`meth (Arg₁: stream):copy(Arg₂: stream)`
   *TBD*

:mini:`meth (Arg₁: stream):copy(Arg₂: stream, Arg₃: integer)`
   *TBD*

:mini:`meth (Arg₁: stream):flush`
   *TBD*

:mini:`type stream::buffered < stream`
   *TBD*

:mini:`meth streambuffered(Arg₁: stream, Arg₂: integer)`
   *TBD*

:mini:`meth (Arg₁: stream::buffered):read(Arg₂: buffer)`
   *TBD*

:mini:`meth (Arg₁: stream::buffered):read(Arg₂: address)`
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


