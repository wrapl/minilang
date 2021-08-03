#include "ml_table.h"
#include "ml_array.h"
#include "ml_macros.h"
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

typedef struct ml_table_t ml_table_t;
typedef struct ml_table_row_t ml_table_row_t;

struct ml_table_t {
	ml_type_t *Type;
	ml_value_t *Columns;
	int Size;
};

ML_TYPE(MLTableT, (MLIteratableT), "table");
// A table is a set of named arrays. The arrays must have the same length.

ml_value_t *ml_table() {
	ml_table_t *Table = new(ml_table_t);
	Table->Type = MLTableT;
	Table->Columns = ml_map();
	return (ml_value_t *)Table;
}

ML_METHOD(MLTableT) {
//>table
// Returns an empty table.
	return ml_table();
}

typedef struct {
	ml_state_t Base;
	ml_table_t *Table;
	ml_map_node_t *Node;
	ml_value_t *Args[1];
} ml_table_init_state_t;

static void ml_table_init_fun(ml_table_init_state_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Value)) ML_RETURN(Value);
	ml_table_t *Table = State->Table;
	if (!ml_array_degree(Value)) ML_ERROR("ValueError", "Cannot add empty array to table");
	int Size = ml_array_size(Value, 0);
	if (Table->Size) {
		if (Size != Table->Size) ML_ERROR("ValueError", "Array size does not match table");
	} else {
		Table->Size = Size;
	}
	ml_map_node_t *Node = State->Node;
	Node->Value = Value;
	Node = Node->Next;
	while (Node) {
		if (ml_is(Node->Value, MLArrayT)) {
			Node = Node->Next;
		} else {
			State->Node = Node;
			State->Args[0] = Node->Value;
			return ml_call((ml_state_t *)State, (ml_value_t *)MLArrayT, 1, State->Args);
		}
	}
	ML_RETURN(Table);
}

ML_METHODX(MLTableT, MLMapT) {
//<Columns
//>table
// Returns a table with the entries from :mini:`Columns`. The keys of :mini:`Columns` must be strings, the values of :mini:`Columns` are converted to arrays using :mini:`array()` if necessary.
	ml_table_t *Table = (ml_table_t *)ml_table();
	ML_MAP_FOREACH(Args[0], Iter) {
		ml_value_t *Key = Iter->Key;
		if (!ml_is(Key, MLStringT)) ML_ERROR("TypeError", "Column name must be a string");
		ml_map_insert(Table->Columns, Key, Iter->Value);
	}
	ml_map_node_t *Node = ((ml_map_t *)Table->Columns)->Head;
	while (Node) {
		if (ml_is(Node->Value, MLArrayT)) {
			ml_value_t *Value = Node->Value;
			if (!ml_array_degree(Value)) ML_ERROR("ValueError", "Cannot add empty array to table");
			int Size = ml_array_size(Value, 0);
			if (Table->Size) {
				if (Size != Table->Size) ML_ERROR("ValueError", "Array size does not match table");
			} else {
				Table->Size = Size;
			}
			Node = Node->Next;
		} else {
			ml_table_init_state_t *State = new(ml_table_init_state_t);
			State->Base.Caller = Caller;
			State->Base.Context = Caller->Context;
			State->Base.run = (ml_state_fn)ml_table_init_fun;
			State->Table = Table;
			State->Node = Node;
			State->Args[0] = Node->Value;
			return ml_call((ml_state_t *)State, (ml_value_t *)MLArrayT, 1, State->Args);
		}
	}
	ML_RETURN(Table);
}

ML_METHODVX(MLTableT, MLNamesT) {
//<Names
//<Value/1, ..., Value/n:any
//>table
// Returns a table using :mini:`Names` for column names and :mini:`Values` as column values, converted to arrays using :mini:`array()` if necessary.
	ml_table_t *Table = (ml_table_t *)ml_table();
	int N = 1;
	ML_NAMES_FOREACH(Args[0], Iter) ml_map_insert(Table->Columns, Iter->Value, Args[N++]);
	ml_map_node_t *Node = ((ml_map_t *)Table->Columns)->Head;
	while (Node) {
		if (ml_is(Node->Value, MLArrayT)) {
			ml_value_t *Value = Node->Value;
			if (!ml_array_degree(Value)) ML_ERROR("ValueError", "Cannot add empty array to table");
			int Size = ml_array_size(Value, 0);
			if (Table->Size) {
				if (Size != Table->Size) ML_ERROR("ValueError", "Array size does not match table");
			} else {
				Table->Size = Size;
			}
			Node = Node->Next;
		} else {
			ml_table_init_state_t *State = new(ml_table_init_state_t);
			State->Base.Caller = Caller;
			State->Base.Context = Caller->Context;
			State->Base.run = (ml_state_fn)ml_table_init_fun;
			State->Table = Table;
			State->Node = Node;
			State->Args[0] = Node->Value;
			return ml_call((ml_state_t *)State, (ml_value_t *)MLArrayT, 1, State->Args);
		}
	}
	ML_RETURN(Table);
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
//<Table
//<Name
//<Value
//>table
// Insert the column :mini:`Name` with values :mini:`Value` into :mini:`Table`.
	return ml_table_insert(Args[0], Args[1], Args[2]);
}

ML_METHODV("insert", MLTableT, MLNamesT) {
//<Table
//<Names
//<Value/1, ..., Value/n:array
//>table
// Insert columns with names from :mini:`Names` and values :mini:`Value/1`, ..., :mini:`Value/n` into :mini:`Table`.
	ml_table_t *Table = (ml_table_t *)Args[0];
	int N = 2;
	ML_NAMES_FOREACH(Args[1], Iter) {
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

ML_METHOD("delete", MLTableT, MLStringT) {
//<Table
//<Name
//>array
// Remove the column :mini:`Name` from :mini:`Table` and return the value array.
	return ml_map_delete(Args[0], Args[1]);
}

ML_METHOD(MLStringT, MLTableT) {
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
//<Table
//<Name
//>array
// Returns the column :mini:`Name` from :mini:`Table`.
	ml_table_t *Table = (ml_table_t *)Args[0];
	ml_value_t *Column = ml_map_search(Table->Columns, Args[1]);
	if (Column == MLNil) ML_RETURN(Column);
	Args[1] = Column;
	return ml_call(Caller, IndexMethod, Count - 1, Args + 1);
}

ML_METHODVX("::", MLTableT, MLStringT) {
//<Table
//<Name
//>array
// Returns the column :mini:`Name` from :mini:`Table`.
	ml_table_t *Table = (ml_table_t *)Args[0];
	ml_value_t *Column = ml_map_search(Table->Columns, Args[1]);
	if (Column == MLNil) ML_RETURN(Column);
	Args[1] = Column;
	return ml_call(Caller, IndexMethod, Count - 1, Args + 1);
}

struct ml_table_row_t {
	ml_type_t *Type;
	ml_table_t *Table;
	int Count;
	ml_value_t *Indices[];
};

static ml_value_t *table_row_assign(ml_table_row_t *Row, ml_value_t *Value) {
	ml_table_t *Table = Row->Table;
	if (ml_is(Value, MLMapT)) {
		ML_MAP_FOREACH(Value, Iter) {
			if (!ml_is(Iter->Key, MLStringT)) return ml_error("TypeError", "Column names must be strings");
			ml_value_t *Column = ml_map_search(Table->Columns, Iter->Key);
			if (Column == MLNil) return ml_error("NameError", "Column %s not in table", ml_string_value(Iter->Key));
			ml_value_t *Slot = ml_array_index((ml_array_t *)Column, Row->Count, Row->Indices);
			ml_value_t *Result = ml_assign(Slot, Iter->Value);
			if (ml_is_error(Result)) return Result;
		}
	} else if (ml_is(Value, MLListT)) {
		ml_map_node_t *Node = ((ml_map_t *)Table->Columns)->Head;
		ML_LIST_FOREACH(Value, Iter) {
			if (!Node) return ml_error("ValueError", "Too many columns in assignment");
			ml_value_t *Column = Node->Value;
			ml_value_t *Slot = ml_array_index((ml_array_t *)Column, Row->Count, Row->Indices);
			ml_value_t *Result = ml_assign(Slot, Iter->Value);
			if (ml_is_error(Result)) return Result;
			Node = Node->Next;
		}
	} else if (ml_is(Value, MLTupleT)) {
		ml_map_node_t *Node = ((ml_map_t *)Table->Columns)->Head;
		int Size = ml_tuple_size(Value);
		for (int Index = 1; Index <= Size; ++Index) {
			if (!Node) return ml_error("ValueError", "Too many columns in assignment");
			ml_value_t *Column = Node->Value;
			ml_value_t *Slot = ml_array_index((ml_array_t *)Column, Row->Count, Row->Indices);
			ml_value_t *Result = ml_assign(Slot, ml_tuple_get(Value, Index));
			if (ml_is_error(Result)) return Result;
			Node = Node->Next;
		}
	} else {
		return ml_error("TypeError", "Cannot assign %s to table row", ml_typeof(Value)->Name);
	}
	return Value;
}

ML_TYPE(MLTableRowT, (MLIteratableT), "table-row",
// A row in a table.
	.assign = (void *)table_row_assign
);

typedef struct {
	ml_type_t *Type;
	ml_table_t *Table;
	ml_value_t *Index;
} ml_table_iter_t;

ML_TYPE(MLTableIterT, (), "table-row-iter"
//!internal
);

static void ML_TYPED_FN(ml_iterate, MLTableT, ml_state_t *Caller, ml_table_t *Table) {
	if (!Table->Size) ML_RETURN(MLNil);
	ml_table_iter_t *Iter = new(ml_table_iter_t);
	Iter->Type = MLTableIterT;
	Iter->Table = Table;
	Iter->Index = ml_integer(1);
	ML_RETURN(Iter);
}

static void ML_TYPED_FN(ml_iter_next, MLTableIterT, ml_state_t *Caller, ml_table_iter_t *Iter) {
	int Index = ml_integer_value_fast(Iter->Index) + 1;
	if (Index > Iter->Table->Size) ML_RETURN(MLNil);
	Iter->Index = ml_integer(Index);
	ML_RETURN(Iter);
}

static void ML_TYPED_FN(ml_iter_key, MLTableIterT, ml_state_t *Caller, ml_table_iter_t *Iter) {
	ML_RETURN(Iter->Index);
}

static void ML_TYPED_FN(ml_iter_value, MLTableIterT, ml_state_t *Caller, ml_table_iter_t *Iter) {
	ml_table_row_t *Row = xnew(ml_table_row_t, 1, ml_value_t *);
	Row->Type = MLTableRowT;
	Row->Table = Iter->Table;
	Row->Count = 1;
	Row->Indices[0] = Iter->Index;
	ML_RETURN(Row);
}

ML_METHOD("[]", MLTableT, MLIntegerT) {
//<Table
//<Row
//>tablerow
// Returns the :mini:`Row`-th row of :mini:`Table`.
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
//<Row
//<Name
//>any
// Returns the value from column :mini:`Name` in :mini:`Row`.
	ml_table_row_t *Row = (ml_table_row_t *)Args[0];
	ml_value_t *Column = ml_map_search(Row->Table->Columns, Args[1]);
	if (Column == MLNil) return Column;
	return ml_array_index((ml_array_t *)Column, Row->Count, Row->Indices);
}

ML_METHOD("::", MLTableRowT, MLStringT) {
//<Row
//<Name
//>any
// Returns the value from column :mini:`Name` in :mini:`Row`.
	ml_table_row_t *Row = (ml_table_row_t *)Args[0];
	ml_value_t *Column = ml_map_search(Row->Table->Columns, Args[1]);
	if (Column == MLNil) return Column;
	return ml_array_index((ml_array_t *)Column, Row->Count, Row->Indices);
}

ML_METHOD(MLStringT, MLTableRowT) {
	ml_table_row_t *Row = (ml_table_row_t *)Args[0];
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	ml_stringbuffer_add(Buffer, "<", 1);
	int Comma = 0;
	ML_MAP_FOREACH(Row->Table->Columns, Iter) {
		if (Comma) ml_stringbuffer_add(Buffer, ", ", 2);
		ml_stringbuffer_append(Buffer, Iter->Key);
		ml_stringbuffer_add(Buffer, " is ", 4);
		ml_value_t *Value = ml_array_index((ml_array_t *)Iter->Value, Row->Count, Row->Indices);
		ml_stringbuffer_append(Buffer, Value);
		Comma = 1;
	}
	ml_stringbuffer_add(Buffer, ">", 1);
	return ml_stringbuffer_value(Buffer);
}

typedef struct {
	ml_type_t *Type;
	ml_map_node_t *Node;
	ml_table_row_t *Row;
} ml_table_row_iter_t;

ML_TYPE(MLTableRowIterT, (), "table-row-iter"
//!internal
);

static void ML_TYPED_FN(ml_iterate, MLTableRowT, ml_state_t *Caller, ml_table_row_t *Row) {
	ml_map_node_t *Node = ((ml_map_t *)Row->Table->Columns)->Head;
	if (!Node) ML_RETURN(MLNil);
	ml_table_row_iter_t *Iter = new(ml_table_row_iter_t);
	Iter->Type = MLTableRowIterT;
	Iter->Row = Row;
	Iter->Node = Node;
	ML_RETURN(Iter);
}

static void ML_TYPED_FN(ml_iter_next, MLTableRowIterT, ml_state_t *Caller, ml_table_row_iter_t *Iter) {
	ml_map_node_t *Node = Iter->Node->Next;
	if (!Node) ML_RETURN(MLNil);
	Iter->Node = Node;
	ML_RETURN(Iter);
}

static void ML_TYPED_FN(ml_iter_key, MLTableRowIterT, ml_state_t *Caller, ml_table_row_iter_t *Iter) {
	ML_RETURN(Iter->Node->Key);
}

static void ML_TYPED_FN(ml_iter_value, MLTableRowIterT, ml_state_t *Caller, ml_table_row_iter_t *Iter) {
	ML_RETURN(ml_array_index((ml_array_t *)Iter->Node->Value, Iter->Row->Count, Iter->Row->Indices));
}

#ifdef ML_CBOR

#include "ml_cbor.h"

static ml_value_t *ML_TYPED_FN(ml_cbor_write, MLTableT, ml_value_t *Table, char *Data, ml_cbor_write_fn WriteFn) {
	ml_cbor_write_tag(Data, WriteFn, 60);
	ml_cbor_write(ml_table_columns(Table), Data, WriteFn);
	return NULL;
}

static ml_value_t *ml_cbor_read_table_fn(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_TYPE(0, MLMapT);
	ml_value_t *Table = ml_table();
	ML_MAP_FOREACH(Args[0], Iter) {
		if (!ml_is(Iter->Key, MLStringT)) return ml_error("CborError", "Invalid table");
		if (!ml_is(Iter->Value, MLArrayT)) return ml_error("CborError", "Invalid table");
		ml_value_t *Result = ml_table_insert(Table, Iter->Key, Iter->Value);
		if (ml_is_error(Result)) return ml_error("CborError", "Invalid table");
	}
	return Table;
}

#endif

void ml_table_init(stringmap_t *Globals) {
#include "ml_table_init.c"
	stringmap_insert(Globals, "table", MLTableT);
#ifdef ML_CBOR
	ml_cbor_default_tag(60, NULL, ml_cbor_read_table_fn);
#endif
}
