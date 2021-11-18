stream
======

:mini:`type stream < any`
   Base type of all byte streams.


:mini:`type stream::fd < stream`
   A file-descriptor based stream.


:mini:`meth (Stream: stream::fd):read(Dest: buffer): integer`
   Reads from :mini:`Stream` into :mini:`Dest` returning the actual number of bytes read.


:mini:`meth (Stream: stream::fd):write(Source: address): integer`
   Writes from :mini:`Source` to :mini:`Stream` returning the actual number of bytes written.


