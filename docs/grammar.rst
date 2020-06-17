Grammar
=======

Here is the complete grammar for *Minilang*.

.. productionlist:: minilang
   program        : { `declaration` | `expression` }
   expression     : `term` { `operator` `term` }
                  : `term` ":=" `expression`
                  : `term` "and" `expression`
                  : `term` "or" `expression`
                  : `term` "for" `identifier` [ "," `identifier` ] "in" `expression`
                  : `term`
   term           : `factor` `argument_list`
                  : `factor` "[" `expression_list` "]"
                  : `factor` `method` [ `argument_list` ]
                  : `factor`
   argument_list  : "(" [ `expression_list` ] ";" `parameter_list` ")" `expression`
                  : "(" [ `expression_list` ] ")"
   expression_list: `expression` { "," `expression` }
   factor         : `block_expression`
                  : `if_expression`
                  : `loop_expression`
                  : `for_expression`
                  : `all_expression`
                  : `not_expression`
                  : `while_expression`
                  : `until_expression`
                  : `exit_expression`
                  : `next_expression`
                  : `fun_expression`
                  : `ret_expression`
                  : `susp_expression`
                  : `with_expression`
                  : `identifier`
                  : `number`
                  : `string`
                  : `method`
                  : "nil"
                  : "old"
                  : "(" `expression` ")"
                  : `list_expression`
                  : `map_expression`
                  : `operator` `expression`
