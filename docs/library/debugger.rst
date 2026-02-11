.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

debugger
========

.. rst-class:: mini-api

:mini:`fun debugger(Function: any): debugger`
   Returns a new debugger for :mini:`Function()`.


:mini:`meth (Debugger: debugger):breakpoint_clear(Source: string, Line: integer)`
   Clears any breakpoints from :mini:`Source` at line :mini:`Line`.


:mini:`meth (Debugger: debugger):breakpoint_set(Source: string, Line: integer)`
   Sets a breakpoint in :mini:`Source` at line :mini:`Line`.


:mini:`meth (Debugger: debugger):continue(State: state, Value: any)`
   Resume :mini:`State` with :mini:`Value` in the debugger.


:mini:`meth (Debugger: debugger):error_mode(Set: any)`
   If :mini:`Set` is not :mini:`nil` then :mini:`Debugger` will stop on errors.


:mini:`meth (Debugger: debugger):step_in(State: state, Value: any)`
   Resume :mini:`State` with :mini:`Value` in the debugger,  stopping after the next line.


:mini:`meth (Debugger: debugger):step_mode(Set: any)`
   If :mini:`Set` is not :mini:`nil` then :mini:`Debugger` will stop on after each line.


:mini:`meth (Debugger: debugger):step_out(State: state, Value: any)`
   Resume :mini:`State` with :mini:`Value` in the debugger,  stopping at the end of the current function.


:mini:`meth (Debugger: debugger):step_over(State: state, Value: any)`
   Resume :mini:`State` with :mini:`Value` in the debugger,  stopping after the next line in the same function (i.e. stepping over function calls).


:mini:`meth (State: state):locals: list[string]`
   Returns the list of locals in :mini:`State`. Returns an empty list if :mini:`State` does not have any debugging information.


:mini:`meth (State: state):source: tuple[string,  integer]`
   Returns the source location for :mini:`State`.


:mini:`meth (State: state):trace: list[state]`
   Returns the call trace from :mini:`State`,  excluding states that do not have debugging information.


