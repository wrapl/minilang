.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

ast
===

.. rst-class:: mini-api

:mini:`type ast::expr`
   An expression
   
   
   * :mini:`:source(Value: ast::expr): string`
   * :mini:`:startline(Value: ast::expr): integer`
   * :mini:`:endline(Value: ast::expr): integer`


:mini:`type ast::expr::and < (AstParentExpr`
   An :mini:`and` expression
   


:mini:`type ast::expr::assign < (AstParentExpr`
   An :mini:`assign` expression
   


:mini:`type ast::expr::blank < (AstExpr`
   A :mini:`blank` expression
   


:mini:`type ast::expr::block < (AstExpr`
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


:mini:`type ast::expr::call < (AstParentExpr`
   A :mini:`call` expression
   


:mini:`type ast::expr::constcall < (AstParentValueExpr`
   A :mini:`const` :mini:`call` expression
   


:mini:`type ast::expr::debug < (AstParentExpr`
   A :mini:`debug` expression
   


:mini:`type ast::expr::def < (AstLocalExpr`
   A :mini:`def` expression
   


:mini:`type ast::expr::default < (AstExpr`
   A :mini:`default` expression
   
   
   * :mini:`:child(Value: ast::expr::default): list[ast::expr]`
   * :mini:`:index(Value: ast::expr::default): integer`
   * :mini:`:flags(Value: ast::expr::default): integer`


:mini:`type ast::expr::defin < (AstLocalExpr`
   A :mini:`def` :mini:`in` expression
   


:mini:`type ast::expr::define < (AstIdentExpr`
   A :mini:`define` expression
   


:mini:`type ast::expr::defunpack < (AstLocalExpr`
   A :mini:`def` :mini:`unpack` expression
   


:mini:`type ast::expr::delegate < (AstParentExpr`
   A :mini:`delegate` expression
   


:mini:`type ast::expr::each < (AstParentExpr`
   An :mini:`each` expression
   


:mini:`type ast::expr::exit < (AstParentExpr`
   An :mini:`exit` expression
   


:mini:`type ast::expr::for < (AstExpr`
   A :mini:`for` expression
   
   
   * :mini:`:key(Value: ast::expr::for): string`
   * :mini:`:local(Value: ast::expr::for): list[ast::local]`
   * :mini:`:sequence(Value: ast::expr::for): list[ast::expr]`
   * :mini:`:body(Value: ast::expr::for): list[ast::expr]`
   * :mini:`:name(Value: ast::expr::for): string`
   * :mini:`:unpack(Value: ast::expr::for): integer`


:mini:`type ast::expr::fun < (AstExpr`
   A :mini:`fun` expression
   
   
   * :mini:`:name(Value: ast::expr::fun): string`
   * :mini:`:params(Value: ast::expr::fun): list[ast::param]`
   * :mini:`:body(Value: ast::expr::fun): list[ast::expr]`
   * :mini:`:returntype(Value: ast::expr::fun): list[ast::expr]`


:mini:`type ast::expr::guard < (AstParentExpr`
   A :mini:`guard` expression
   


:mini:`type ast::expr::ident < (AstExpr`
   An :mini:`ident` expression
   
   
   * :mini:`:ident(Value: ast::expr::ident): string`


:mini:`type ast::expr::if < (AstExpr`
   An :mini:`if` expression
   
   
   * :mini:`:cases(Value: ast::expr::if): list[ast::ifcase]`
   * :mini:`:else(Value: ast::expr::if): list[ast::expr]`


:mini:`type ast::expr::ifconfig < (AstExpr`
   An :mini:`if` :mini:`config` expression
   
   
   * :mini:`:child(Value: ast::expr::ifconfig): list[ast::expr]`
   * :mini:`:config(Value: ast::expr::ifconfig): string`


:mini:`type ast::expr::inline < (AstParentExpr`
   An :mini:`inline` expression
   


:mini:`type ast::expr::it < (AstExpr`
   An :mini:`it` expression
   


:mini:`type ast::expr::let < (AstLocalExpr`
   A :mini:`let` expression
   


:mini:`type ast::expr::letin < (AstLocalExpr`
   A :mini:`let` :mini:`in` expression
   


:mini:`type ast::expr::letunpack < (AstLocalExpr`
   A :mini:`let` :mini:`unpack` expression
   


:mini:`type ast::expr::list < (AstParentExpr`
   A :mini:`list` expression
   


:mini:`type ast::expr::local < (AstExpr`
   A :mini:`local` expression
   
   
   * :mini:`:local(Value: ast::expr::local): list[ast::local]`
   * :mini:`:child(Value: ast::expr::local): list[ast::expr]`
   * :mini:`:count(Value: ast::expr::local): integer`


:mini:`type ast::expr::loop < (AstParentExpr`
   A :mini:`loop` expression
   


:mini:`type ast::expr::map < (AstParentExpr`
   A :mini:`map` expression
   


:mini:`type ast::expr::next < (AstParentExpr`
   A :mini:`next` expression
   


:mini:`type ast::expr::nil < (AstExpr`
   A :mini:`nil` expression
   


:mini:`type ast::expr::not < (AstParentExpr`
   A :mini:`not` expression
   


:mini:`type ast::expr::old < (AstExpr`
   An :mini:`old` expression
   


:mini:`type ast::expr::or < (AstParentExpr`
   An :mini:`or` expression
   


:mini:`type ast::expr::parent < (AstExpr`
   A :mini:`parent` expression
   
   
   * :mini:`:child(Value: ast::expr::parent): list[ast::expr]`
   * :mini:`:name(Value: ast::expr::parent): string`


:mini:`type ast::expr::parentvalue < (AstExpr`
   A :mini:`parent` :mini:`value` expression
   
   
   * :mini:`:child(Value: ast::expr::parentvalue): list[ast::expr]`
   * :mini:`:value(Value: ast::expr::parentvalue): any`


:mini:`type ast::expr::ref < (AstLocalExpr`
   A :mini:`ref` expression
   


:mini:`type ast::expr::refin < (AstLocalExpr`
   A :mini:`ref` :mini:`in` expression
   


:mini:`type ast::expr::refunpack < (AstLocalExpr`
   A :mini:`ref` :mini:`unpack` expression
   


:mini:`type ast::expr::register < (AstExpr`
   A :mini:`register` expression
   


:mini:`type ast::expr::resolve < (AstParentValueExpr`
   A :mini:`resolve` expression
   


:mini:`type ast::expr::return < (AstParentExpr`
   A :mini:`return` expression
   


:mini:`type ast::expr::scoped < (AstExpr`
   A :mini:`scoped` expression
   


:mini:`type ast::expr::string < (AstExpr`
   A :mini:`string` expression
   
   
   * :mini:`:parts(Value: ast::expr::string): list[ast::stringpart]`


:mini:`type ast::expr::subst < (AstExpr`
   A :mini:`subst` expression
   


:mini:`type ast::expr::suspend < (AstParentExpr`
   A :mini:`suspend` expression
   


:mini:`type ast::expr::switch < (AstParentExpr`
   A :mini:`switch` expression
   


:mini:`type ast::expr::tuple < (AstParentExpr`
   A :mini:`tuple` expression
   


:mini:`type ast::expr::unknown < (AstExpr`
   An :mini:`unknown` expression
   


:mini:`type ast::expr::value < (AstExpr`
   A :mini:`value` expression
   
   
   * :mini:`:value(Value: ast::expr::value): any`


:mini:`type ast::expr::var < (AstLocalExpr`
   A :mini:`var` expression
   


:mini:`type ast::expr::varin < (AstLocalExpr`
   A :mini:`var` :mini:`in` expression
   


:mini:`type ast::expr::vartype < (AstLocalExpr`
   A :mini:`var` :mini:`type` expression
   


:mini:`type ast::expr::varunpack < (AstLocalExpr`
   A :mini:`var` :mini:`unpack` expression
   


:mini:`type ast::expr::with < (AstLocalExpr`
   A :mini:`with` expression
   


:mini:`type ast::ifcase`
   An if case
   
   
   * :mini:`:condition(Value: ast::ifcase): list[ast::expr]`
   * :mini:`:body(Value: ast::ifcase): list[ast::expr]`
   * :mini:`:local(Value: ast::ifcase): list[ast::local]`
   * :mini:`:token(Value: ast::ifcase): integer`


:mini:`type ast::local`
   A local
   
   
   * :mini:`:ident(Value: ast::local): string`
   * :mini:`:line(Value: ast::local): integer`
   * :mini:`:index(Value: ast::local): integer`


:mini:`type ast::names < (MLList`
   *TBD*


:mini:`type ast::param`
   A param
   
   
   * :mini:`:ident(Value: ast::param): string`
   * :mini:`:type(Value: ast::param): list[ast::expr]`
   * :mini:`:line(Value: ast::param): integer`
   * :mini:`:kind(Value: ast::param): ast::paramkind`


:mini:`type ast::stringpart`
   A string part
   
   
   * :mini:`:child(Value: ast::stringpart): list[ast::expr]`
   * :mini:`:chars(Value: ast::stringpart): string`
   * :mini:`:length(Value: ast::stringpart): integer`
   * :mini:`:line(Value: ast::stringpart): integer`


:mini:`meth (Expr: expr):ast: ast::expr`
   Returns a tuple describing the expression :mini:`Expr`.


:mini:`type paramkind < enum`
   * :mini:`::Default`
   * :mini:`::Extra`
   * :mini:`::Named`
   * :mini:`::ByRef`
   * :mini:`::AsVar`


