macro
=====

:mini:`type expr`
   An expression value used by the compiler to implement macros.


:mini:`meth :scoped(Expr: expr, Names: names, Values: any, ...): expr`
   Returns a new expression which wraps :mini:`Expr` with the constant definitions from :mini:`Names` and :mini:`Values`.


:mini:`meth :scoped(Expr: expr, Definitions: map): expr`
   Returns a new expression which wraps :mini:`Expr` with the constant definitions from :mini:`Definitions`.


:mini:`meth :subst(Expr: expr, Names: names, Exprs: expr, ...): expr`
   Returns a new expression which wraps substitutes macro references (e.g. :mini:`:$Name`) with expressions from :mini:`Names` and :mini:`Exprs`.


:mini:`meth :subst(Expr: expr, Names: list, Exprs: list): expr`
   Returns a new expression which wraps substitutes macro references (e.g. :mini:`:$Name`) with expressions from :mini:`Names` and :mini:`Exprs`.


:mini:`fun macro(Function: function): macro`
   Returns a new macro which applies :mini:`Function` when compiled.

   :mini:`Function` should have the following signature: :mini:`Function(Expr₁: expr, Expr₂: expr, ...): expr`.


:mini:`type macro`
   A macro.


:mini:`fun macro::ident(Name: string): expr`
   Returns a new identifier expression.


:mini:`fun macro::value(Value: any): expr`
   Returns a new value expression.


:mini:`type block::builder`
   Utility object for building a block expression.


:mini:`meth :var(Builder: block::builder, Name: string): blockbuilder`
   Adds a :mini:`var`-declaration to a block.


:mini:`meth :var(Builder: block::builder, Name: string, Expr: expr): blockbuilder`
   Adds a :mini:`var`-declaration to a block with initializer :mini:`Expr`.


:mini:`meth :let(Builder: block::builder, Name: string, Expr: expr): blockbuilder`
   Adds a :mini:`let`-declaration to a block with initializer :mini:`Expr`.


:mini:`meth :do(Builder: block::builder, Expr...: expr, ...): blockbuilder`
   Adds the expression :mini:`Expr` to a block.


:mini:`meth :expr(Builder: block::builder): expr`
   Finishes a block and returns it as an expression.


:mini:`fun macro::block(): blockbuilder`
   Returns a new block builder.


:mini:`type expr::builder`
   Utility object for building a block expression.


:mini:`fun macro::list(): exprbuilder`
   Returns a new list builder.


:mini:`fun macro::list(): exprbuilder`
   Returns a new list builder.


:mini:`fun macro::list(): exprbuilder`
   Returns a new list builder.


:mini:`meth :add(Builder: expr::builder, Expr...: expr, ...): blockbuilder`
   Adds the expression :mini:`Expr` to a block.


:mini:`meth :expr(Builder: expr::builder): expr`
   Finishes a block and returns it as an expression.


