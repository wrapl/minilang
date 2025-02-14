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


:mini:`type ast::expr::and < ast::expr::parent`
   An :mini:`and` expression
   


:mini:`type ast::expr::args < ast::expr`
   An :mini:`args` expression
   


:mini:`type ast::expr::assign < ast::expr::parent`
   An :mini:`assign` expression
   


:mini:`type ast::expr::blank < ast::expr`
   A :mini:`blank` expression
   


:mini:`type ast::expr::block < ast::expr`
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


:mini:`type ast::expr::call < ast::expr::parent`
   A :mini:`call` expression
   


:mini:`type ast::expr::constcall < ast::expr::parentvalue`
   A :mini:`const` :mini:`call` expression
   


:mini:`type ast::expr::debug < ast::expr::parent`
   A :mini:`debug` expression
   


:mini:`type ast::expr::def < ast::expr::local`
   A :mini:`def` expression
   


:mini:`type ast::expr::default < ast::expr`
   A :mini:`default` expression
   
   
   * :mini:`:child(Value: ast::expr::default): list[ast::expr]`
   * :mini:`:index(Value: ast::expr::default): integer`
   * :mini:`:flags(Value: ast::expr::default): integer`


:mini:`type ast::expr::defin < ast::expr::local`
   A :mini:`def` :mini:`in` expression
   


:mini:`type ast::expr::define < ast::expr::ident`
   A :mini:`define` expression
   


:mini:`type ast::expr::defunpack < ast::expr::local`
   A :mini:`def` :mini:`unpack` expression
   


:mini:`type ast::expr::delegate < ast::expr::parent`
   A :mini:`delegate` expression
   


:mini:`type ast::expr::each < ast::expr::parent`
   An :mini:`each` expression
   


:mini:`type ast::expr::exit < ast::expr::parent`
   An :mini:`exit` expression
   


:mini:`type ast::expr::for < ast::expr`
   A :mini:`for` expression
   
   
   * :mini:`:key(Value: ast::expr::for): string`
   * :mini:`:local(Value: ast::expr::for): list[ast::local]`
   * :mini:`:sequence(Value: ast::expr::for): list[ast::expr]`
   * :mini:`:body(Value: ast::expr::for): list[ast::expr]`
   * :mini:`:name(Value: ast::expr::for): string`
   * :mini:`:unpack(Value: ast::expr::for): integer`


:mini:`type ast::expr::fun < ast::expr`
   A :mini:`fun` expression
   
   
   * :mini:`:name(Value: ast::expr::fun): string`
   * :mini:`:params(Value: ast::expr::fun): list[ast::param]`
   * :mini:`:body(Value: ast::expr::fun): list[ast::expr]`
   * :mini:`:returntype(Value: ast::expr::fun): list[ast::expr]`


:mini:`type ast::expr::guard < ast::expr::parent`
   A :mini:`guard` expression
   


:mini:`type ast::expr::ident < ast::expr`
   An :mini:`ident` expression
   
   
   * :mini:`:ident(Value: ast::expr::ident): string`


:mini:`type ast::expr::if < ast::expr`
   An :mini:`if` expression
   
   
   * :mini:`:cases(Value: ast::expr::if): list[ast::ifcase]`
   * :mini:`:else(Value: ast::expr::if): list[ast::expr]`


:mini:`type ast::expr::ifconfig < ast::expr`
   An :mini:`if` :mini:`config` expression
   
   
   * :mini:`:child(Value: ast::expr::ifconfig): list[ast::expr]`
   * :mini:`:config(Value: ast::expr::ifconfig): string`


:mini:`type ast::expr::inline < ast::expr::parent`
   An :mini:`inline` expression
   


:mini:`type ast::expr::it < ast::expr`
   An :mini:`it` expression
   


:mini:`type ast::expr::let < ast::expr::local`
   A :mini:`let` expression
   


:mini:`type ast::expr::letin < ast::expr::local`
   A :mini:`let` :mini:`in` expression
   


:mini:`type ast::expr::letunpack < ast::expr::local`
   A :mini:`let` :mini:`unpack` expression
   


:mini:`type ast::expr::list < ast::expr::parent`
   A :mini:`list` expression
   


:mini:`type ast::expr::local < ast::expr`
   A :mini:`local` expression
   
   
   * :mini:`:local(Value: ast::expr::local): list[ast::local]`
   * :mini:`:child(Value: ast::expr::local): list[ast::expr]`
   * :mini:`:count(Value: ast::expr::local): integer`


:mini:`type ast::expr::loop < ast::expr::parent`
   A :mini:`loop` expression
   


:mini:`type ast::expr::map < ast::expr::parent`
   A :mini:`map` expression
   


:mini:`type ast::expr::next < ast::expr::parent`
   A :mini:`next` expression
   


:mini:`type ast::expr::nil < ast::expr`
   A :mini:`nil` expression
   


:mini:`type ast::expr::not < ast::expr::parent`
   A :mini:`not` expression
   


:mini:`type ast::expr::old < ast::expr`
   An :mini:`old` expression
   


:mini:`type ast::expr::or < ast::expr::parent`
   An :mini:`or` expression
   


:mini:`type ast::expr::parent < ast::expr`
   A :mini:`parent` expression
   
   
   * :mini:`:child(Value: ast::expr::parent): list[ast::expr]`
   * :mini:`:name(Value: ast::expr::parent): string`


:mini:`type ast::expr::parentvalue < ast::expr`
   A :mini:`parent` :mini:`value` expression
   
   
   * :mini:`:child(Value: ast::expr::parentvalue): list[ast::expr]`
   * :mini:`:value(Value: ast::expr::parentvalue): any`


:mini:`type ast::expr::recur < ast::expr`
   A :mini:`recur` expression
   


:mini:`type ast::expr::ref < ast::expr::local`
   A :mini:`ref` expression
   


:mini:`type ast::expr::refin < ast::expr::local`
   A :mini:`ref` :mini:`in` expression
   


:mini:`type ast::expr::refunpack < ast::expr::local`
   A :mini:`ref` :mini:`unpack` expression
   


:mini:`type ast::expr::register < ast::expr`
   A :mini:`register` expression
   


:mini:`type ast::expr::resolve < ast::expr::parentvalue`
   A :mini:`resolve` expression
   


:mini:`type ast::expr::return < ast::expr::parent`
   A :mini:`return` expression
   


:mini:`type ast::expr::scoped < ast::expr`
   A :mini:`scoped` expression
   


:mini:`type ast::expr::string < ast::expr`
   A :mini:`string` expression
   
   
   * :mini:`:parts(Value: ast::expr::string): list[ast::stringpart]`


:mini:`type ast::expr::subst < ast::expr`
   A :mini:`subst` expression
   


:mini:`type ast::expr::suspend < ast::expr::parent`
   A :mini:`suspend` expression
   


:mini:`type ast::expr::switch < ast::expr::parent`
   A :mini:`switch` expression
   


:mini:`type ast::expr::tuple < ast::expr::parent`
   A :mini:`tuple` expression
   


:mini:`type ast::expr::unknown < ast::expr`
   An :mini:`unknown` expression
   


:mini:`type ast::expr::value < ast::expr`
   A :mini:`value` expression
   
   
   * :mini:`:value(Value: ast::expr::value): any`


:mini:`type ast::expr::var < ast::expr::local`
   A :mini:`var` expression
   


:mini:`type ast::expr::varin < ast::expr::local`
   A :mini:`var` :mini:`in` expression
   


:mini:`type ast::expr::vartype < ast::expr::local`
   A :mini:`var` :mini:`type` expression
   


:mini:`type ast::expr::varunpack < ast::expr::local`
   A :mini:`var` :mini:`unpack` expression
   


:mini:`type ast::expr::with < ast::expr::local`
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


:mini:`type ast::names < list`
   *TBD*


:mini:`type ast::param`
   A param
   
   
   * :mini:`:ident(Value: ast::param): string`
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


