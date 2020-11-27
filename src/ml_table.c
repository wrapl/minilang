#include "ml_table.h"
#include "ml_array.h"
#include "ml_macros.h"
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

ML_METHOD_DECL(MLTableOf, "table::of");

typedef struct ml_table_t ml_table_t;
typedef struct ml_table_row_t ml_table_row_t;

struct ml_table_t {
	const ml_type_t *Type;
	ml_value_t *Columns;
	int Size;
};

ML_TYPE(MLTableT, (MLIteratableT), "table");

ml_value_t *ml_table() {
	ml_table_t *Table = new(ml_table_t);
	Table->Type = MLTableT;
	Table->Columns = ml_map();
	return (ml_value_t *)Table;
}

ML_METHOD(MLTableOfMethod) {
	return ml_table();
}

ML_METHOD(MLTableOfMethod, MLMapT) {
	ml_table_t *Table = (ml_table_t *)ml_table();
	ML_MAP_FOREACH(Args[0], Iter) {
		ml_value_t *Key = Iter->Key;
		ml_value_t *Value = Iter->Value;
		if (!ml_is(Key, MLStringT)) return ml_error("TypeError", "Column name must be a string");
		if (!ml_is(Value, MLArrayT)) return ml_error("TypeError", "Column value must be an array");
		if (!ml_array_degree(Value)) return ml_error("ValueError", "Cannot add empty array to table");
		int Size = ml_array_size(Value, 0);
		if (Table->Size) {
			if (Size != Table->Size) return ml_error("ValueError", "Array size does not match table");
		} else {
			Table->Size = Size;
		}
		ml_map_insert(Table->Columns, Key, Value);
	}
	return (ml_value_t *)Table;
}

ML_METHODV(MLTableOfMethod, MLNamesT) {
	ml_table_t *Table = (ml_table_t *)ml_table();
	int N = 1;
	ML_NAMES_FOREACH(Args[0], Iter) {
		ML_CHECK_ARG_TYPE(N, MLArrayT);
		ml_value_t *Value = Args[N++];
		if (!ml_array_degree(Value)) return ml_error("ValueError", "Cannot add empty array to table");
		int Size = ml_array_size(Value, 0);
		if (Table->Size) {
			if (Size != Table->Size) return ml_error("ValueError", "Array size does not match table");
		} else {
			Table->Size = Size;
		}
		ml_map_insert(Table->Columns, Iter->Value, Value);
	}
	return (ml_value_t *)Table;
}

ml_value_t *ml_table_insert(ml_value_t *Value, ml_value_t *Name, ml_value_t *Column) {
	ml_table_t *Table = (ml_table_t *)Value;
	if (!ml_array_degree(Column)) return ml_error("ValueError", "Cannot add empty array to table");
	int Size = ml_array_size(Column, 0);
	if (Table->Size) {
		if (Size != Table->Size) return ml_error("ValueError", "Array size does not match table");
	} else {
		Table->Size = Size;
	}
	ml_map_insert(Table->Columns, Name, Column);
	return Value;
}

ml_value_t *ml_table_columns(ml_value_t *Table) {
	return ((ml_table_t *)Table)->Columns;
}

ML_METHOD("insert", MLTableT, MLStringT, MLArrayT) {
	return ml_table_insert(Args[0], Args[1], Args[2]);
}

ML_METHOD("delete", MLTableT, MLStringT) {
	return ml_map_delete(Args[0], Args[1]);
}

ML_METHOD(MLStringOfMethod, MLTableT) {
	ml_table_t *Table = (ml_table_t *)Args[0];
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	ml_stringbuffer_add(Buffer, "table(", 6);
	int Comma = 0;
	ML_MAP_FOREACH(Table->Columns, Iter) {
		if (Comma) ml_stringbuffer_add(Buffer, ", ", 2);
		ml_stringbuffer_append(Buffer, Iter->Key);
		Comma = 1;
	}
	ml_stringbuffer_add(Buffer, ")", 1);
	return ml_stringbuffer_value(Buffer);
}

extern ml_value_t *IndexMethod;

ML_METHODVX("[]", MLTableT, MLStringT) {
	ml_table_t *Table = (ml_table_t *)Args[0];
	ml_value_t *Column = ml_map_search(Table->Columns, Args[1]);
	if (Column == MLNil) ML_RETURN(Column);
	Args[1] = Column;
	return ml_call(Caller, IndexMethod, Count - 1, Args + 1);
}

ML_METHODVX("::", MLTableT, MLStringT) {
	ml_table_t *Table = (ml_table_t *)Args[0];
	ml_value_t *Column = ml_map_search(Table->Columns, Args[1]);
	if (Column == MLNil) ML_RETURN(Column);
	Args[1] = Column;
	return ml_call(Caller, IndexMethod, Count - 1, Args + 1);
}

struct ml_table_row_t {
	const ml_type_t *Type;
	ml_table_t *Table;
	int Count;
	ml_value_t *Indices[];
};

ML_TYPE(MLTableRowT, (MLIteratableT), "table-row");

ML_METHOD("[]", MLTableT, MLIntegerT) {
	ml_table_t *Table = (ml_table_t *)Args[0];
	int Size = Table->Size;
	if (!Size) return MLNil;
	ml_table_row_t *Row = xnew(ml_table_row_t, Count - 1, ml_value_t *);
	Row->Type = MLTableRowT;
	Row->Table = Table;
	Row->Count = Count - 1;
	for (int I = 1; I < Count; ++I) Row->Indices[I - 1] = Args[I];
	return (ml_value_t *)Row;
}

ML_METHOD("[]", MLTableRowT, MLStringT) {
	ml_table_row_t *Row = (ml_table_row_t *)Args[0];
	ml_value_t *Column = ml_map_search(Row->Table->Columns, Args[1]);
	if (Column == MLNil) return Column;
	return ml_array_index((ml_array_t *)Column, Row->Count, Row->Indices);
}

ML_METHOD("::", MLTableRowT, MLStringT) {
	ml_table_row_t *Row = (ml_table_row_t *)Args[0];
	ml_value_t *Column = ml_map_search(Row->Table->Columns, Args[1]);
	if (Column == MLNil) return Column;
	return ml_array_index((ml_array_t *)Column, Row->Count, Row->Indices);
}

ML_METHOD(MLStringOfMethod, MLTableRowT) {
	ml_table_row_t *Row = (ml_table_row_t *)Args[0];
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	ml_stringbuffer_add(Buffer, "{", 1);
	int Comma = 0;
	ML_MAP_FOREACH(Row->Table->Columns, Iter) {
		if (Comma) ml_stringbuffer_add(Buffer, ", ", 2);
		ml_stringbuffer_append(Buffer, Iter->Key);
		ml_stringbuffer_add(Buffer, " is ", 4);
		ml_value_t *Value = ml_array_index((ml_array_t *)Iter->Value, Row->Count, Row->Indices);
		ml_stringbuffer_append(Buffer, Value);
		Comma = 1;
	}
	ml_stringbuffer_add(Buffer, "}", 1);
	return ml_stringbuffer_value(Buffer);
}

void ml_table_init(stringmap_t *Globals) {
#include "ml_table_init.c"
	MLTableT->Constructor = MLTableOfMethod;
	stringmap_insert(Globals, "table", MLTableT);
}
