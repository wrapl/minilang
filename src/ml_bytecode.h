#ifndef ML_BYTECODE_H
#define ML_BYTECODE_H

#include "ml_types.h"
#include "ml_runtime.h"

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct ml_closure_t ml_closure_t;
typedef struct ml_closure_info_t ml_closure_info_t;
typedef struct ml_inst_t ml_inst_t;

extern ml_type_t MLClosureT[];

#define ML_PARAM_DEFAULT 0
#define ML_PARAM_EXTRA 1
#define ML_PARAM_NAMED 2

#define SHA256_BLOCK_SIZE 32

struct ml_closure_info_t {
	ml_inst_t *Entry, *Return;
	const char *Source;
	ml_decl_t *Decls;
#ifdef ML_JIT
	void *JITStart, *JITEntry, *JITReturn;
#endif
	stringmap_t Params[1];
	int LineNo, End, FrameSize;
	int NumParams, NumUpValues;
	int ExtraArgs, NamedArgs;
	int Hashed;
	unsigned char Hash[SHA256_BLOCK_SIZE];
};

typedef struct ml_param_type_t ml_param_type_t;

struct ml_param_type_t {
	ml_param_type_t *Next;
	const ml_type_t *Type;
	int Index;
};

struct ml_closure_t {
	const ml_type_t *Type;
	ml_closure_info_t *Info;
	ml_param_type_t *ParamTypes;
	ml_value_t *UpValues[];
};

static inline stringmap_t *ml_closure_params(ml_value_t *Closure) {
	return ((ml_closure_t *)Closure)->Info->Params;
}

typedef union {
	ml_inst_t *Inst;
	int Index;
	int Count;
	ml_value_t *Value;
	ml_closure_info_t *ClosureInfo;
	ml_decl_t *Decls;
	const char *Ptr;
} ml_param_t;

typedef enum {
	MLI_RETURN,
	MLI_SUSPEND,
	MLI_RESUME,
	MLI_NIL,
	MLI_NIL_PUSH,
	MLI_SOME,
	MLI_AND,
	MLI_OR,
	MLI_PUSH,
	MLI_WITH,
	MLI_WITH_VAR,
	MLI_WITHX,
	MLI_POP,
	MLI_ENTER,
	MLI_EXIT,
	MLI_LOOP,
	MLI_TRY,
	MLI_CATCH,
	MLI_LOAD,
	MLI_LOAD_PUSH,
	MLI_VAR,
	MLI_VAR_TYPE,
	MLI_VARX,
	MLI_LET,
	MLI_LETI,
	MLI_LETX,
	MLI_REF,
	MLI_REFI,
	MLI_REFX,
	MLI_FOR,
	MLI_IF,
	MLI_NEXT,
	MLI_VALUE,
	MLI_KEY,
	MLI_CALL,
	MLI_CONST_CALL,
	MLI_ASSIGN,
	MLI_LOCAL,
	MLI_LOCAL_PUSH,
	MLI_UPVALUE,
	MLI_LOCALX,
	MLI_TUPLE_NEW,
	MLI_TUPLE_SET,
	MLI_LIST_NEW,
	MLI_LIST_APPEND,
	MLI_MAP_NEW,
	MLI_MAP_INSERT,
	MLI_CLOSURE,
#ifdef ML_GENERICS
	MLI_CLOSURE_TYPED,
#endif
	MLI_PARAM_TYPE,
	MLI_PARTIAL_NEW,
	MLI_PARTIAL_SET,
	MLI_STRING_NEW,
	MLI_STRING_ADD,
	MLI_STRING_ADDS,
	MLI_STRING_END,
	MLI_RESOLVE,
	MLI_IF_DEBUG
} ml_opcode_t;

typedef struct ml_frame_t ml_frame_t;

struct ml_inst_t {
	ml_opcode_t Opcode:8;
	unsigned int PotentialBreakpoint:1;
	unsigned int Processed:1;
	unsigned int Hashed:1;
	unsigned int Reserved:5;
	unsigned int Label:16;
	unsigned int LineNo:32;
	ml_param_t Params[];
};

#define ML_FRAME_REUSE (1 << 0)
#define ML_FRAME_REENTRY (1 << 1)

#define ML_FRAME_REUSE_SIZE 224

typedef struct ml_variable_t ml_variable_t;

struct ml_variable_t {
	const ml_type_t *Type;
	ml_value_t *Value;
	const ml_type_t *VarType;
};

extern ml_type_t MLVariableT[];

const char *ml_closure_debug(ml_value_t *Value);
void ml_closure_sha256(ml_value_t *Closure, unsigned char Hash[SHA256_BLOCK_SIZE]);

void ml_closure_info_finish(ml_closure_info_t *Info);

#ifdef ML_CBOR_BYTECODE

#include "ml_cbor.h"

void ml_cbor_write_closure(ml_closure_t *Closure, ml_stringbuffer_t *Buffer);

ml_value_t *ml_cbor_read_closure(void *Data, int Count, ml_value_t **Args);

#endif

void ml_bytecode_init();

#ifdef	__cplusplus
}
#endif

#endif
