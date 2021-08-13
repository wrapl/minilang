#include "update_impl.h"

#define OP_SUB(A, B) A - B

UPDATE_ROW_OPS_IMPL(sub, OP_SUB)

#define OP_RSUB(A, B) B - A

UPDATE_ROW_OPS_IMPL(rsub, OP_RSUB)
