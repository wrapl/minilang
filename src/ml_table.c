#include "ml_table.h"
#include "ml_array.h"
#include "ml_macros.h"
#include "ml_utils.h"
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

#undef ML_CATEGORY
#define ML_CATEGORY "table"

typedef struct ml_table_t ml_table_t;
typedef struct ml_table_column_t ml_table_column_t;
typedef struct ml_table_row_t ml_table_row_t;

struct ml_table_t {
	ml_type_t *Type;
	ml_table_row_t *Rows;
	ml_table_column_t *Columns;
	stringmap_t ColumnNames[1];
	size_t Capacity, Offset, Length;
};

struct ml_table_column_t {
	ml_type_t *Type;
	ml_table_column_t *Next;
	ml_table_t *Table;
	ml_value_t *Name;
	ml_array_t *Values;
};

struct ml_table_row_t {
	ml_type_t *Type;
	ml_table_t *Table;
};

ML_TYPE(MLTableT, (MLSequenceT), "table");
// A table is a set of named arrays. The arrays must have the same length.



extern ml_type_t MLTableRowT[];

static int ml_table_row_assign_same(const char *Name, ml_table_column_t *Column, int *Indices) {
	ml_array_t *Values = Column->Values;
	int RowSize = Values->Dimensions[0].Stride;
	void *Target = ml_array_data(Values) + Indices[0] * RowSize;
	void *Source = ml_array_data(Values) + Indices[1] * RowSize;
	memcpy(Target, Source, RowSize);
	return 0;
}

typedef struct {
	ml_table_t *Target;
	ml_value_t *Error;
	ml_value_t *TargetIndices[1];
	ml_value_t *SourceIndices[1];
} ml_table_row_assign_t;

static int ml_table_row_assign_other(const char *Name, ml_table_column_t *Source, ml_table_row_assign_t *Assign) {
	ml_table_column_t *Target = stringmap_search(Assign->Target->ColumnNames, Name);
	if (!Target) return 0;
	ml_value_t *Slot = ml_array_index(Target->Values, 1, Assign->TargetIndices);
	ml_value_t *Value = ml_array_index(Source->Values, 1, Assign->SourceIndices);
	ml_value_t *Result = ml_simple_assign(Slot, ml_deref(Value));
	if (ml_is_error(Result)) {
		Assign->Error = Result;
		return 1;
	}
	return 0;
}

static void ml_table_row_assign(ml_state_t *Caller, ml_table_row_t *Row, ml_value_t *Value) {
	ml_table_t *Table = Row->Table;
	int Index = (Row - Table->Rows) + 1;
	if (Index <= 0 || Index > Table->Length) ML_ERROR("ValueError", "Row is no longer valid");
	if (ml_is(Value, MLMapT)) {
		ml_value_t *Indices[1] = {ml_integer(Index)};
		ML_MAP_FOREACH(Value, Iter) {
			if (!ml_is(Iter->Key, MLStringT)) ML_ERROR("TypeError", "Column names must be strings");
			const char *Name = ml_string_value(Iter->Key);
			ml_table_column_t *Column = stringmap_search(Table->ColumnNames, Name);
			if (!Column) ML_ERROR("NameError", "Column %s not in table", Name);
			ml_value_t *Slot = ml_array_index(Column->Values, 1, Indices);
			ml_value_t *Result = ml_simple_assign(Slot, Iter->Value);
			if (ml_is_error(Result)) ML_RETURN(Result);
		}
	} else if (ml_is(Value, MLTupleT)) {
		ml_value_t *Indices[1] = {ml_integer(Index)};
		int Count = ml_tuple_size(Value);
		if (!Count) ML_RETURN(Value);
		ml_value_t *Names = ml_tuple_get(Value, 1);
		if (ml_typeof(Names) == MLNamesT) {
			if (ml_names_length(Names) + 1 != Count) ML_ERROR("ShapeError", "Tuple does not have enough values");
			int I = 1;
			ML_NAMES_FOREACH(Names, Iter) {
				const char *Name = ml_string_value(Iter->Value);
				ml_table_column_t *Column = stringmap_search(Table->ColumnNames, Name);
				if (!Column) ML_ERROR("NameError", "Column %s not in table", Name);
				ml_value_t *Slot = ml_array_index(Column->Values, 1, Indices);
				ml_value_t *Result = ml_simple_assign(Slot, ml_tuple_get(Value, ++I));
				if (ml_is_error(Result)) ML_RETURN(Result);
			}
		} else {
			ml_table_column_t *Column = Table->Columns;
			for (int I = 1; I <= Count; ++I) {
				if (!Column) break;
				ml_value_t *Slot = ml_array_index(Column->Values, 1, Indices);
				ml_value_t *Result = ml_simple_assign(Slot, ml_tuple_get(Value, I));
				if (ml_is_error(Result)) ML_RETURN(Result);
				Column = Column->Next;
			}
		}
	} else if (ml_is(Value, MLListT)) {
		ml_value_t *Indices[1] = {ml_integer(Index)};
		ml_table_column_t *Column = Table->Columns;
		ML_LIST_FOREACH(Value, Iter) {
			if (!Column) break;
			ml_value_t *Slot = ml_array_index(Column->Values, 1, Indices);
			ml_value_t *Result = ml_simple_assign(Slot, Iter->Value);
			if (ml_is_error(Result)) ML_RETURN(Result);
			Column = Column->Next;
		}
	} else if (ml_is(Value, MLTableRowT)) {
		ml_table_row_t *Row2 = (ml_table_row_t *)Value;
		ml_table_t *Table2 = Row2->Table;
		int Index2 = (Row2 - Table2->Rows) + 1;
		if (Index2 <= 0 || Index2 > Table2->Length) ML_ERROR("ValueError", "Row is no longer valid");
		if (Table == Table2) {
			if (Index != Index2) {
				int Indices[2] = {Index - 1, Index2 - 1};
				stringmap_foreach(Table->ColumnNames, Indices, (void *)ml_table_row_assign_same);
			}
		} else {
			ml_table_row_assign_t Assign[1] = {{Table, NULL, {ml_integer(Index)}, {ml_integer(Index2)}}};
			if (stringmap_foreach(Table2->ColumnNames, Assign, (void *)ml_table_row_assign_other)) {
				ML_RETURN(Assign->Error);
			}
		}
	}
	ML_RETURN(Value);
}

ML_TYPE(MLTableRowT, (), "table::row",
	.assign = (void *)ml_table_row_assign
);

static ml_value_t *ml_table_column_deref(ml_table_column_t *Column) {
	return (ml_value_t *)Column->Values ?: MLNil;
}

static void ml_table_column_append(ml_table_t *Table, ml_table_column_t *Column) {
	ml_table_column_t *OldColumn = stringmap_insert(Table->ColumnNames, ml_string_value(Column->Name), Column);
	if (OldColumn) {
		if (OldColumn != Column) {
			ml_table_column_t **Slot = &Table->Columns;
			while (Slot[0] != OldColumn) Slot = &Slot[0]->Next;
			Column->Next = OldColumn->Next;
			Slot[0] = Column;
		}
	} else {
		ml_table_column_t **Slot = &Table->Columns;
		while (Slot[0]) Slot = &Slot[0]->Next;
		Column->Next = NULL;
		Slot[0] = Column;
	}
}

static void ml_table_column_assign(ml_state_t *Caller, ml_table_column_t *Column, ml_value_t *Value) {
	ml_table_t *Table = Column->Table;
	ml_array_t *Source = (ml_array_t *)ml_array_of(Value);
	if (ml_is_error((ml_value_t *)Source)) ML_RETURN(Source);
	if (!Source->Degree) ML_ERROR("ShapeError", "Empty arrays cannot be added to table");
	if (!Table->ColumnNames->Size) {
		size_t Length = Source->Dimensions[0].Size;
		ml_table_row_t *Row = Table->Rows = anew(ml_table_row_t, Length + 1);
		for (int I = Length; --I >= 0; ++Row) {
			Row->Type = MLTableRowT;
			Row->Table = Table;
		}
		Table->Length = Length;
		Table->Capacity = Length;
		Table->Offset = 0;
	} else if (Source->Dimensions[0].Size != Table->Length) {
		ML_ERROR("ShapeError", "Arrays must have same length");
	}
	ml_array_t *Array = ml_array_alloc(Source->Format, Source->Degree);
	size_t RowSize = MLArraySizes[Source->Format];
	for (int I = Source->Degree; --I >= 0;) {
		Array->Dimensions[I].Stride = RowSize;
		size_t Size = Array->Dimensions[I].Size = Source->Dimensions[I].Size;
		RowSize *= Size;
	}
	Array->Base.Length = RowSize;
	RowSize = Array->Dimensions[0].Stride;
	void *Data;
	if (Source->Format == ML_ARRAY_FORMAT_ANY) {
		Data = bnew(RowSize * Table->Capacity);
	} else {
		Data = snew(RowSize * Table->Capacity);
	}
	Array->Base.Value = Data + RowSize * Table->Offset;
	ml_array_copy_data(Source, Array->Base.Value);
	Column->Values = Array;
	ml_table_column_append(Table, Column);
	ML_RETURN(Value);
}

ML_TYPE(MLTableColumnT, (), "table::column",
	.deref = (void *)ml_table_column_deref,
	.assign = (void *)ml_table_column_assign
);

ml_array_t *ml_table_insert_column(ml_table_t *Table, const char *Name, ml_array_t *Source) {
	// Source->Degree >= 1
	// Source->Dimensions[0].Size == Table->Length
	ml_table_column_t *Column = stringmap_search(Table->ColumnNames, Name);
	if (!Column) {
		Column = new(ml_table_column_t);
		Column->Type = MLTableColumnT;
		Column->Table = Table;
		Column->Name = ml_string(Name, -1);
	}
	ml_array_t *Array = ml_array_alloc(Source->Format, Source->Degree);
	size_t RowSize = MLArraySizes[Source->Format];
	for (int I = Source->Degree; --I >= 0;) {
		Array->Dimensions[I].Stride = RowSize;
		size_t Size = Array->Dimensions[I].Size = Source->Dimensions[I].Size;
		RowSize *= Size;
	}
	Array->Base.Length = RowSize;
	RowSize = Array->Dimensions[0].Stride;
	void *Data;
	if (Source->Format == ML_ARRAY_FORMAT_ANY) {
		Data = bnew(RowSize * Table->Capacity);
	} else {
		Data = snew(RowSize * Table->Capacity);
	}
	Array->Base.Value = Data + RowSize * Table->Offset;
	ml_array_copy_data(Source, Array->Base.Value);
	Column->Values = Array;
	ml_table_column_append(Table, Column);
	return Array;
}

ml_array_t *ml_table_insert(ml_value_t *Table, const char *Name, ml_value_t *Value) {
	ml_array_t *Source = (ml_array_t *)(ml_is(Value, MLArrayT) ? Value : ml_array_of(Value));
	return ml_table_insert_column((ml_table_t *)Table, Name, Source);
}

ml_value_t *ml_table() {
	ml_table_t *Table = new(ml_table_t);
	Table->Type = MLTableT;
	Table->Length = 0;
	Table->Capacity = 0;
	Table->Offset = 0;
	return (ml_value_t *)Table;
}

ML_METHOD(MLTableT) {
	return ml_table();
}

ML_METHODV(MLTableT, MLMapT) {
	ml_map_node_t *Node = ((ml_map_t *)Args[0])->Head;
	if (!Node) return ml_table();
	if (!ml_is(Node->Key, MLStringT)) return ml_error("TypeError", "Column name must be a string");
	ml_array_t *Source = (ml_array_t *)(ml_is(Node->Value, MLArrayT) ? Node->Value : ml_array_of(Node->Value));
	if (Source->Base.Type == MLErrorT) return (ml_value_t *)Source;
	if (!Source->Degree) return ml_error("ShapeError", "Empty arrays cannot be added to table");
	size_t Length = Source->Dimensions[0].Size;
	ml_table_t *Table = new(ml_table_t);
	Table->Type = MLTableT;
	ml_table_row_t *Row = Table->Rows = anew(ml_table_row_t, Length + 1);
	for (int I = Length; --I >= 0; ++Row) {
		Row->Type = MLTableRowT;
		Row->Table = Table;
	}
	Table->Length = Length;
	Table->Capacity = Length;
	Table->Offset = 0;
	ml_table_insert_column(Table, ml_string_value(Node->Key), Source);
	while ((Node = Node->Next)) {
		if (!ml_is(Node->Key, MLStringT)) return ml_error("TypeError", "Column name must be a string");
		ml_array_t *Source = (ml_array_t *)(ml_is(Node->Value, MLArrayT) ? Node->Value : ml_array_of(Node->Value));
		if (Source->Base.Type == MLErrorT) return (ml_value_t *)Source;
		if (!Source->Degree || Source->Dimensions[0].Size != Length) return ml_error("ShapeError", "Arrays must have same length");
		ml_table_insert_column(Table, ml_string_value(Node->Key), Source);
	}
	return (ml_value_t *)Table;
}

ML_METHODV(MLTableT, MLNamesT, MLAnyT) {
	ML_NAMES_CHECK_ARG_COUNT(0);
	if (!ml_names_length(Args[0])) return ml_table();
	for (int I = 1; I < Count; ++I) {
		ml_value_t *Source = ml_array_of(Args[I]);
		if (ml_is_error(Source)) return Source;
		Args[I] = Source;
	}
	if (!ml_array_degree(Args[1])) return ml_error("ShapeError", "Empty arrays cannot be added to table");
	size_t Length = ((ml_array_t *)Args[1])->Dimensions[0].Size;
	ml_table_t *Table = new(ml_table_t);
	Table->Type = MLTableT;
	ml_table_row_t *Row = Table->Rows = anew(ml_table_row_t, Length + 1);
	for (int I = Length; --I >= 0; ++Row) {
		Row->Type = MLTableRowT;
		Row->Table = Table;
	}
	Table->Length = Length;
	Table->Capacity = Length;
	Table->Offset = 0;
	int I = 0;
	ML_NAMES_FOREACH(Args[0], Iter) {
		++I;
		ml_array_t *Source = (ml_array_t *)Args[I];
		if (!Source->Degree || Source->Dimensions[0].Size != Length) return ml_error("ShapeError", "Arrays must have same length");
		ml_table_insert_column(Table, ml_string_value(Iter->Value), Source);
	}
	return (ml_value_t *)Table;
}

ML_METHODV(MLTableT, MLNamesT, MLTypeT) {
	ML_NAMES_CHECK_ARG_COUNT(0);
	if (!ml_names_length(Args[0])) return ml_table();
	ml_table_t *Table = new(ml_table_t);
	Table->Type = MLTableT;
	Table->Length = 0;
	Table->Capacity = 0;
	Table->Offset = 0;
	int I = 0;
	ML_NAMES_FOREACH(Args[0], Iter) {
		++I;
		ml_array_format_t Format = ml_array_format((ml_type_t *)Args[I]);
		if (Format == ML_ARRAY_FORMAT_NONE) return ml_error("ValueError", "Invalid array type");
		ml_array_t *Source = ml_array(Format, 1, 4);
		Source->Dimensions[0].Size = 0;
		Source->Base.Length = 0;
		ml_table_insert_column(Table, ml_string_value(Iter->Value), Source);
	}
	return (ml_value_t *)Table;
}

ml_value_t *ml_table_columns(ml_value_t *Table) {
	ml_value_t *Columns = ml_map();
	for (ml_table_column_t *Column = ((ml_table_t *)Table)->Columns; Column; Column = Column->Next) {
		ml_map_insert(Columns, Column->Name, (ml_value_t *)Column->Values);
	}
	return Columns;
}

typedef struct {
	size_t Index, Offset, Length, Capacity;
} shift_info_t;

static int ml_table_column_expand(const char *Name, ml_table_column_t *Column, shift_info_t *Info) {
	size_t Index = Info->Index;
	size_t Offset = Info->Offset;
	size_t Length = Info->Length;
	ml_array_t *Array = Column->Values;
	size_t RowSize = Array->Dimensions[0].Stride;
	void *Data;
	if (Array->Format == ML_ARRAY_FORMAT_ANY) {
		Data = bnew(RowSize * Info->Capacity);
	} else {
		Data = snew(RowSize * Info->Capacity);
	}
	void *Next = mempcpy(Data + Offset * RowSize, Array->Base.Value, (Index - 1) * RowSize);
	if (Array->Format == ML_ARRAY_FORMAT_ANY) {
		ml_value_t **P = (ml_value_t **)Next;
		for (int I = RowSize / sizeof(ml_value_t *); --I >= 0;) *P++ = MLNil;
	} else {
		memset(Next, 0, RowSize);
	}
	Next += RowSize;
	memcpy(Next, Array->Base.Value + (Index - 1) * RowSize, (Length - (Index - 1)) * RowSize);
	Array->Dimensions[0].Size += 1;
	Array->Base.Length += RowSize;
	Array->Base.Value = Data + Offset * RowSize;
	return 0;
}

static int ml_table_column_shift_up(const char *Name, ml_table_column_t *Column, shift_info_t *Info) {
	size_t Index = Info->Index;
	size_t Length = Info->Length;
	ml_array_t *Array = Column->Values;
	size_t RowSize = Array->Dimensions[0].Stride;
	void *Data = Array->Base.Value;
	memmove(Data + Index * RowSize, Data + (Index - 1) * RowSize, (Length - (Index - 1)) * RowSize);
	if (Array->Format == ML_ARRAY_FORMAT_ANY) {
		ml_value_t **P = (ml_value_t **)(Data + (Index - 1) * RowSize);
		for (int I = RowSize / sizeof(ml_value_t *); --I >= 0;) *P++ = MLNil;
	} else {
		memset(Data + (Index - 1) * RowSize, 0, RowSize);
	}
	Array->Dimensions[0].Size += 1;
	Array->Base.Length += RowSize;
	return 0;
}

static int ml_table_column_shift_down(const char *Name, ml_table_column_t *Column, shift_info_t *Info) {
	size_t Index = Info->Index;
	ml_array_t *Array = Column->Values;
	size_t RowSize = Array->Dimensions[0].Stride;
	void *Data = Array->Base.Value;
	memmove(Data - RowSize, Data, (Index - 1) * RowSize);
	if (Array->Format == ML_ARRAY_FORMAT_ANY) {
		ml_value_t **P = (ml_value_t **)(Data + (Index - 2) * RowSize);
		for (int I = RowSize / sizeof(ml_value_t *); --I >= 0;) *P++ = MLNil;
	} else {
		memset(Data + (Index - 2) * RowSize, 0, RowSize);
	}
	Array->Dimensions[0].Size += 1;
	Array->Base.Length += RowSize;
	Array->Base.Value -= RowSize;
	return 0;
}

void ml_table_insert_row(ml_table_t *Table, size_t Index) {
	shift_info_t Info[1] = {{Index, Table->Offset, Table->Length, Table->Capacity}};
	if (Info->Length == Info->Capacity) {
		Info->Capacity += (Info->Capacity >> 2) + 4;
		Info->Offset = (Info->Capacity - Info->Length - 1) / 2;
		Table->Capacity = Info->Capacity;
		Table->Offset = Info->Offset;
		stringmap_foreach(Table->ColumnNames, Info, (void *)ml_table_column_expand);
		ml_table_row_t *Row = Table->Rows = anew(ml_table_row_t, Info->Capacity + 1);
		for (int I = Info->Length; --I >= 0; ++Row) {
			Row->Type = MLTableRowT;
			Row->Table = Table;
		}
	} else if (!Info->Offset || (Index > Info->Length / 2)) {
		stringmap_foreach(Table->ColumnNames, Info, (void *)ml_table_column_shift_up);
	} else {
		Table->Offset = --Info->Offset;
		stringmap_foreach(Table->ColumnNames, Info, (void *)ml_table_column_shift_down);
	}
	Table->Length = Info->Length + 1;
	Table->Rows[Info->Length] = (ml_table_row_t){MLTableRowT, Table};
}

static int ml_table_column_shrink_up(const char *Name, ml_table_column_t *Column, shift_info_t *Info) {
	size_t Index = Info->Index;
	ml_array_t *Array = Column->Values;
	size_t RowSize = Array->Dimensions[0].Stride;
	void *Data = Array->Base.Value;
	memmove(Data + RowSize, Data, (Index - 1) * RowSize);
	memset(Data, 0, RowSize);
	Array->Dimensions[0].Size -= 1;
	Array->Base.Length -= RowSize;
	Array->Base.Value += RowSize;
	return 0;
}

static int ml_table_column_shrink_down(const char *Name, ml_table_column_t *Column, shift_info_t *Info) {
	size_t Index = Info->Index;
	size_t Length = Info->Length;
	ml_array_t *Array = Column->Values;
	size_t RowSize = Array->Dimensions[0].Stride;
	void *Data = Array->Base.Value;
	memmove(Data + (Index - 1) * RowSize, Data + Index * RowSize, (Length - Index) * RowSize);
	memset(Data + (Length - 1) * RowSize, 0, RowSize);
	Array->Dimensions[0].Size -= 1;
	Array->Base.Length -= RowSize;
	Array->Base.Value += RowSize;
	return 0;
}

void ml_table_delete_row(ml_table_t *Table, size_t Index) {
	shift_info_t Info[1] = {{Index, Table->Offset, Table->Length, Table->Capacity}};
	if (Info->Index > Info->Length / 2) {
		stringmap_foreach(Table->ColumnNames, Info, (void *)ml_table_column_shrink_down);
	} else {
		Table->Offset = ++Info->Offset;
		stringmap_foreach(Table->ColumnNames, Info, (void *)ml_table_column_shrink_up);
	}
	Table->Length = Info->Length - 1;
	Table->Rows[Table->Length] = (ml_table_row_t){NULL, NULL};

}

ML_METHOD("length", MLTableT) {
	ml_table_t *Table = (ml_table_t *)Args[0];
	return ml_integer(Table->Length);
}

ML_METHOD("capacity", MLTableT) {
	ml_table_t *Table = (ml_table_t *)Args[0];
	return ml_integer(Table->Capacity);
}

ML_METHOD("offset", MLTableT) {
	ml_table_t *Table = (ml_table_t *)Args[0];
	return ml_integer(Table->Offset);
}

ML_METHOD("columns", MLTableT) {
	return ml_table_columns(Args[0]);
}

ML_METHOD("insert", MLTableT, MLStringT, MLAnyT) {
	ml_table_t *Table = (ml_table_t *)Args[0];
	ml_value_t *Value = Args[2];
	ml_array_t *Source = (ml_array_t *)(ml_is(Value, MLArrayT) ? Value : ml_array_of(Value));
	if (!Source->Degree) return ml_error("ShapeError", "Empty arrays cannot be added to table");
	if (!Table->ColumnNames->Size) {
		size_t Length = Source->Dimensions[0].Size;
		ml_table_row_t *Row = Table->Rows = anew(ml_table_row_t, Length + 1);
		for (int I = Length; --I >= 0; ++Row) {
			Row->Type = MLTableRowT;
			Row->Table = Table;
		}
		Table->Length = Length;
		Table->Capacity = Length;
		Table->Offset = 0;
	} else if (Source->Dimensions[0].Size != Table->Length) {
		return ml_error("ShapeError", "Arrays must have same length");
	}
	ml_table_insert_column(Table, ml_string_value(Args[1]), Source);
	return (ml_value_t *)Table;
}

ML_METHODV("insert", MLTableT, MLNamesT, MLAnyT) {
	ML_NAMES_CHECK_ARG_COUNT(1);
	ml_table_t *Table = (ml_table_t *)Args[0];
	int I = 1;
	ML_NAMES_FOREACH(Args[1], Iter) {
		++I;
		ML_CHECK_ARG_TYPE(I, MLArrayT);
		ml_value_t *Value = Args[I];
		ml_array_t *Source = (ml_array_t *)(ml_is(Value, MLArrayT) ? Value : ml_array_of(Value));
		if (!Source->Degree) return ml_error("ShapeError", "Empty arrays cannot be added to table");
		if (!Table->ColumnNames->Size) {
			size_t Length = Source->Dimensions[0].Size;
			ml_table_row_t *Row = Table->Rows = anew(ml_table_row_t, Length + 1);
			for (int I = Length; --I >= 0; ++Row) {
				Row->Type = MLTableRowT;
				Row->Table = Table;
			}
			Table->Length = Length;
			Table->Capacity = Length;
			Table->Offset = 0;
		} else if (Source->Dimensions[0].Size != Table->Length) {
			return ml_error("ShapeError", "Arrays must have same length");
		}
		ml_table_insert_column(Table, ml_string_value(Iter->Value), Source);
	}
	return (ml_value_t *)Table;
}

ML_METHOD("delete", MLTableT, MLStringT) {
	ml_table_t *Table = (ml_table_t *)Args[0];
	ml_table_column_t *Column = stringmap_remove(Table->ColumnNames, ml_string_value(Args[1]));
	if (!Column) return MLNil;
	ml_table_column_t **Slot = &Table->Columns;
	while (Slot[0] != Column) Slot = &Slot[0]->Next;
	Slot[0] = Column->Next;
	return (ml_value_t *)Column;
}

ML_METHOD("[]", MLTableT, MLStringT) {
	ml_table_t *Table = (ml_table_t *)Args[0];
	ml_table_column_t *Column = stringmap_search(Table->ColumnNames, ml_string_value(Args[1]));
	if (!Column) {
		Column = new(ml_table_column_t);
		Column->Type = MLTableColumnT;
		Column->Table = Table;
		Column->Name = Args[1];
	}
	return (ml_value_t *)Column;
}

ML_METHOD("::", MLTableT, MLStringT) {
	ml_table_t *Table = (ml_table_t *)Args[0];
	ml_table_column_t *Column = stringmap_search(Table->ColumnNames, ml_string_value(Args[1]));
	if (!Column) {
		Column = new(ml_table_column_t);
		Column->Type = MLTableColumnT;
		Column->Table = Table;
		Column->Name = Args[1];
	}
	return (ml_value_t *)Column;
}

extern ml_value_t *AppendMethod;

ML_METHOD(AppendMethod, MLStringBufferT, MLTableT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_table_t *Table = (ml_table_t *)Args[1];
	for (ml_table_column_t *Column = ((ml_table_t *)Table)->Columns; Column; Column = Column->Next) {
		ml_stringbuffer_write(Buffer, ml_string_value(Column->Name), ml_string_length(Column->Name));
		ml_stringbuffer_write(Buffer, ": ", strlen(": "));
		ml_stringbuffer_simple_append(Buffer, (ml_value_t *)Column->Values);
		ml_stringbuffer_write(Buffer, "\n", strlen("\n"));
	}
	return MLSome;
}

static void ML_TYPED_FN(ml_iterate, MLTableT, ml_state_t *Caller, ml_table_t *Table) {
	if (!Table->Length) ML_RETURN(MLNil);
	ML_RETURN(Table->Rows);
}

static void ML_TYPED_FN(ml_iter_next, MLTableRowT, ml_state_t *Caller, ml_table_row_t *Row) {
	++Row;
	if (Row->Type) ML_RETURN(Row);
	ML_RETURN(MLNil);
}

static void ML_TYPED_FN(ml_iter_key, MLTableRowT, ml_state_t *Caller, ml_table_row_t *Row) {
	ml_table_t *Table = Row->Table;
	int Index = (Row - Table->Rows) + 1;
	ML_RETURN(ml_integer(Index));
}

static void ML_TYPED_FN(ml_iter_value, MLTableRowT, ml_state_t *Caller, ml_table_row_t *Row) {
	ML_RETURN(Row);
}

ML_METHOD("[]", MLTableT, MLIntegerT) {
	ml_table_t *Table = (ml_table_t *)Args[0];
	int Index = ml_integer_value(Args[1]);
	if (Index <= 0) Index += Table->Length + 1;
	if (Index <= 0) return MLNil;
	if (Index > Table->Length) return MLNil;
	return (ml_value_t *)(Table->Rows + Index - 1);
}

ML_METHOD("[]", MLTableRowT, MLStringT) {
	ml_table_row_t *Row = (ml_table_row_t *)Args[0];
	ml_table_t *Table = Row->Table;
	int Index = (Row - Table->Rows) + 1;
	if (Index <= 0 || Index > Table->Length) return ml_error("ValueError", "Row is no longer valid");
	ml_table_column_t *Column = stringmap_search(Table->ColumnNames, ml_string_value(Args[1]));
	if (!Column) return MLNil;
	ml_value_t *Indices[1] = {ml_integer(Index)};
	return ml_array_index(Column->Values, 1, Indices);
}

ML_METHOD("::", MLTableRowT, MLStringT) {
	ml_table_row_t *Row = (ml_table_row_t *)Args[0];
	ml_table_t *Table = Row->Table;
	int Index = (Row - Table->Rows) + 1;
	if (Index <= 0 || Index > Table->Length) return ml_error("ValueError", "Row is no longer valid");
	ml_table_column_t *Column = stringmap_search(Table->ColumnNames, ml_string_value(Args[1]));
	if (!Column) return MLNil;
	ml_value_t *Indices[1] = {ml_integer(Index)};
	return ml_array_index(Column->Values, 1, Indices);
}

typedef struct {
	ml_stringbuffer_t *Buffer;
	ml_value_t *Indices[1];
	int Comma;
} ml_table_row_append_t;

static int ml_table_row_append_column(const char *Name, ml_table_column_t *Column, ml_table_row_append_t *Append) {
	if (Append->Comma) ml_stringbuffer_write(Append->Buffer, ", ", 2);
	ml_stringbuffer_write(Append->Buffer, Name, strlen(Name));
	ml_stringbuffer_write(Append->Buffer, " is ", 4);
	ml_value_t *Value = ml_array_index(Column->Values, 1, Append->Indices);
	ml_stringbuffer_simple_append(Append->Buffer, Value);
	Append->Comma = 1;
	return 0;
}

ML_METHOD("append", MLStringBufferT, MLTableRowT) {
//<Buffer
//<Value
// Appends a representation of :mini:`Value` to :mini:`Buffer`.
	ml_table_row_t *Row = (ml_table_row_t *)Args[1];
	ml_table_t *Table = Row->Table;
	int Index = (Row - Table->Rows) + 1;
	if (Index <= 0 || Index > Table->Length) return ml_error("ValueError", "Row is no longer valid");
	ml_table_row_append_t Append[1] = {{(ml_stringbuffer_t *)Args[0], {ml_integer(Index)}, 0}};
	ml_stringbuffer_put(Append->Buffer, '<');
	stringmap_foreach(Table->ColumnNames, Append, (void *)ml_table_row_append_column);
	ml_stringbuffer_put(Append->Buffer, '>');
	return MLSome;
}

ML_METHODV("push", MLTableT, MLNamesT) {
	ML_NAMES_CHECK_ARG_COUNT(1);
	ml_table_t *Table = (ml_table_t *)Args[0];
	int Index = 1;
	ml_table_insert_row(Table, Index);
	ml_value_t *Indices[1] = {ml_integer(Index)};
	int I = 1;
	ML_NAMES_FOREACH(Args[1], Iter) {
		++I;
		ml_table_column_t *Column = stringmap_search(Table->ColumnNames, ml_string_value(Iter->Value));
		if (!Column) continue;
		ml_value_t *Value = ml_simple_assign(ml_array_index(Column->Values, 1, Indices), Args[I]);
		if (ml_is_error(Value)) return Value;
	}
	return (ml_value_t *)Table;
}

ML_METHOD("push", MLTableT, MLListT) {
	ml_table_t *Table = (ml_table_t *)Args[0];
	int Index = 1;
	ml_table_insert_row(Table, Index);
	ml_value_t *Indices[1] = {ml_integer(Index)};
	ml_table_column_t *Column = Table->Columns;
	ML_LIST_FOREACH(Args[1], Iter) {
		if (!Column) continue;
		ml_value_t *Value = ml_simple_assign(ml_array_index(Column->Values, 1, Indices), Iter->Value);
		if (ml_is_error(Value)) return Value;
		Column = Column->Next;
	}
	return (ml_value_t *)Table;
}

ML_METHODV("put", MLTableT, MLNamesT) {
	ML_NAMES_CHECK_ARG_COUNT(1);
	ml_table_t *Table = (ml_table_t *)Args[0];
	int Index = Table->Length + 1;
	ml_table_insert_row(Table, Index);
	ml_value_t *Indices[1] = {ml_integer(Index)};
	int I = 1;
	ML_NAMES_FOREACH(Args[1], Iter) {
		++I;
		ml_table_column_t *Column = stringmap_search(Table->ColumnNames, ml_string_value(Iter->Value));
		if (!Column) continue;
		ml_value_t *Value = ml_simple_assign(ml_array_index(Column->Values, 1, Indices), Args[I]);
		if (ml_is_error(Value)) return Value;
	}
	return (ml_value_t *)Table;
}

ML_METHOD("put", MLTableT, MLListT) {
	ml_table_t *Table = (ml_table_t *)Args[0];
	int Index = Table->Length + 1;
	ml_table_insert_row(Table, Index);
	ml_value_t *Indices[1] = {ml_integer(Index)};
	ml_table_column_t *Column = Table->Columns;
	ML_LIST_FOREACH(Args[1], Iter) {
		if (!Column) continue;
		ml_value_t *Value = ml_simple_assign(ml_array_index(Column->Values, 1, Indices), Iter->Value);
		if (ml_is_error(Value)) return Value;
		Column = Column->Next;
	}
	return (ml_value_t *)Table;
}

ML_METHODV("insert", MLTableT, MLIntegerT, MLNamesT) {
	ML_NAMES_CHECK_ARG_COUNT(2);
	ml_table_t *Table = (ml_table_t *)Args[0];
	int Index = ml_integer_value(Args[1]);
	if (Index <= 0) Index += Table->Length + 1;
	if (Index <= 0 || Index > Table->Length + 1) return MLNil;
	ml_table_insert_row(Table, Index);
	ml_value_t *Indices[1] = {ml_integer(Index)};
	int I = 2;
	ML_NAMES_FOREACH(Args[2], Iter) {
		++I;
		ml_table_column_t *Column = stringmap_search(Table->ColumnNames, ml_string_value(Iter->Value));
		if (!Column) continue;
		ml_value_t *Value = ml_simple_assign(ml_array_index(Column->Values, 1, Indices), Args[I]);
		if (ml_is_error(Value)) return Value;
	}
	return (ml_value_t *)Table;
}

ML_METHOD("insert", MLTableT, MLIntegerT, MLListT) {
	ml_table_t *Table = (ml_table_t *)Args[0];
	int Index = ml_integer_value(Args[1]);
	if (Index <= 0) Index += Table->Length + 1;
	if (Index <= 0 || Index > Table->Length + 1) return MLNil;
	ml_table_insert_row(Table, Index);
	ml_value_t *Indices[1] = {ml_integer(Index)};
	ml_table_column_t *Column = Table->Columns;
	ML_LIST_FOREACH(Args[2], Iter) {
		if (!Column) continue;
		ml_value_t *Value = ml_simple_assign(ml_array_index(Column->Values, 1, Indices), Iter->Value);
		if (ml_is_error(Value)) return Value;
		Column = Column->Next;
	}
	return (ml_value_t *)Table;
}

typedef struct {
	ml_state_t Base;
	ml_table_t *Table;
	ml_table_row_t *Rows;
	ml_value_t *Compare;
	int32_t *Source, *Dest;
	int32_t *IndexA, *LimitA, *IndexB, *LimitB;
	int32_t *Target, *Limit;
	ml_value_t *Args[2];
	size_t Length, BlockSize;
} ml_table_sort_state_t;

static void ml_table_sort_run(ml_table_sort_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	int32_t *Target = State->Target;
	if (Value != MLNil) {
		int32_t *Index = State->IndexA;
		*Target++ = *Index++;
		if (Index < State->LimitA) {
			State->Target = Target;
			State->IndexA = Index;
			State->Args[0] = (ml_value_t *)(State->Rows + *Index);
			State->Args[1] = (ml_value_t *)(State->Rows + *State->IndexB);
			return ml_call(State, State->Compare, 2, State->Args);
		}
		Target = mempcpy(Target, State->IndexB, (State->LimitB - State->IndexB) * sizeof(int32_t));
	} else {
		int32_t *Index = State->IndexB;
		*Target++ = *Index++;
		if (Index < State->LimitB) {
			State->Target = Target;
			State->IndexB = Index;
			State->Args[0] = (ml_value_t *)(State->Rows + *State->IndexA);
			State->Args[1] = (ml_value_t *)(State->Rows + *Index);
			return ml_call(State, State->Compare, 2, State->Args);
		}
		Target = mempcpy(Target, State->IndexA, (State->LimitA - State->IndexA) * sizeof(int32_t));
	}
	size_t Remaining = State->Limit - Target;
	size_t BlockSize = State->BlockSize;
	int32_t *IndexA = State->LimitB;
	if (Remaining <= BlockSize) {
		memcpy(Target, State->LimitB, Remaining * sizeof(ml_slice_node_t));
		BlockSize *= 2;
		Remaining = State->Length;
		if (Remaining <= BlockSize) {
			int32_t *Source = State->Source;
			int32_t *Dest = State->Dest;
			for (int I = 0; I < Remaining; ++I) Source[Dest[I]] = I;
			for (ml_table_column_t *Column = State->Table->Columns; Column; Column = Column->Next) {
				memcpy(Dest, Source, Remaining * sizeof(int32_t));
				ml_array_reorder(Column->Values, Dest, Remaining);
			}
			ML_CONTINUE(State->Base.Caller, State->Table);
		}
		State->BlockSize = BlockSize;
		int32_t *Temp = State->Source;
		IndexA = State->Source = State->Dest;
		Target = State->Dest = Temp;
		State->Limit = Target + State->Length;
	}
	State->Target = Target;
	State->IndexA = IndexA;
	int32_t *IndexB = IndexA + BlockSize;
	State->LimitA = State->IndexB = IndexB;
	Remaining -= BlockSize;
	State->LimitB = IndexB + (Remaining < BlockSize ? Remaining : BlockSize);
	State->Args[0] = (ml_value_t *)(State->Rows + *IndexA);
	State->Args[1] = (ml_value_t *)(State->Rows + *IndexB);
	return ml_call(State, State->Compare, 2, State->Args);
}

ML_METHODX("sort", MLTableT, MLFunctionT) {
	ml_table_t *Table = (ml_table_t *)Args[0];
	size_t Length = Table->Length;
	if (Length < 2) ML_RETURN(Table);
	ml_table_sort_state_t *State = new(ml_table_sort_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_table_sort_run;
	State->Table = Table;
	State->Compare = Args[1];
	int32_t *Source = State->Source = asnew(int32_t, Length);
	int32_t *Dest = State->Dest = asnew(int32_t, Length);
	for (int I = 0; I < Length; ++I) Source[I] = I;
	State->Rows = Table->Rows;
	State->IndexA = Source;
	State->IndexB = State->LimitA = Source + 1;
	State->LimitB = Source + 2;
	State->Target = Dest;
	State->Limit = Dest + Length;
	State->Length = Length;
	State->BlockSize = 1;
	State->Args[0] = (ml_value_t *)(Table->Rows + 0);
	State->Args[1] = (ml_value_t *)(Table->Rows + 1);
	return ml_call(State, State->Compare, 2, State->Args);
}

#ifdef ML_MATH

ML_METHOD("permute", MLTableT, MLPermutationT) {
	ml_table_t *Table = (ml_table_t *)Args[0];
	ml_array_t *Permutation = (ml_array_t *)Args[1];
	int Length = Table->Length;
	if (Permutation->Dimensions[0].Size != Length) {
		return ml_error("ShapeError", "Permutation length does not match list");
	}
	int32_t *Dest = (int32_t *)Permutation->Base.Value;
	int32_t *Order = alloca(Length * sizeof(int32_t));
	for (ml_table_column_t *Column = Table->Columns; Column; Column = Column->Next) {
		for (int32_t I = 0; I < Length; ++I) Order[Dest[I] - 1] = I;
		ml_array_reorder(Column->Values, Order, Length);
	}
	return (ml_value_t *)Table;
}

#endif

static ml_value_t *ML_TYPED_FN(ml_serialize, MLTableT, ml_table_t *Table) {
	ml_value_t *Result = ml_list();
	ml_list_put(Result, ml_cstring("table"));
	ml_list_put(Result, ml_table_columns((ml_value_t *)Table));
	return Result;
}

ML_DESERIALIZER("table") {
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLMapT);
	ml_table_t *Table = (ml_table_t *)ml_table();
	ML_MAP_FOREACH(Args[0], Iter) {
		if (!ml_is(Iter->Key, MLStringT)) return ml_error("SerializationError", "Invalid table");
		ml_array_t *Source = (ml_array_t *)ml_array_of(Iter->Value);
		if (Source->Base.Type == MLErrorT) return (ml_value_t *)Source;
		ml_value_t *Result = (ml_value_t *)ml_table_insert_column(Table, ml_string_value(Iter->Key), Source);
		if (ml_is_error(Result)) return ml_error("SerializationError", "Invalid table");
	}
	ml_table_column_t *Column = Table->Columns;
	if (Column) {
		size_t Length = Column->Values->Dimensions[0].Size;
		ml_table_row_t *Row = Table->Rows = anew(ml_table_row_t, Length + 1);
		for (int I = Length; --I >= 0; ++Row) {
			Row->Type = MLTableRowT;
			Row->Table = Table;
		}
		Table->Length = Length;
		Table->Capacity = Length;
		Table->Offset = 0;
		while ((Column = Column->Next)) if (Column->Values->Dimensions[0].Size != Length) {
			return ml_error("SerializationError", "Table columns do not have same length");
		}
	}
	return (ml_value_t *)Table;
}

void ml_table_init(stringmap_t *Globals) {
#include "ml_table_init.c"
	stringmap_insert(Globals, "table", MLTableT);
}

