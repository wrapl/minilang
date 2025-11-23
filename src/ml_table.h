#ifndef ML_TABLE_H
#define ML_TABLE_H

#include "minilang.h"
#include "ml_array.h"

#ifdef __cplusplus
extern "C" {
#endif

extern ml_type_t MLTableT[];
extern ml_type_t MLTableRowT[];

void ml_table_init(stringmap_t *Globals);
ml_value_t *ml_table();
ml_array_t *ml_table_insert(ml_value_t *Table, const char *Name, ml_value_t *Source);
ml_value_t *ml_table_columns(ml_value_t *Table);
int ml_table_column_foreach(ml_value_t *Table, void *Data, int (*fn)(ml_value_t *Name, ml_value_t *Values, void *Data));

ml_value_t *ml_table_row_table(ml_value_t *TableRow);
int ml_table_row_foreach(ml_value_t *TableRow, void *Data, int (*fn)(ml_value_t *, ml_value_t *, void *));

#ifdef __cplusplus
}
#endif

#endif
