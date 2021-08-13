#include "update_impl.h"

#define OP_ADD(A, B) A + B

UPDATE_ROW_OPS_IMPL(add, OP_ADD)
