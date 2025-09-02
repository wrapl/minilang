#ifndef ML_TABLE_H
#define ML_TABLE_H

#include "minilang.h"
#include "ml_array.h"

#ifdef __cplusplus
extern "C" {
#endif

extern ml_type_t MLTableT[];

void ml_table_init(stringmap_t *Globals);
ml_value_t *ml_table();
ml_array_t *ml_table_insert(ml_value_t *Table, const char *Name, ml_value_t *Source);
ml_value_t *ml_table_columns(ml_value_t *Table);

#ifdef __cplusplus
}
#endif

#endif
