.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

function
========

.. rst-class:: mini-api

:mini:`fun function::constant(Value: any): function::value`
   *TBD*


:mini:`fun function::variable(Value: any): function::value`
   *TBD*


:mini:`type function`
   The base type of all functions.


:mini:`meth (Function: function) ! (List: list): any`
   Calls :mini:`Function` with the values in :mini:`List` as positional arguments.


:mini:`meth (Function: function) ! (List: list, Map: map): any`
   Calls :mini:`Function` with the values in :mini:`List` as positional arguments and the keys and values in :mini:`Map` as named arguments.
   Returns an error if any of the keys in :mini:`Map` is not a string or method.


:mini:`meth (Function: function) ! (Map: map): any`
   Calls :mini:`Function` with the keys and values in :mini:`Map` as named arguments.
   Returns an error if any of the keys in :mini:`Map` is not a string or method.


:mini:`meth (Function: function) ! (Tuple: tuple): any`
   Calls :mini:`Function` with the values in :mini:`Tuple` as positional arguments.


:mini:`meth (Function: function) ! (Tuple: tuple, Map: map): any`
   Calls :mini:`Function` with the values in :mini:`Tuple` as positional arguments and the keys and values in :mini:`Map` as named arguments.
   Returns an error if any of the keys in :mini:`Map` is not a string or method.


:mini:`meth (Function: function) !! (List: list): function::partial`
   
   .. deprecated:: 2.7.0
   
      Use :mini:`$!` instead.
   
   Returns a function equivalent to :mini:`fun(Args...) Function(List[1],  List[2],  ...,  Args...)`.


:mini:`meth (Function: function) $ (Values: any, ...): function::partial`
   Returns a function equivalent to :mini:`fun(Args...) Function(Values...,  Args...)`.


:mini:`meth (Function: function) $! (List: list): function::partial`
   Returns a function equivalent to :mini:`fun(Args...) Function(List[1],  List[2],  ...,  Args...)`.


:mini:`meth (Base: function) -> (Function: function): chained`
   Returns a chained function equivalent to :mini:`Function(Base(...))`.

   .. code-block:: mini

      let F := :upper -> (3 * _)
      F("hello") :> "HELLOHELLOHELLO"
      F("cake") :> "CAKECAKECAKE"


:mini:`meth (Base: function) ->! (F: function): function`
   Returns a chained function equivalent to :mini:`F ! Base(...)`.

   .. code-block:: mini

      let F := list ->! 3 :> <chained>
      F("cat") :> "t"


:mini:`meth (Base: function) ->? (F: function): function`
   Returns a chained function equivalent to :mini:`Base(...){F(it)}`.

   .. code-block:: mini

      let F := 1 ->? (2 | _) -> (_ / 2) :> <chained>
      list(1 .. 10 -> F)
      :> [nil, 1, nil, 2, nil, 3, nil, 4, nil, 5]


:mini:`meth /(Function: function): function`
   Returns a function equivalent to :mini:`fun(Args...) Function()`.


:mini:`meth (Base: function) => (Function: function): chained`
   Returns a chained function equivalent to :mini:`Function(Base(Arg₁),  Base(Arg₂),  ...)`.

   .. code-block:: mini

      let F := :upper => +
      F("h", "e", "l", "l", "o") :> "HELLO"


:mini:`type function::partial < function, sequence`
   *TBD*


:mini:`fun function::partial(Function: function, Size: integer): function::partial`
   *TBD*


:mini:`meth (Arg₁: function::partial)[...]`
   *TBD*


:mini:`meth (Arg₁: function::partial):arity`
   *TBD*


:mini:`meth (Arg₁: function::partial):set`
   *TBD*


:mini:`type function::value < function`
   *TBD*


