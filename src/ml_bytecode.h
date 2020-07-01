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
#ifdef USE_ML_JIT
	void *JITStart, *JITEntry, *JITReturn;
#endif
	stringmap_t Params[1];
	int End, FrameSize;
	int NumParams, NumUpValues;
	int ExtraArgs, NamedArgs;
	unsigned char Hash[SHA256_BLOCK_SIZE];
};

struct ml_closure_t {
	const ml_type_t *Type;
	ml_closure_info_t *Info;
	int PartialCount;
	ml_value_t *UpValues[];
};

typedef union {
	ml_inst_t *Inst;
	int Index;
	int Count;
	ml_value_t *Value;
	ml_closure_info_t *ClosureInfo;
	ml_decl_t *Decls;
} ml_param_t;

typedef enum {
	MLI_RETURN,
	MLI_SUSPEND,
	MLI_RESUME,
	MLI_NIL,
	MLI_SOME,
	MLI_IF,
	MLI_ELSE,
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
	MLI_VAR,
	MLI_VARX,
	MLI_LET,
	MLI_LETI,
	MLI_LETX,
	MLI_FOR,
	MLI_NEXT,
	MLI_VALUE,
	MLI_KEY,
	MLI_CALL,
	MLI_CONST_CALL,
	MLI_ASSIGN,
	MLI_LOCAL,
	MLI_LOCALX,
	MLI_TUPLE_NEW,
	MLI_TUPLE_SET,
	MLI_LIST_NEW,
	MLI_LIST_APPEND,
	MLI_MAP_NEW,
	MLI_MAP_INSERT,
	MLI_CLOSURE,
	MLI_PARTIAL_NEW,
	MLI_PARTIAL_SET
} ml_opcode_t;

typedef enum {
	MLIT_NONE,
	MLIT_INST,
	MLIT_INST_INST,
	MLIT_INST_INDEX,
	MLIT_INST_INDEX_COUNT,
	MLIT_INST_COUNT,
	MLIT_INST_COUNT_COUNT,
	MLIT_INST_COUNT_VALUE,
	MLIT_INST_VALUE,
	MLIT_INST_CLOSURE
} ml_inst_type_t;

typedef struct ml_frame_t ml_frame_t;

extern const ml_inst_type_t MLInstTypes[];

typedef void (*ml_inst_fn_t)(ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top, ml_inst_t *Inst);

struct ml_inst_t {
#ifdef ML_USE_INST_FNS
	ml_inst_fn_t run;
#endif
	ml_opcode_t Opcode:8;
	unsigned int PotentialBreakpoint:1;
	unsigned int Processed:1;
	unsigned int Reserved:6;
	unsigned int Label:16;
	unsigned int LineNo:32;
	ml_param_t Params[];
};

struct ml_frame_t {
	ml_state_t Base;
	const char *Source;
	ml_inst_t *Inst;
	ml_value_t **Top;
	ml_inst_t *OnError;
	ml_value_t **UpValues;
#ifdef USE_ML_SCHEDULER
	ml_schedule_t Schedule;
#endif
#ifdef DEBUG_VERSION
	ml_debugger_t *Debugger;
	size_t *Breakpoints;
	ml_decl_t *Decls;
	size_t Revision;
	int DebugReentry;
#endif
	ml_value_t *Stack[];
};

const char *ml_closure_debug(ml_value_t *Value);
void ml_closure_sha256(ml_value_t *Closure, unsigned char Hash[SHA256_BLOCK_SIZE]);

void ml_closure_info_finish(ml_closure_info_t *Info);

#ifdef USE_ML_CBOR_BYTECODE

#include "ml_cbor.h"

void ml_cbor_write_closure(ml_closure_t *Closure, ml_stringbuffer_t *Buffer);

ml_value_t *ml_cbor_read_closure(void *Data, int Count, ml_value_t **Args);

#endif

void ml_bytecode_init();

#ifdef	__cplusplus
}
#endif

#endif
