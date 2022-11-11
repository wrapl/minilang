.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

ast
===

.. rst-class:: mini-api

.. _type-ast-expr:

:mini:`type ast::expr`
   An expression
   
   * :mini:`:source(Value: ast::expr): string`
   * :mini:`:startline(Value: ast::expr): integer`
   * :mini:`:endline(Value: ast::expr): integer`


.. _type-ast-expr-and:

:mini:`type ast::expr::and < astparentexpr`
   An :mini:`and` expression
   


.. _type-ast-expr-assign:

:mini:`type ast::expr::assign < astparentexpr`
   An :mini:`assign` expression
   


.. _type-ast-expr-blank:

:mini:`type ast::expr::blank < astexpr`
   A :mini:`blank` expression
   


.. _type-ast-expr-block:

:mini:`type ast::expr::block < astexpr`
   A :mini:`block` expression
   
   * :mini:`:vars(Value: ast::expr::block): list[ast::local]`
   * :mini:`:lets(Value: ast::expr::block): list[ast::local]`
   * :mini:`:defs(Value: ast::expr::block): list[ast::local]`
   * :mini:`:child(Value: ast::expr::block): list[ast::expr]`
   * :mini:`:catchbody(Value: ast::expr::block): list[ast::expr]`
   * :mini:`:must(Value: ast::expr::block): list[ast::expr]`
   * :mini:`:catchident(Value: ast::expr::block): string`
   * :mini:`:numvars(Value: ast::expr::block): integer`
   * :mini:`:numlets(Value: ast::expr::block): integer`
   * :mini:`:numdefs(Value: ast::expr::block): integer`


.. _type-ast-expr-call:

:mini:`type ast::expr::call < astparentexpr`
   A :mini:`call` expression
   


.. _type-ast-expr-constcall:

:mini:`type ast::expr::constcall < astparentvalueexpr`
   A :mini:`const` :mini:`call` expression
   


.. _type-ast-expr-debug:

:mini:`type ast::expr::debug < astparentexpr`
   A :mini:`debug` expression
   


.. _type-ast-expr-def:

:mini:`type ast::expr::def < astlocalexpr`
   A :mini:`def` expression
   


.. _type-ast-expr-default:

:mini:`type ast::expr::default < astexpr`
   A :mini:`default` expression
   
   * :mini:`:child(Value: ast::expr::default): list[ast::expr]`
   * :mini:`:index(Value: ast::expr::default): integer`
   * :mini:`:flags(Value: ast::expr::default): integer`


.. _type-ast-expr-defin:

:mini:`type ast::expr::defin < astlocalexpr`
   A :mini:`def` :mini:`in` expression
   


.. _type-ast-expr-define:

:mini:`type ast::expr::define < astidentexpr`
   A :mini:`define` expression
   


.. _type-ast-expr-defunpack:

:mini:`type ast::expr::defunpack < astlocalexpr`
   A :mini:`def` :mini:`unpack` expression
   


.. _type-ast-expr-delegate:

:mini:`type ast::expr::delegate < astparentexpr`
   A :mini:`delegate` expression
   


.. _type-ast-expr-each:

:mini:`type ast::expr::each < astparentexpr`
   An :mini:`each` expression
   


.. _type-ast-expr-exit:

:mini:`type ast::expr::exit < astparentexpr`
   An :mini:`exit` expression
   


.. _type-ast-expr-for:

:mini:`type ast::expr::for < astexpr`
   A :mini:`for` expression
   
   * :mini:`:key(Value: ast::expr::for): string`
   * :mini:`:local(Value: ast::expr::for): list[ast::local]`
   * :mini:`:sequence(Value: ast::expr::for): list[ast::expr]`
   * :mini:`:body(Value: ast::expr::for): list[ast::expr]`
   * :mini:`:name(Value: ast::expr::for): string`
   * :mini:`:unpack(Value: ast::expr::for): integer`


.. _type-ast-expr-fun:

:mini:`type ast::expr::fun < astexpr`
   A :mini:`fun` expression
   
   * :mini:`:name(Value: ast::expr::fun): string`
   * :mini:`:params(Value: ast::expr::fun): list[ast::param]`
   * :mini:`:body(Value: ast::expr::fun): list[ast::expr]`
   * :mini:`:returntype(Value: ast::expr::fun): list[ast::expr]`


.. _type-ast-expr-guard:

:mini:`type ast::expr::guard < astparentexpr`
   A :mini:`guard` expression
   


.. _type-ast-expr-ident:

:mini:`type ast::expr::ident < astexpr`
   An :mini:`ident` expression
   
   * :mini:`:ident(Value: ast::expr::ident): string`


.. _type-ast-expr-if:

:mini:`type ast::expr::if < astexpr`
   An :mini:`if` expression
   
   * :mini:`:cases(Value: ast::expr::if): list[ast::ifcase]`
   * :mini:`:else(Value: ast::expr::if): list[ast::expr]`


.. _type-ast-expr-inline:

:mini:`type ast::expr::inline < astparentexpr`
   An :mini:`inline` expression
   


.. _type-ast-expr-it:

:mini:`type ast::expr::it < astexpr`
   An :mini:`it` expression
   


.. _type-ast-expr-let:

:mini:`type ast::expr::let < astlocalexpr`
   A :mini:`let` expression
   


.. _type-ast-expr-letin:

:mini:`type ast::expr::letin < astlocalexpr`
   A :mini:`let` :mini:`in` expression
   


.. _type-ast-expr-letunpack:

:mini:`type ast::expr::letunpack < astlocalexpr`
   A :mini:`let` :mini:`unpack` expression
   


.. _type-ast-expr-list:

:mini:`type ast::expr::list < astparentexpr`
   A :mini:`list` expression
   


.. _type-ast-expr-local:

:mini:`type ast::expr::local < astexpr`
   A :mini:`local` expression
   
   * :mini:`:local(Value: ast::expr::local): list[ast::local]`
   * :mini:`:child(Value: ast::expr::local): list[ast::expr]`
   * :mini:`:count(Value: ast::expr::local): integer`


.. _type-ast-expr-loop:

:mini:`type ast::expr::loop < astparentexpr`
   A :mini:`loop` expression
   


.. _type-ast-expr-map:

:mini:`type ast::expr::map < astparentexpr`
   A :mini:`map` expression
   


.. _type-ast-expr-names:

:mini:`type ast::expr::names < astvalueexpr`
   A :mini:`names` expression
   


.. _type-ast-expr-next:

:mini:`type ast::expr::next < astparentexpr`
   A :mini:`next` expression
   


.. _type-ast-expr-nil:

:mini:`type ast::expr::nil < astexpr`
   A :mini:`nil` expression
   


.. _type-ast-expr-not:

:mini:`type ast::expr::not < astparentexpr`
   A :mini:`not` expression
   


.. _type-ast-expr-old:

:mini:`type ast::expr::old < astexpr`
   An :mini:`old` expression
   


.. _type-ast-expr-or:

:mini:`type ast::expr::or < astparentexpr`
   An :mini:`or` expression
   


.. _type-ast-expr-parent:

:mini:`type ast::expr::parent < astexpr`
   A :mini:`parent` expression
   
   * :mini:`:child(Value: ast::expr::parent): list[ast::expr]`
   * :mini:`:name(Value: ast::expr::parent): string`


.. _type-ast-expr-parentvalue:

:mini:`type ast::expr::parentvalue < astexpr`
   A :mini:`parent` :mini:`value` expression
   
   * :mini:`:child(Value: ast::expr::parentvalue): list[ast::expr]`
   * :mini:`:value(Value: ast::expr::parentvalue): any`


.. _type-ast-expr-ref:

:mini:`type ast::expr::ref < astlocalexpr`
   A :mini:`ref` expression
   


.. _type-ast-expr-refin:

:mini:`type ast::expr::refin < astlocalexpr`
   A :mini:`ref` :mini:`in` expression
   


.. _type-ast-expr-refunpack:

:mini:`type ast::expr::refunpack < astlocalexpr`
   A :mini:`ref` :mini:`unpack` expression
   


.. _type-ast-expr-register:

:mini:`type ast::expr::register < astexpr`
   A :mini:`register` expression
   


.. _type-ast-expr-resolve:

:mini:`type ast::expr::resolve < astparentvalueexpr`
   A :mini:`resolve` expression
   


.. _type-ast-expr-return:

:mini:`type ast::expr::return < astparentexpr`
   A :mini:`return` expression
   


.. _type-ast-expr-scoped:

:mini:`type ast::expr::scoped < astexpr`
   A :mini:`scoped` expression
   


.. _type-ast-expr-string:

:mini:`type ast::expr::string < astexpr`
   A :mini:`string` expression
   
   * :mini:`:parts(Value: ast::expr::string): list[ast::stringpart]`


.. _type-ast-expr-subst:

:mini:`type ast::expr::subst < astexpr`
   A :mini:`subst` expression
   


.. _type-ast-expr-suspend:

:mini:`type ast::expr::suspend < astparentexpr`
   A :mini:`suspend` expression
   


.. _type-ast-expr-switch:

:mini:`type ast::expr::switch < astparentexpr`
   A :mini:`switch` expression
   


.. _type-ast-expr-tuple:

:mini:`type ast::expr::tuple < astparentexpr`
   A :mini:`tuple` expression
   


.. _type-ast-expr-unknown:

:mini:`type ast::expr::unknown < astexpr`
   An :mini:`unknown` expression
   


.. _type-ast-expr-value:

:mini:`type ast::expr::value < astexpr`
   A :mini:`value` expression
   
   * :mini:`:value(Value: ast::expr::value): any`


.. _type-ast-expr-var:

:mini:`type ast::expr::var < astlocalexpr`
   A :mini:`var` expression
   


.. _type-ast-expr-varin:

:mini:`type ast::expr::varin < astlocalexpr`
   A :mini:`var` :mini:`in` expression
   


.. _type-ast-expr-vartype:

:mini:`type ast::expr::vartype < astlocalexpr`
   A :mini:`var` :mini:`type` expression
   


.. _type-ast-expr-varunpack:

:mini:`type ast::expr::varunpack < astlocalexpr`
   A :mini:`var` :mini:`unpack` expression
   


.. _type-ast-expr-with:

:mini:`type ast::expr::with < astlocalexpr`
   A :mini:`with` expression
   


.. _type-ast-ifcase:

:mini:`type ast::ifcase`
   An if case
   
   * :mini:`:condition(Value: ast::ifcase): list[ast::expr]`
   * :mini:`:body(Value: ast::ifcase): list[ast::expr]`
   * :mini:`:local(Value: ast::ifcase): list[ast::local]`
   * :mini:`:token(Value: ast::ifcase): integer`


.. _type-ast-local:

:mini:`type ast::local`
   A local
   
   * :mini:`:ident(Value: ast::local): string`
   * :mini:`:line(Value: ast::local): integer`
   * :mini:`:index(Value: ast::local): integer`


.. _type-ast-param:

:mini:`type ast::param`
   A param
   
   * :mini:`:ident(Value: ast::param): string`
   * :mini:`:type(Value: ast::param): list[ast::expr]`
   * :mini:`:line(Value: ast::param): integer`
   * :mini:`:kind(Value: ast::param): ast::paramkind`


.. _type-ast-stringpart:

:mini:`type ast::stringpart`
   A string part
   
   * :mini:`:child(Value: ast::stringpart): list[ast::expr]`
   * :mini:`:chars(Value: ast::stringpart): string`
   * :mini:`:length(Value: ast::stringpart): integer`
   * :mini:`:line(Value: ast::stringpart): integer`


:mini:`meth (Expr: expr):ast: ast::expr`
   Returns a tuple describing the expression :mini:`Expr`.


.. _type-paramkind:

:mini:`type paramkind < enum`
   *TBD*


