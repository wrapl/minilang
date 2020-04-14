#include "../minilang.h"
#include "../ml_macros.h"
#include "../ml_library.h"
#include "../ml_cbor.h"
#include "radb/radb.h"
#include <libgen.h>
#include <stdio.h>

typedef struct ml_string_store_t {
	const ml_type_t *Type;
	string_store_t *Handle;
} ml_string_store_t;

typedef struct ml_string_store_writer_t {
	const ml_type_t *Type;
	string_store_writer_t Handle[1];
} ml_string_store_writer_t;

typedef struct ml_string_store_reader_t {
	const ml_type_t *Type;
	string_store_reader_t Handle[1];
} ml_string_store_reader_t;

static ml_type_t *StringStoreT;
static ml_type_t *StringStoreWriterT;
static ml_type_t *StringStoreReaderT;

static ml_value_t *ml_string_store_open(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	ml_string_store_t *Store = new(ml_string_store_t);
	Store->Type = StringStoreT;
	Store->Handle = string_store_open(ml_string_value(Args[0]));
	if (!Store->Handle) return ml_error("StoreError", "Error opening string store");
	return Store;
}

static ml_value_t *ml_string_store_create(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(2);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	ML_CHECK_ARG_TYPE(1, MLIntegerT);
	size_t ChunkSize = 0;
	if (Count > 2) {
		ML_CHECK_ARG_TYPE(2, MLIntegerT);
		ChunkSize = ml_integer_value(Args[2]);
	}
	ml_string_store_t *Store = new(ml_string_store_t);
	Store->Type = StringStoreT;
	Store->Handle = string_store_create(ml_string_value(Args[0]), ml_integer_value(Args[1]), ChunkSize);
	return Store;
}

ML_METHOD("get", StringStoreT, MLIntegerT) {
	ml_string_store_t *Store = (ml_string_store_t *)Args[0];
	size_t Index = ml_integer_value(Args[1]);
	size_t Length = string_store_get_size(Store->Handle, Index);
	if (Length == INVALID_INDEX) return ml_error("IndexError", "Invalid index");
	char *Value = snew(Length);
	string_store_get_value(Store->Handle, Index, Value);
	Value[Length] = 0;
	return ml_string(Value, Length);
}

ML_METHOD("set", StringStoreT, MLIntegerT, MLStringT) {
	ml_string_store_t *Store = (ml_string_store_t *)Args[0];
	size_t Index = ml_integer_value(Args[1]);
	string_store_set(Store->Handle, Index, ml_string_value(Args[2]), ml_string_length(Args[2]));
	return Args[2];
}

ML_METHOD("write", StringStoreT, MLIntegerT) {
	ml_string_store_t *Store = (ml_string_store_t *)Args[0];
	ml_string_store_writer_t *Writer = new(ml_string_store_writer_t);
	Writer->Type = StringStoreWriterT;
	string_store_writer_open(Writer->Handle, Store->Handle, ml_integer_value(Args[1]));
	return Writer;
}

ML_METHOD("write", StringStoreWriterT, MLStringT) {
	ml_string_store_writer_t *Writer = (ml_string_store_writer_t *)Args[0];
	size_t Total = string_store_writer_write(Writer->Handle, ml_string_value(Args[1]), ml_string_length(Args[1]));
	return ml_integer(Total);
}

ML_METHOD("read", StringStoreT, MLIntegerT) {
	ml_string_store_t *Store = (ml_string_store_t *)Args[0];
	ml_string_store_reader_t *Reader = new(ml_string_store_reader_t);
	Reader->Type = StringStoreReaderT;
	string_store_reader_open(Reader->Handle, Store->Handle, ml_integer_value(Args[1]));
	return Reader;
}

ML_METHOD("read", StringStoreReaderT, MLIntegerT) {
	ml_string_store_reader_t *Reader = (ml_string_store_reader_t *)Args[0];
	size_t Size = ml_integer_value(Args[1]);
	char *Buffer = GC_MALLOC_ATOMIC(Size);
	size_t Length = string_store_reader_read(Reader->Handle, Buffer, Size);
	return ml_string(Buffer, Length);
}

static ml_type_t *CborStoreT;

static ml_value_t *ml_cbor_store_open(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	ml_string_store_t *Store = new(ml_string_store_t);
	Store->Type = CborStoreT;
	Store->Handle = string_store_open(ml_string_value(Args[0]));
	if (!Store->Handle) return ml_error("StoreError", "Error opening string store");
	return Store;
}

static ml_value_t *ml_cbor_store_create(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(2);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	ML_CHECK_ARG_TYPE(1, MLIntegerT);
	size_t ChunkSize = 0;
	if (Count > 2) {
		ML_CHECK_ARG_TYPE(2, MLIntegerT);
		ChunkSize = ml_integer_value(Args[2]);
	}
	ml_string_store_t *Store = new(ml_string_store_t);
	Store->Type = CborStoreT;
	Store->Handle = string_store_create(ml_string_value(Args[0]), ml_integer_value(Args[1]), ChunkSize);
	return Store;
}

static ml_value_t *ml_value_fn(ml_value_t *Callback, ml_value_t *Value) {
	return ml_inline(Callback, 1, Value);
}

static ml_tag_t ml_value_tag_fn(uint64_t Tag, ml_value_t *Callback, void **Data) {
	Data[0] = ml_inline(Callback, 1, ml_integer(Tag));
	return ml_value_fn;
}

ML_METHOD("get", CborStoreT, MLIntegerT) {
	ml_string_store_t *Store = (ml_string_store_t *)Args[0];
	size_t Index = ml_integer_value(Args[1]);
	size_t Length = string_store_get_size(Store->Handle, Index);
	if (Length == INVALID_INDEX) return ml_error("IndexError", "Invalid index");
	ml_cbor_reader_t *Cbor = ml_cbor_reader_new(Count > 2 ? Args[2] : MLNil, ml_value_tag_fn);
	string_store_reader_t Reader[1];
	string_store_reader_open(Reader, Store->Handle, Index);
	char Buffer[16];
	size_t Size;
	do {
		Size = string_store_reader_read(Reader, Buffer, 16);
		ml_cbor_reader_read(Cbor, Buffer, Size);
	} while (Size == 16);
	return ml_cbor_reader_get(Cbor);
}

ML_METHOD("set", CborStoreT, MLIntegerT, MLAnyT) {
	ml_string_store_t *Store = (ml_string_store_t *)Args[0];
	size_t Index = ml_integer_value(Args[1]);
	string_store_writer_t Writer[1];
	string_store_writer_open(Writer, Store->Handle, Index);
	ml_cbor_write(Args[2], Writer, string_store_writer_write);
	return Args[2];
}

typedef struct ml_string_index_t {
	const ml_type_t *Type;
	string_index_t *Handle;
} ml_string_index_t;

static ml_type_t *StringIndexT;

static ml_value_t *ml_string_index_open(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	ml_string_index_t *Store = new(ml_string_index_t);
	Store->Type = StringIndexT;
	Store->Handle = string_index_open(ml_string_value(Args[0]));
	if (!Store->Handle) return ml_error("StoreError", "Error opening string store");
	return Store;
}

static ml_value_t *ml_string_index_create(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	size_t ChunkSize = 0;
	if (Count > 1) {
		ML_CHECK_ARG_TYPE(2, MLIntegerT);
		ChunkSize = ml_integer_value(Args[1]);
	}
	ml_string_index_t *Store = new(ml_string_index_t);
	Store->Type = StringIndexT;
	Store->Handle = string_index_create(ml_string_value(Args[0]), ChunkSize);
	return Store;
}

ML_METHOD("insert", StringIndexT, MLStringT) {
	ml_string_index_t *Store = (ml_string_index_t *)Args[0];
	size_t Index = string_index_insert(Store->Handle, ml_string_value(Args[1]));
	return ml_integer(Index);
}

ML_METHOD("search", StringIndexT, MLStringT) {
	ml_string_index_t *Store = (ml_string_index_t *)Args[0];
	size_t Index = string_index_search(Store->Handle, ml_string_value(Args[1]));
	return ml_integer(Index);
}

void ml_library_entry(ml_value_t *Module, ml_getter_t GlobalGet, void *Globals) {
	const char *Dir = dirname(GC_strdup(ml_module_path(Module)));
	ml_value_t *Import = GlobalGet(Globals, "import");
	ml_inline(Import, 1, ml_string_format("%s/ml_cbor.so", Dir));

	StringStoreT = ml_type(MLAnyT, "string-store");
	StringStoreWriterT = ml_type(MLAnyT, "string-store-writer");
	StringStoreReaderT = ml_type(MLAnyT, "string-store-reader");
	ml_module_export(Module, "string_store_open", ml_function(0, ml_string_store_open));
	ml_module_export(Module, "string_store_create", ml_function(0, ml_string_store_create));

	CborStoreT = ml_type(MLAnyT, "cbor-store");
	ml_module_export(Module, "cbor_store_open", ml_function(0, ml_cbor_store_open));
	ml_module_export(Module, "cbor_store_create", ml_function(0, ml_cbor_store_create));

	StringIndexT = ml_type(MLAnyT, "string-index");
	ml_module_export(Module, "string_index_open", ml_function(0, ml_string_index_open));
	ml_module_export(Module, "string_index_create", ml_function(0, ml_string_index_create));

#include "ml_radb_init.c"
}
