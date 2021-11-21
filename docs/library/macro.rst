macro
=====

:mini:`type expr`
   An expression value used by the compiler to implement macros.


:mini:`meth (Expr: expr):scoped(Names: names, Values: any, ...): expr`
   Returns a new expression which wraps :mini:`Expr` with the constant definitions from :mini:`Names` and :mini:`Values`.


:mini:`meth (Expr: expr):scoped(Definitions: map): expr`
   Returns a new expression which wraps :mini:`Expr` with the constant definitions from :mini:`Definitions`.


:mini:`meth (Expr: expr):subst(Names: names, Exprs: expr, ...): expr`
   Returns a new expression which wraps substitutes macro references (e.g. :mini:`:$Name`) with expressions from :mini:`Names` and :mini:`Exprs`.


:mini:`meth (Expr: expr):subst(Names: list, Exprs: list): expr`
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


:mini:`meth (Builder: block::builder):var(Name: string): blockbuilder`
   Adds a :mini:`var`-declaration to a block.


:mini:`meth (Builder: block::builder):var(Name: string, Expr: expr): blockbuilder`
   Adds a :mini:`var`-declaration to a block with initializer :mini:`Expr`.


:mini:`meth (Builder: block::builder):let(Name: string, Expr: expr): blockbuilder`
   Adds a :mini:`let`-declaration to a block with initializer :mini:`Expr`.


:mini:`meth (Builder: block::builder):do(Expr...: expr, ...): blockbuilder`
   Adds the expression :mini:`Expr` to a block.


:mini:`meth (Builder: block::builder):end: expr`
   Finishes a block and returns it as an expression.


:mini:`fun macro::block(): blockbuilder`
   Returns a new block builder.


:mini:`type expr::builder`
   Utility object for building a block expression.


:mini:`fun macro::tuple(): exprbuilder`
   Returns a new list builder.


:mini:`fun macro::list(): exprbuilder`
   Returns a new list builder.


:mini:`fun macro::map(): exprbuilder`
   Returns a new list builder.


:mini:`fun macro::call(): exprbuilder`
   Returns a new call builder.


:mini:`meth (Builder: expr::builder):add(Expr...: expr, ...): blockbuilder`
   Adds the expression :mini:`Expr` to a block.


:mini:`meth (Builder: expr::builder):end: expr`
   Finishes a block and returns it as an expression.


