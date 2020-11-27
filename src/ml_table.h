#ifndef ML_TABLE_H
#define ML_TABLE_H

#include "minilang.h"

#ifdef	__cplusplus
extern "C" {
#endif

extern ml_type_t MLTableT[];

ml_value_t *ml_table();
ml_value_t *ml_table_insert(ml_value_t *Table, ml_value_t *Name, ml_value_t *Value);
ml_value_t *ml_table_columns(ml_value_t *Table);

#ifdef	__cplusplus
}
#endif

#endif
