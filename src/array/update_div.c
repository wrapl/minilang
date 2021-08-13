#include "update_impl.h"

#define OP_DIV(A, B) A / B

UPDATE_ROW_OPS_IMPL(div, OP_DIV)

#define OP_RDIV(A, B) B / A

UPDATE_ROW_OPS_IMPL(rdiv, OP_RDIV)
