#include "minilang.h"
#include "ml_macros.h"
#include "ml_cbor.h"

typedef struct {
	ml_state_t *Base;
	FILE *File;
	inthash_t Previous[1];
} ml_state_reader_t;

typedef ml_state_t *(*ml_state_read_fn)(ml_state_reader_t *Reader);

static stringmap_t MLStateTypes[1] = {STRINGMAP_INIT};

void ml_state_type_register(const char *Name, ml_state_read_fn *ReadFn) {
	stringmap_insert(MLStateTypes, Name, ReadFn);
}

ml_state_t *ml_state_load(FILE *File) {

}

typedef struct {
	ml_state_t *Base;
	FILE *File;
	inthash_t Previous[1];
} ml_state_writer_t;

ml_value_t *ml_state_write(ml_state_writer_t *Writer, ml_state_t *State) {
	if (!State) {
		uint64_t Value = (uintptr_t)State;
		fwrite(&Value, sizeof(Value), 1, Writer->File);
		fwrite("null", strlen("null"), 1, Writer->File);
		return NULL;
	}
	if (State == Writer->Base) {
		uint64_t Value = (uintptr_t)State;
		fwrite(&Value, sizeof(Value), 1, Writer->File);
		fwrite("base", strlen("base"), 1, Writer->File);
		return NULL;
	}
	if (!State->Type) return ml_error("TypeError", "Unable to save untyped state");
	typeof(ml_state_write) *fn = ml_typed_fn_get(State->Type, ml_state_write);
	if (!fn) return ml_error("TypeError", "Unable to save %s state", State->Type->Name);
	uint64_t Value = (uintptr_t)State;
	fwrite(&Value, sizeof(Value), 1, Writer->File);
	return fn(Writer, State);
}

ml_value_t *ml_state_save(ml_state_t *State, ml_state_t *Base, FILE *File) {
	return NULL;
}
