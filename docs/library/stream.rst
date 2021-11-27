stream
======

:mini:`type stream < any`
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

:mini:`type stream::fd < stream`
   A file-descriptor based stream.


:mini:`meth (Stream: stream::fd):read(Dest: buffer): integer`
   Reads from :mini:`Stream` into :mini:`Dest` returning the actual number of bytes read.


:mini:`meth (Stream: stream::fd):write(Source: address): integer`
   Writes from :mini:`Source` to :mini:`Stream` returning the actual number of bytes written.


