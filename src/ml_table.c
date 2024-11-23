#include "ml_table.h"
#include "ml_array.h"
#include "ml_macros.h"
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

#undef ML_CATEGORY
#define ML_CATEGORY "table"

typedef struct ml_table_t ml_table_t;
typedef struct ml_table_row_t ml_table_row_t;

struct ml_table_t {
	ml_type_t *Type;
	ml_value_t *ColumnNames;
	ml_table_row_t *Rows;
	size_t Size;
};

struct ml_table_row_t {
	ml_type_t *Type;
	ml_table_t *Table;
};

ML_TYPE(MLTableT, (MLSequenceT), "table");
// A table is a set of named arrays. The arrays must have the same length.

static inline ml_value_t *ml_table_row_index(ml_table_row_t *Row) {
	return ml_integer(Row + 1 - Row->Table->Rows);
}

static void ml_table_row_assign(ml_state_t *Caller, ml_table_row_t *Row, ml_value_t *Value) {
	ml_table_t *Table = Row->Table;
	ml_value_t *Indices[1] = {ml_table_row_index(Row)};
	if (ml_is(Value, MLMapT)) {
		ML_MAP_FOREACH(Value, Iter) {
			if (!ml_is(Iter->Key, MLStringT)) ML_ERROR("TypeError", "Column names must be strings");
			ml_value_t *Column = ml_map_search(Table->ColumnNames, Iter->Key);
			if (Column == MLNil) ML_ERROR("NameError", "Column %s not in table", ml_string_value(Iter->Key));
			ml_value_t *Slot = ml_array_index((ml_array_t *)Column, 1, Indices);
			ml_value_t *Result = ml_simple_assign(Slot, Iter->Value);
			if (ml_is_error(Result)) ML_RETURN(Result);
		}
	} else if (ml_is(Value, MLListT)) {
		ml_map_node_t *Node = ((ml_map_t *)Table->ColumnNames)->Head;
		ML_LIST_FOREACH(Value, Iter) {
			if (!Node) ML_ERROR("ValueError", "Too many columns in assignment");
			ml_value_t *Column = Node->Value;
			ml_value_t *Slot = ml_array_index((ml_array_t *)Column, 1, Indices);
			ml_value_t *Result = ml_simple_assign(Slot, Iter->Value);
			if (ml_is_error(Result)) ML_RETURN(Result);
			Node = Node->Next;
		}
	} else if (ml_is(Value, MLTupleT)) {
		ml_map_node_t *Node = ((ml_map_t *)Table->ColumnNames)->Head;
		int Size = ml_tuple_size(Value);
		for (int Index = 1; Index <= Size; ++Index) {
			if (!Node) ML_ERROR("ValueError", "Too many columns in assignment");
			ml_value_t *Column = Node->Value;
			ml_value_t *Slot = ml_array_index((ml_array_t *)Column, 1, Indices);
			ml_value_t *Result = ml_simple_assign(Slot, ml_tuple_get(Value, Index));
			if (ml_is_error(Result)) ML_RETURN(Result);
			Node = Node->Next;
		}
	} else {
		ML_ERROR("TypeError", "Cannot assign %s to table row", ml_typeof(Value)->Name);
	}
	ML_RETURN(Value);
}

ML_TYPE(MLTableRowT, (MLSequenceT), "table-row",
// A row in a table.
	.assign = (void *)ml_table_row_assign
);

ml_value_t *ml_table() {
	ml_table_t *Table = new(ml_table_t);
	Table->Type = MLTableT;
	Table->ColumnNames = ml_map();
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
	size_t Size = ml_array_size(Value, 0);
	if (Table->Rows) {
		if (Size != Table->Size) ML_ERROR("ValueError", "Array size does not match table");
	} else {
		Table->Size = Size;
		Table->Rows = anew(ml_table_row_t, Size + 1);
		for (int I = 0; I < Size; ++I) {
			Table->Rows[I].Type = MLTableRowT;
			Table->Rows[I].Table = Table;
		}
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
		ml_map_insert(Table->ColumnNames, Key, Iter->Value);
	}
	ml_map_node_t *Node = ((ml_map_t *)Table->ColumnNames)->Head;
	while (Node) {
		if (ml_is(Node->Value, MLArrayT)) {
			ml_value_t *Value = Node->Value;
			if (!ml_array_degree(Value)) ML_ERROR("ValueError", "Cannot add empty array to table");
			size_t Size = ml_array_size(Value, 0);
			if (Table->Rows) {
				if (Size != Table->Size) ML_ERROR("ValueError", "Array size does not match table");
			} else {
				Table->Size = Size;
				Table->Rows = anew(ml_table_row_t, Size + 1);
				for (int I = 0; I < Size; ++I) {
					Table->Rows[I].Type = MLTableRowT;
					Table->Rows[I].Table = Table;
				}
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
	ML_NAMES_CHECKX_ARG_COUNT(0);
	ml_table_t *Table = (ml_table_t *)ml_table();
	int N = 1;
	ML_NAMES_FOREACH(Args[0], Iter) ml_map_insert(Table->ColumnNames, Iter->Value, Args[N++]);
	ml_map_node_t *Node = ((ml_map_t *)Table->ColumnNames)->Head;
	while (Node) {
		if (ml_is(Node->Value, MLArrayT)) {
			ml_value_t *Value = Node->Value;
			if (!ml_array_degree(Value)) ML_ERROR("ValueError", "Cannot add empty array to table");
			size_t Size = ml_array_size(Value, 0);
			if (Table->Rows) {
				if (Size != Table->Size) ML_ERROR("ValueError", "Array size does not match table");
			} else {
				Table->Size = Size;
				Table->Rows = anew(ml_table_row_t, Size + 1);
				for (int I = 0; I < Size; ++I) {
					Table->Rows[I].Type = MLTableRowT;
					Table->Rows[I].Table = Table;
				}
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

typedef struct {
	ml_state_t Base;
	ml_table_t *Table;
	ml_value_t *Names, *Values;
	ml_value_t *List;
	int Index, Count;
} ml_table_pivot_state_t;

static void ml_table_pivot_state_run(ml_table_pivot_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	ml_map_insert(State->Table->ColumnNames, ml_list_get(State->Names, State->Index), Value);
	if (State->Index == State->Count) ML_CONTINUE(State->Base.Caller, State->Table);
	int Index = ++State->Index;
	ml_list_node_t *Node = ((ml_list_t *)State->List)->Head;
	ML_LIST_FOREACH(State->Values, Iter) {
		Node->Value = ml_list_get(Iter->Value, Index);
		Node = Node->Next;
	}
	return ml_call((ml_state_t *)State, (ml_value_t *)MLArrayT, 1, &State->List);
}

ML_METHODX(MLTableT, MLListT, MLListT) {
//<Names
//<Rows
//>table
// Returns a table using :mini:`Names` for column names and :mini:`Rows` as rows, where each row in :mini:`Rows` is a list of values corresponding to :mini:`Names`.
	ml_table_t *Table = (ml_table_t *)ml_table();
	int N = ml_list_length(Args[0]);
	ML_LIST_FOREACH(Args[0], Iter) {
		if (!ml_is(Iter->Value, MLStringT)) ML_ERROR("TypeError", "Expected string not %s", ml_typeof(Iter->Value)->Name);
	}
	ML_LIST_FOREACH(Args[1], Iter) {
		if (!ml_is(Iter->Value, MLListT)) ML_ERROR("TypeError", "Expected list not %s", ml_typeof(Iter->Value)->Name);
		if (ml_list_length(Iter->Value) != N) ML_ERROR("ValueError", "List lengths do not match");
	}
	if (!N) ML_RETURN(Table);
	int Size = ml_list_length(Args[1]);
	Table->Size = Size;
	Table->Rows = anew(ml_table_row_t, Size + 1);
	for (int I = 0; I < Size; ++I) {
		Table->Rows[I].Type = MLTableRowT;
		Table->Rows[I].Table = Table;
	}
	ml_value_t *List = ml_list();
	for (int I = 0; I < Size; ++I) ml_list_put(List, MLNil);
	ml_table_pivot_state_t *State = new(ml_table_pivot_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_table_pivot_state_run;
	State->Table = Table;
	State->Names = Args[0];
	State->Values = Args[1];
	State->List = List;
	State->Index = 1;
	State->Count = N;
	ml_list_node_t *Node = ((ml_list_t *)List)->Head;
	ML_LIST_FOREACH(Args[1], Iter) {
		Node->Value = ml_list_get(Iter->Value, 1);
		Node = Node->Next;
	}
	return ml_call((ml_state_t *)State, (ml_value_t *)MLArrayT, 1, &State->List);
}

ml_value_t *ml_table_insert(ml_value_t *Value, ml_value_t *Name, ml_value_t *Column) {
	ml_table_t *Table = (ml_table_t *)Value;
	if (!ml_array_degree(Column)) return ml_error("ValueError", "Cannot add empty array to table");
	size_t Size = ml_array_size(Column, 0);
	if (Table->Rows) {
		if (Size != Table->Size) return ml_error("ValueError", "Array size does not match table");
	} else {
		Table->Size = Size;
		Table->Rows = anew(ml_table_row_t, Size + 1);
		for (int I = 0; I < Size; ++I) {
			Table->Rows[I].Type = MLTableRowT;
			Table->Rows[I].Table = Table;
		}
	}
	ml_map_insert(Table->ColumnNames, Name, Column);
	return Value;
}

ml_value_t *ml_table_columns(ml_value_t *Table) {
	return ((ml_table_t *)Table)->ColumnNames;
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
	ML_NAMES_CHECK_ARG_COUNT(1);
	ml_table_t *Table = (ml_table_t *)Args[0];
	int N = 2;
	ML_NAMES_FOREACH(Args[1], Iter) {
		ML_CHECK_ARG_TYPE(N, MLArrayT);
		ml_value_t *Value = Args[N++];
		if (!ml_array_degree(Value)) return ml_error("ValueError", "Cannot add empty array to table");
		size_t Size = ml_array_size(Value, 0);
		if (Table->Rows) {
			if (Size != Table->Size) return ml_error("ValueError", "Array size does not match table");
		} else {
			Table->Size = Size;
			Table->Rows = anew(ml_table_row_t, Size + 1);
			for (int I = 0; I < Size; ++I) {
				Table->Rows[I].Type = MLTableRowT;
				Table->Rows[I].Table = Table;
			}
		}
		ml_map_insert(Table->ColumnNames, Iter->Value, Value);
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

ML_METHOD("append", MLStringBufferT, MLTableT) {
//<Buffer
//<Value
// Appends a representation of :mini:`Value` to :mini:`Buffer`.
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_table_t *Table = (ml_table_t *)Args[1];
	ml_stringbuffer_write(Buffer, "table(", 6);
	int Comma = 0;
	ML_MAP_FOREACH(Table->ColumnNames, Iter) {
		if (Comma) ml_stringbuffer_write(Buffer, ", ", 2);
		ml_stringbuffer_simple_append(Buffer, Iter->Key);
		Comma = 1;
	}
	ml_stringbuffer_put(Buffer, ')');
	return MLSome;
}

extern ml_value_t *IndexMethod;

ML_METHODVX("[]", MLTableT, MLStringT) {
//<Table
//<Name
//>array
// Returns the column :mini:`Name` from :mini:`Table`.
	ml_table_t *Table = (ml_table_t *)Args[0];
	ml_value_t *Column = ml_map_search(Table->ColumnNames, Args[1]);
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
	ml_value_t *Column = ml_map_search(Table->ColumnNames, Args[1]);
	if (Column == MLNil) ML_RETURN(Column);
	Args[1] = Column;
	return ml_call(Caller, IndexMethod, Count - 1, Args + 1);
}

static void ML_TYPED_FN(ml_iterate, MLTableT, ml_state_t *Caller, ml_table_t *Table) {
	if (!Table->Size) ML_RETURN(MLNil);
	ML_RETURN(Table->Rows);
}

static void ML_TYPED_FN(ml_iter_next, MLTableRowT, ml_state_t *Caller, ml_table_row_t *Row) {
	++Row;
	if (Row->Type) ML_RETURN(Row);
	ML_RETURN(MLNil);
}

static void ML_TYPED_FN(ml_iter_key, MLTableRowT, ml_state_t *Caller, ml_table_row_t *Row) {
	ML_RETURN(ml_table_row_index(Row));
}

static void ML_TYPED_FN(ml_iter_value, MLTableRowT, ml_state_t *Caller, ml_table_row_t *Row) {
	ML_RETURN(Row);
}

ML_METHOD("[]", MLTableT, MLIntegerT) {
//<Table
//<Row
//>tablerow
// Returns the :mini:`Row`-th row of :mini:`Table`.
	ml_table_t *Table = (ml_table_t *)Args[0];
	size_t Size = Table->Size;
	size_t Index = ml_integer_value(Args[1]) - 1;
	if (Index < 0 || Index >= Size) return MLNil;
	return (ml_value_t *)(Table->Rows + Index);
}

ML_METHOD("[]", MLTableRowT, MLStringT) {
//<Row
//<Name
//>any
// Returns the value from column :mini:`Name` in :mini:`Row`.
	ml_table_row_t *Row = (ml_table_row_t *)Args[0];
	ml_value_t *Column = ml_map_search(Row->Table->ColumnNames, Args[1]);
	if (Column == MLNil) return Column;
	ml_value_t *Indices[1] = {ml_table_row_index(Row)};
	return ml_array_index((ml_array_t *)Column, 1, Indices);
}

ML_METHOD("::", MLTableRowT, MLStringT) {
//<Row
//<Name
//>any
// Returns the value from column :mini:`Name` in :mini:`Row`.
	ml_table_row_t *Row = (ml_table_row_t *)Args[0];
	ml_value_t *Column = ml_map_search(Row->Table->ColumnNames, Args[1]);
	if (Column == MLNil) return Column;
	ml_value_t *Indices[1] = {ml_table_row_index(Row)};
	return ml_array_index((ml_array_t *)Column, 1, Indices);
}

ML_METHOD("append", MLStringBufferT, MLTableRowT) {
//<Buffer
//<Value
// Appends a representation of :mini:`Value` to :mini:`Buffer`.
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_table_row_t *Row = (ml_table_row_t *)Args[1];
	ml_value_t *Indices[1] = {ml_table_row_index(Row)};
	ml_stringbuffer_put(Buffer, '<');
	int Comma = 0;
	ML_MAP_FOREACH(Row->Table->ColumnNames, Iter) {
		if (Comma) ml_stringbuffer_write(Buffer, ", ", 2);
		ml_stringbuffer_simple_append(Buffer, Iter->Key);
		ml_stringbuffer_write(Buffer, " is ", 4);
		ml_value_t *Value = ml_array_index((ml_array_t *)Iter->Value, 1, Indices);
		ml_stringbuffer_simple_append(Buffer, Value);
		Comma = 1;
	}
	ml_stringbuffer_put(Buffer, '>');
	return MLSome;
}

typedef struct {
	ml_type_t *Type;
	ml_map_node_t *Node;
	ml_table_row_t *Row;
	ml_value_t *Indices[1];
} ml_table_row_iter_t;

ML_TYPE(MLTableRowIterT, (), "table-row-iter"
//!internal
);

static void ML_TYPED_FN(ml_iterate, MLTableRowT, ml_state_t *Caller, ml_table_row_t *Row) {
	ml_map_node_t *Node = ((ml_map_t *)Row->Table->ColumnNames)->Head;
	if (!Node) ML_RETURN(MLNil);
	ml_table_row_iter_t *Iter = new(ml_table_row_iter_t);
	Iter->Type = MLTableRowIterT;
	Iter->Row = Row;
	Iter->Node = Node;
	Iter->Indices[0] = ml_table_row_index(Row);
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
	ML_RETURN(ml_array_index((ml_array_t *)Iter->Node->Value, 1, Iter->Indices));
}

static ml_value_t *ML_TYPED_FN(ml_serialize, MLTableT, ml_table_t *Table) {
	ml_value_t *Result = ml_list();
	ml_list_put(Result, ml_cstring("table"));
	ml_list_put(Result, Table->ColumnNames);
	return Result;
}

ML_DESERIALIZER("table") {
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLMapT);
	ml_value_t *Table = ml_table();
	ML_MAP_FOREACH(Args[0], Iter) {
		if (!ml_is(Iter->Key, MLStringT)) return ml_error("SerializationError", "Invalid table");
		if (!ml_is(Iter->Value, MLArrayT)) return ml_error("SerializationError", "Invalid table");
		ml_value_t *Result = ml_table_insert(Table, Iter->Key, Iter->Value);
		if (ml_is_error(Result)) return ml_error("SerializationError", "Invalid table");
	}
	return Table;
}

void ml_table_init(stringmap_t *Globals) {
#include "ml_table_init.c"
	stringmap_insert(Globals, "table", MLTableT);
}
