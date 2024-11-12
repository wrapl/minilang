#include "ml_table.h"
#include "ml_array.h"
#include "ml_macros.h"
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
	stringmap_t Columns[1];
	size_t Capacity, Offset, Length;
};

struct ml_table_row_t {
	ml_type_t *Type;
	ml_table_t *Table;
};

ML_TYPE(MLTableT, (MLSequenceT), "table");
// A table is a set of named arrays. The arrays must have the same length.

extern ml_type_t MLTableRowT[];

static void ml_table_row_assign(ml_state_t *Caller, ml_table_row_t *Row, ml_value_t *Value) {
	ml_table_t *Table = Row->Table;
	ml_value_t *Indices[1] = {ml_integer((Row - Table->Rows) + 1 - Table->Offset)};
	if (ml_is(Value, MLMapT)) {

	} else if (ml_is(Value, MLTableRowT)) {

	}
	ML_RETURN(Value);
}

ML_TYPE(MLTableRowT, (), "table::row",
	.assign = (void *)ml_table_row_assign
);

ml_array_t *ml_table_insert_column(ml_table_t *Table, const char *Name, ml_array_t *Source) {
	// Source->Degree >= 1
	// Source->Dimensions[0].Size == Table->Length
	ml_array_t *Column = ml_array_alloc(Source->Format, Source->Degree);
	size_t RowSize = MLArraySizes[Source->Format];
	for (int I = Source->Degree; --I >= 0;) {
		Column->Dimensions[I].Stride = RowSize;
		size_t Size = Column->Dimensions[I].Size = Source->Dimensions[I].Size;
		RowSize *= Size;
	}
	Column->Base.Length = RowSize;
	RowSize = Column->Dimensions[0].Stride;
	void *Data;
	if (Source->Format == ML_ARRAY_FORMAT_ANY) {
		Data = bnew(RowSize * Table->Capacity);
	} else {
		Data = snew(RowSize * Table->Capacity);
	}
	Column->Base.Value = Data + RowSize * Table->Offset;
	ml_array_copy(Column, Source);
	stringmap_insert(Table->Columns, Name, Column);
	return Column;
}

ML_METHOD(MLTableT) {
	ml_table_t *Table = new(ml_table_t);
	Table->Type = MLTableT;
	ml_table_row_t *Row = Table->Rows = anew(ml_table_row_t, 4);
	for (int I = 4; --I >= 0; ++Row) {
		Row->Type = MLTableRowT;
		Row->Table = Table;
	}
	Table->Length = 0;
	Table->Capacity = 4;
	Table->Offset = 2;
	return (ml_value_t *)Table;
}

ML_METHODV(MLTableT, MLNamesT, MLArrayT) {
	ML_NAMES_CHECK_ARG_COUNT(0);
	if (!ml_names_length(Args[0])) return ml_error("ValueError", "Must pass at least one column");
	if (!ml_array_degree(Args[1])) return ml_error("ShapeError", "Empty arrays cannot be added to table");
	size_t Length = ((ml_array_t *)Args[1])->Dimensions[0].Size;
	ml_table_t *Table = new(ml_table_t);
	Table->Type = MLTableT;
	ml_table_row_t *Row = Table->Rows = anew(ml_table_row_t, Length);
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
		ML_CHECK_ARG_TYPE(I, MLArrayT);
		ml_array_t *Source = (ml_array_t *)Args[I];
		if (!Source->Degree || Source->Dimensions[0].Size != Length) return ml_error("ShapeError", "Arrays must have same length");
		ml_table_insert_column(Table, ml_string_value(Iter->Value), Source);
	}
	return (ml_value_t *)Table;
}

typedef struct {
	size_t Index, Offset, Length, Capacity;
} shift_info_t;

static int ml_table_column_expand(const char *Name, ml_array_t *Column, shift_info_t *Info) {
	size_t Index = Info->Index;
	size_t Offset = Info->Offset;
	size_t Length = Info->Length;
	size_t RowSize = Column->Dimensions[0].Stride;
	void *Data;
	if (Column->Format == ML_ARRAY_FORMAT_ANY) {
		Data = bnew(RowSize * Info->Capacity);
	} else {
		Data = snew(RowSize * Info->Capacity);
	}
	void *Next = mempcpy(Data + Offset * RowSize, Column->Base.Value, (Index - 1) * RowSize);
	if (Column->Format == ML_ARRAY_FORMAT_ANY) {
		ml_value_t **P = (ml_value_t **)Next;
		for (int I = RowSize / sizeof(ml_value_t *); --I >= 0;) *P++ = MLNil;
	} else {
		memset(Next, 0, RowSize);
	}
	Next += RowSize;
	memcpy(Next, Column->Base.Value + (Index - 1) * RowSize, (Length - (Index - 1)) * RowSize);
	Column->Dimensions[0].Size += 1;
	Column->Base.Length += RowSize;
	Column->Base.Value = Data + Offset * RowSize;
	return 0;
}

static int ml_table_column_shift_up(const char *Name, ml_array_t *Column, shift_info_t *Info) {
	size_t Index = Info->Index;
	size_t Length = Info->Length;
	size_t RowSize = Column->Dimensions[0].Stride;
	void *Data = Column->Base.Value;
	memmove(Data + Index * RowSize, Data + (Index - 1) * RowSize, (Length - (Index - 1)) * RowSize);
	if (Column->Format == ML_ARRAY_FORMAT_ANY) {
		ml_value_t **P = (ml_value_t **)(Data + (Index - 1) * RowSize);
		for (int I = RowSize / sizeof(ml_value_t *); --I >= 0;) *P++ = MLNil;
	} else {
		memset(Data + (Index - 1) * RowSize, 0, RowSize);
	}
	Column->Dimensions[0].Size += 1;
	Column->Base.Length += RowSize;
	return 0;
}

static int ml_table_column_shift_down(const char *Name, ml_array_t *Column, shift_info_t *Info) {
	size_t Index = Info->Index;
	size_t RowSize = Column->Dimensions[0].Stride;
	void *Data = Column->Base.Value;
	memmove(Data - RowSize, Data, (Index - 1) * RowSize);
	if (Column->Format == ML_ARRAY_FORMAT_ANY) {
		ml_value_t **P = (ml_value_t **)(Data + (Index - 1) * RowSize);
		for (int I = RowSize / sizeof(ml_value_t *); --I >= 0;) *P++ = MLNil;
	} else {
		memset(Data + (Index - 1) * RowSize, 0, RowSize);
	}
	Column->Dimensions[0].Size += 1;
	Column->Base.Length += RowSize;
	Column->Base.Value -= RowSize;
	return 0;
}

void ml_table_insert_row(ml_table_t *Table, size_t Index) {
	shift_info_t Info[1] = {{Index, Table->Offset, Table->Length, Table->Capacity}};
	if (Info->Length == Info->Capacity) {
		Info->Capacity += (Info->Capacity >> 2) + 4;
		Info->Offset = (Info->Capacity - Info->Length - 1) / 2;
		Table->Capacity = Info->Capacity;
		Table->Offset = Info->Offset;
		stringmap_foreach(Table->Columns, Info, (void *)ml_table_column_expand);
		ml_table_row_t *Row = Table->Rows = anew(ml_table_row_t, Info->Capacity);
		for (int I = Info->Capacity; --I >= 0; ++Row) {
			Row->Type = MLTableRowT;
			Row->Table = Table;
		}
	} else if (!Info->Offset || (Index > Info->Length / 2)) {
		stringmap_foreach(Table->Columns, Info, (void *)ml_table_column_shift_up);
	} else {
		Table->Offset = --Info->Offset;
		stringmap_foreach(Table->Columns, Info, (void *)ml_table_column_shift_down);
	}
	Table->Length = Info->Length + 1;
}

static int ml_table_column_shrink_up(const char *Name, ml_array_t *Column, shift_info_t *Info) {
	size_t Index = Info->Index;
	size_t RowSize = Column->Dimensions[0].Stride;
	void *Data = Column->Base.Value;
	memmove(Data + RowSize, Data, (Index - 1) * RowSize);
	memset(Data, 0, RowSize);
	Column->Dimensions[0].Size -= 1;
	Column->Base.Length -= RowSize;
	Column->Base.Value += RowSize;
	return 0;
}

static int ml_table_column_shrink_down(const char *Name, ml_array_t *Column, shift_info_t *Info) {
	size_t Index = Info->Index;
	size_t Length = Info->Length;
	size_t RowSize = Column->Dimensions[0].Stride;
	void *Data = Column->Base.Value;
	memmove(Data + (Index - 1) * RowSize, Data + Index * RowSize, (Length - Index) * RowSize);
	memset(Data + (Length - 1) * RowSize, 0, RowSize);
	Column->Dimensions[0].Size -= 1;
	Column->Base.Length -= RowSize;
	Column->Base.Value += RowSize;
	return 0;
}

void ml_table_delete_row(ml_table_t *Table, size_t Index) {
	shift_info_t Info[1] = {{Index, Table->Offset, Table->Length, Table->Capacity}};
	if (Info->Index > Info->Length / 2) {
		stringmap_foreach(Table->Columns, Info, (void *)ml_table_column_shrink_down);
	} else {
		Table->Offset = ++Info->Offset;
		stringmap_foreach(Table->Columns, Info, (void *)ml_table_column_shrink_up);
	}
	Table->Length = Info->Length - 1;
}

ML_METHOD("insert", MLTableT, MLStringT, MLArrayT) {
	ml_table_t *Table = (ml_table_t *)Args[0];
	ml_array_t *Source = (ml_array_t *)Args[2];
	if (!Source->Degree) return ml_error("ShapeError", "Arrays must have same length");
	if (!Table->Columns->Size) {
		size_t Length = Source->Dimensions[0].Size;
		ml_table_row_t *Row = Table->Rows = anew(ml_table_row_t, Length);
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

ML_METHODV("insert", MLTableT, MLNamesT, MLArrayT) {
	ML_NAMES_CHECK_ARG_COUNT(1);
	ml_table_t *Table = (ml_table_t *)Args[0];
	int I = 1;
	ML_NAMES_FOREACH(Args[1], Iter) {
		++I;
		ML_CHECK_ARG_TYPE(I, MLArrayT);
		ml_array_t *Source = (ml_array_t *)Args[I];
		if (!Source->Degree) return ml_error("ShapeError", "Arrays must have same length");
		if (!Table->Columns->Size) {
			size_t Length = Source->Dimensions[0].Size;
			ml_table_row_t *Row = Table->Rows = anew(ml_table_row_t, Length);
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
	return (ml_value_t *)stringmap_remove(Table->Columns, ml_string_value(Args[1])) ?: MLNil;
}

ML_METHOD("[]", MLTableT, MLStringT) {
	ml_table_t *Table = (ml_table_t *)Args[0];
	return (ml_value_t *)stringmap_search(Table->Columns, ml_string_value(Args[1])) ?: MLNil;
}

ML_METHOD("[]", MLTableT, MLIntegerT) {
	ml_table_t *Table = (ml_table_t *)Args[0];
	int Index = ml_integer_value(Args[1]);
	if (Index <= 0) Index += Table->Length + 1;
	if (Index <= 0) return MLNil;
	if (Index > Table->Length) return MLNil;
	return (ml_value_t *)(Table->Rows + (Table->Offset + Index - 1));
}

ML_METHOD("[]", MLTableRowT, MLStringT) {
	ml_table_row_t *Row = (ml_table_row_t *)Args[0];
	ml_table_t *Table = Row->Table;
	int Index = (Row - Table->Rows) + 1 - Table->Offset;
	if (Index <= 0 || Index > Table->Length) return MLNil;
	ml_array_t *Column = (ml_array_t *)stringmap_search(Table->Columns, ml_string_value(Args[1]));
	if (!Column) return MLNil;
	ml_value_t *Indices[1] = {ml_integer(Index)};
	return ml_array_index(Column, 1, Indices);
}

ML_METHOD("::", MLTableRowT, MLStringT) {
	ml_table_row_t *Row = (ml_table_row_t *)Args[0];
	ml_table_t *Table = Row->Table;
	int Index = (Row - Table->Rows) + 1 - Table->Offset;
	if (Index <= 0 || Index > Table->Length) return MLNil;
	ml_array_t *Column = (ml_array_t *)stringmap_search(Table->Columns, ml_string_value(Args[1]));
	if (!Column) return MLNil;
	ml_value_t *Indices[1] = {ml_integer(Index)};
	return ml_array_index(Column, 1, Indices);
}

typedef struct {
	ml_stringbuffer_t *Buffer;
	ml_value_t *Indices[1];
	int Comma;
} ml_table_row_append_t;

static int ml_table_row_append_column(const char *Name, ml_array_t *Column, ml_table_row_append_t *Append) {
	if (Append->Comma) ml_stringbuffer_write(Append->Buffer, ", ", 2);
	ml_stringbuffer_write(Append->Buffer, Name, strlen(Name));
	ml_stringbuffer_write(Append->Buffer, " is ", 4);
	ml_value_t *Value = ml_array_index((ml_array_t *)Column, 1, Append->Indices);
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
	int Index = (Row - Table->Rows) + 1 - Table->Offset;
	if (Index <= 0 || Index > Table->Length) return MLNil;
	ml_table_row_append_t Append[1] = {{(ml_stringbuffer_t *)Args[0], {ml_integer(Index)}, 0}};
	ml_stringbuffer_put(Append->Buffer, '<');
	stringmap_foreach(Table->Columns, Append, (void *)ml_table_row_append_column);
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
		ml_array_t *Column = (ml_array_t *)stringmap_search(Table->Columns, ml_string_value(Iter->Value));
		if (!Column) continue;
		ml_value_t *Value = ml_simple_assign(ml_array_index(Column, 1, Indices), Args[I]);
		if (ml_is_error(Value)) return Value;
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
		ml_array_t *Column = (ml_array_t *)stringmap_search(Table->Columns, ml_string_value(Iter->Value));
		if (!Column) continue;
		ml_value_t *Value = ml_simple_assign(ml_array_index(Column, 1, Indices), Args[I]);
		if (ml_is_error(Value)) return Value;
	}
	return (ml_value_t *)Table;
}

void ml_table_init(stringmap_t *Globals) {
#include "ml_table2_init.c"
	stringmap_insert(Globals, "table", MLTableT);
}

