Contexts
========

All code in *Minilang* executes in a *context*. The context defines inherited state such as the current debugger, scheduler, thread (if thread support is enabled) and method table. Additional context specific variables can also be defined. Unless otherwise modified, the context is inherited through each function call.

Context variables
-----------------

New context variables can be created using the :mini:`context()` function. Context variables are callable as functions, and either return the current value of the corresponding variable in the current context, or execute a function in a new context where the corresponding variable is bound to a specific value.

.. code-block:: mini

   let Var := context()
   
   Var(100;) do
      print('Current value of Var = {Var()}\n')
   end
   
   print('Current value of Var = {Var()}\n')


.. code-block:: console

   Current value of Var = 100
   Current value of Var = nil

Host context specific values
----------------------------

Context variables are intended to be defined and used at runtime within *Minilang* code. For faster and more controlled cases, additional context specific values can be defined by the host application when *Minilang* is embedded. Each context specific value is assigned an integer index making lookup faster than *Minilang* defined context variables.

.. c:function:: int ml_context_index()

   Registers a new context specific value and returns its index.

.. c:function:: ml_context_t *ml_context(ml_context_t *Parent)

   Creates a new context, inheriting context specific values from `Parent`.

.. c:function:: void ml_context_set(ml_context_t *Context, int Index, void *Value)

   Sets the context specific value with the specified index. `Context` should be a newly created context, it's usually incorrect to modify an existing context.

.. c:function:: void *ml_context_get(ml_context_t *Context, int Index)

   Gets the context specific value with the specified index.

Predefined indicies
...................

The following context specific value indices are predefined in *Minilang*. Note that additional context specific values may be defined in the future, the functions above are the only safe way to define and use new context specific values in a host application.

* :c:`ML_METHOD_INDEX`: used to hold the context specific method table. Can be used to restrict method definitions to specific contexts (e.g. in multi-tenant code).
* :c:`ML_DEBUGGER_INDEX`: used to hold the current debugger, if any.
* :c:`ML_SCHEDULER_INDEX`: used to hold information about the current scheduler.

