#ifndef ML_BYTECODE_H
#define ML_BYTECODE_H

#include "ml_debugger.h"
#include "ml_types.h"
#include "ml_runtime.h"

#ifdef	__cplusplus
extern "C" {
#endif

#include "sha256.h"

typedef struct ml_closure_t ml_closure_t;
typedef struct ml_closure_info_t ml_closure_info_t;
typedef struct ml_inst_t ml_inst_t;

extern ml_type_t MLClosureT[];

#define ML_PARAM_DEFAULT 0
#define ML_PARAM_EXTRA 1
#define ML_PARAM_NAMED 2

struct ml_closure_info_t {
	ml_inst_t *Entry, *Return;
	const char *Source;
	ml_debug_function_t *Debug;
	stringmap_t Params[1];
	int FrameSize;
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
} ml_param_t;

typedef enum {
	MLI_RETURN,
	MLI_SUSPEND,
	MLI_RESUME,
	MLI_NIL,
	MLI_SOME,
	MLI_IF,
	MLI_IF_VAR,
	MLI_IF_LET,
	MLI_ELSE,
	MLI_PUSH,
	MLI_PUSHX,
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
	MLI_LETX,
	MLI_FOR,
	MLI_NEXT,
	MLI_VALUE,
	MLI_KEY,
	MLI_CALL,
	MLI_CONST_CALL,
	MLI_ASSIGN,
	MLI_LOCAL,
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

struct ml_inst_t {
	ml_opcode_t Opcode:8;
	int Flags:24;
	int LineNo:32;
	ml_param_t Params[];
};

const char *ml_closure_debug(ml_value_t *Value);
void ml_closure_sha256(ml_value_t *Closure, unsigned char Hash[SHA256_BLOCK_SIZE]);

void ml_closure_info_sha256(ml_closure_info_t *Info);
const char *ml_closure_info_debug(ml_closure_info_t *Info);

void ml_bytecode_init();

#ifdef	__cplusplus
}
#endif

#endif
