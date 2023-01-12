#ifndef ML_BYTECODE_H
#define ML_BYTECODE_H

#include "ml_types.h"
#include "ml_runtime.h"
#include "ml_opcodes.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ml_closure_t ml_closure_t;
typedef struct ml_closure_info_t ml_closure_info_t;

extern ml_type_t MLClosureT[];
extern ml_type_t MLClosureInfoT[];

typedef union ml_inst_t ml_inst_t;

union ml_inst_t {
	struct {
		ml_opcode_t Opcode:8;
		unsigned int Reserved:8;
		unsigned int Label:16;
		unsigned int Line:32;
	};
	ml_inst_t *Inst;
	ml_inst_t **Insts;
	int Count;
	ml_value_t *Value;
	ml_closure_info_t *ClosureInfo;
	ml_decl_t *Decls;
	const char *Chars;
	const char **Ptrs;
	void *Data;
};

#define SHA256_BLOCK_SIZE 32

#define ML_CLOSURE_EXTRA_ARGS 1
#define ML_CLOSURE_NAMED_ARGS 2
#define ML_CLOSURE_LABELLED 4
#define ML_CLOSURE_HASHED 8

struct ml_closure_info_t {
	ml_type_t *Type;
	ml_inst_t *Entry, *Return, *Halt;
	const char *Name, *Source;
	ml_decl_t *Decls;
#ifdef ML_JIT
	void *JITStart, *JITEntry, *JITReturn;
#endif
	stringmap_t Params[1];
	int StartLine, EndLine, FrameSize;
	int NumParams, NumUpValues;
	int Flags;
	unsigned char Hash[SHA256_BLOCK_SIZE];
};

typedef struct ml_param_type_t ml_param_type_t;

struct ml_param_type_t {
	ml_param_type_t *Next;
	ml_type_t *Type;
	int Index, HasDefault;
};

ml_value_t *ml_closure(ml_closure_info_t *Info);

struct ml_closure_t {
	const ml_type_t *Type;
	const char *Name;
	ml_closure_info_t *Info;
	ml_param_type_t *ParamTypes;
	ml_value_t *UpValues[];
};

static inline stringmap_t *ml_closure_params(ml_value_t *Closure) {
	return ((ml_closure_t *)Closure)->Info->Params;
}

typedef struct ml_frame_t ml_frame_t;

ml_value_t *ml_variable(ml_value_t *Value, ml_type_t *Type);

extern ml_type_t MLVariableT[];

const char *ml_closure_debug(ml_value_t *Value);
void ml_closure_sha256(ml_value_t *Closure, unsigned char Hash[SHA256_BLOCK_SIZE]);

void ml_closure_info_labels(ml_closure_info_t *Info);

void ml_closure_list(ml_value_t *Closure);

#ifdef ML_CBOR_BYTECODE

#include "ml_cbor.h"

void ml_cbor_write_closure(ml_closure_t *Closure, ml_stringbuffer_t *Buffer);

ml_value_t *ml_cbor_read_closure(void *Data, int Count, ml_value_t **Args);

#endif

size_t ml_count_cached_frames();

void ml_bytecode_init();

#ifdef __cplusplus
}
#endif

#endif
