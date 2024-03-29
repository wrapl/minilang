.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

macro
=====

.. rst-class:: mini-api

:mini:`fun macro::block(): blockbuilder`
   Returns a new block builder.


:mini:`fun macro::call(): exprbuilder`
   Returns a new call builder.


:mini:`fun macro::list(): exprbuilder`
   Returns a new list builder.


:mini:`fun macro::map(): exprbuilder`
   Returns a new list builder.


:mini:`fun macro::tuple(): exprbuilder`
   Returns a new list builder.


:mini:`fun macro::value(Value: any): expr`
   Returns a new value expression.


:mini:`type block::builder`
   Utility object for building a block expression.


:mini:`meth (Builder: block::builder):do(Exprᵢ: expr, ...): blockbuilder`
   Adds each expression :mini:`Exprᵢ` to a block.


:mini:`meth (Builder: block::builder):end: expr`
   Finishes a block and returns it as an expression.


:mini:`meth (Builder: block::builder):let(Name: string, Expr: expr): blockbuilder`
   Adds a :mini:`let`-declaration to a block with initializer :mini:`Expr`.


:mini:`meth (Builder: block::builder):var(Name: string): blockbuilder`
   Adds a :mini:`var`-declaration to a block.


:mini:`meth (Builder: block::builder):var(Name: string, Expr: expr): blockbuilder`
   Adds a :mini:`var`-declaration to a block with initializer :mini:`Expr`.


:mini:`type expr`
   An expression value used by the compiler to implement macros.


:mini:`fun macro::subst(Expr: expr): macro`
   Returns a new macro which substitutes its arguments into :mini:`Expr`.


:mini:`meth (Expr: expr):scoped(Definitions: map): expr`
   Returns a new expression which wraps :mini:`Expr` with the constant definitions from :mini:`Definitions`.


:mini:`meth (Expr: expr):scoped(Module: module): expr`
   Returns a new expression which wraps :mini:`Expr` with the exports from :mini:`Module`.


:mini:`meth (Expr: expr):scoped(Name₁ is Value₁, ...): expr`
   Returns a new expression which wraps :mini:`Expr` with the constant definitions from :mini:`Names` and :mini:`Values`.


:mini:`meth (Expr: expr):scoped(Module: type): expr`
   Returns a new expression which wraps :mini:`Expr` with the exports from :mini:`Module`.


:mini:`meth (Expr: expr):subst(Names: list, Subs: list): expr`
   Returns a new expression which substitutes macro references to :mini:`:$Nameᵢ` with the corresponding expressions :mini:`Subᵢ`.


:mini:`meth (Expr: expr):subst(Subs: map, ...): expr`
   Returns a new expression which substitutes macro references to :mini:`:$Nameᵢ` with the corresponding expression :mini:`Subᵢ`.


:mini:`meth (Expr: expr):subst(Name₁ is Sub₁, ...): expr`
   Returns a new expression which substitutes macro references to :mini:`:$Nameᵢ` with the corresponding expression :mini:`Subᵢ`.


:mini:`type expr::builder`
   Utility object for building a block expression.


:mini:`meth (Builder: expr::builder):add(Expr: expr, ...): blockbuilder`
   Adds the expression :mini:`Expr` to a block.


:mini:`meth (Builder: expr::builder):end: expr`
   Finishes a block and returns it as an expression.


:mini:`type macro`
   A macro.


:mini:`fun macro(Function: function): macro`
   Returns a new macro which applies :mini:`Function` when compiled.
   :mini:`Function` should have the following signature: :mini:`Function(Expr₁: expr,  Expr₂: expr,  ...): expr`.


:mini:`fun macro::fun(Params: map, Arg₂: expr): expr`
   Returns a new function expression.


:mini:`fun macro::ident(Name: string): expr`
   Returns a new identifier expression.


