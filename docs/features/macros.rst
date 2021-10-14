Macros
======

*Minilang* provides some support for meta-programming using macros. During compilation, function calls which meet certain criteria are treated as macro expansions:

#. The function being called must evaluate to a constant value at the *compilation* stage, and
#. The constant value is a :mini:`macro` value.

In this situation, the compiler applies the macro with the specified arguments passed as *expression* values. The macro must then return another *expression* value which the compiler then compiles in place of the original function call.

This method of implementing macros is different to most other languages that support macros in that the substitution is performed *during* compilation rather than *before*.

:mini:`macro` is a normal *Minilang* type and macros can be defined using a constant declaration (either :mini:`def name := macro(Callback)` or the compact alternative :mini:`macro: name(Callback)`). For example:

.. code-block:: mini

   macro: log(; Expr) do
      :{print('[{:$Source}:{:$Line}] {:$Expr}\n'), Expr is Expr, Source is macro::value(Expr:source), Line is macro::value(Expr:start)}
   end
   
   log(1 + 1)
 
.. code-block:: console

   [<console>:1] 2
