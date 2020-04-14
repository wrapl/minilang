#ifndef ML_BYTECODE_THREADED_H
#define ML_BYTECODE_THREADED_H

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
	debug_function_t *Debug;
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

typedef struct ml_frame_t ml_frame_t;

struct ml_inst_t {
	ml_value_t *(*run)(ml_inst_t *Inst, ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top);
	int Flags:32;
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
