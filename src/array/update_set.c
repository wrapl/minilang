#include "update_impl.h"

#define OP_SET(A, B) B

UPDATE_ROW_OPS_IMPL(set, OP_SET, OP_SET)
