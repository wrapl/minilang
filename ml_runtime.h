#ifndef ML_RUNTIME_H
#define ML_RUNTIME_H

#include "sha256.h"

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct ml_source_t ml_source_t;
typedef struct ml_frame_t ml_frame_t;
typedef struct ml_inst_t ml_inst_t;

struct ml_closure_info_t {
	ml_inst_t *Entry;
	int FrameSize;
	int NumParams, NumUpValues;
	unsigned char Hash[SHA256_BLOCK_SIZE];
};

struct ml_source_t {
	const char *Name;
	int Line;
};

typedef union {
	ml_inst_t *Inst;
	int Index;
	int Count;
	ml_value_t *Value;
	const char *Name;
	ml_closure_info_t *ClosureInfo;
} ml_param_t;

struct ml_inst_t {
	ml_inst_t *(*run)(ml_inst_t *Inst, ml_frame_t *Frame);
	ml_source_t Source;
	ml_param_t Params[];
};

#define MAX_TRACE 16

struct ml_error_t {
	const ml_type_t *Type;
	const char *Error;
	const char *Message;
	ml_source_t Trace[MAX_TRACE];
};

void ml_error_trace_add(ml_value_t *Value, ml_source_t Source);

ml_inst_t *mli_push_run(ml_inst_t *Inst, ml_frame_t *Frame);
ml_inst_t *mli_pop_run(ml_inst_t *Inst, ml_frame_t *Frame);
ml_inst_t *mli_pop2_run(ml_inst_t *Inst, ml_frame_t *Frame);
ml_inst_t *mli_enter_run(ml_inst_t *Inst, ml_frame_t *Frame);
ml_inst_t *mli_var_run(ml_inst_t *Inst, ml_frame_t *Frame);
ml_inst_t *mli_def_run(ml_inst_t *Inst, ml_frame_t *Frame);
ml_inst_t *mli_exit_run(ml_inst_t *Inst, ml_frame_t *Frame);
ml_inst_t *mli_try_run(ml_inst_t *Inst, ml_frame_t *Frame);
ml_inst_t *mli_catch_run(ml_inst_t *Inst, ml_frame_t *Frame);
ml_inst_t *mli_call_run(ml_inst_t *Inst, ml_frame_t *Frame);
ml_inst_t *mli_const_call_run(ml_inst_t *Inst, ml_frame_t *Frame);
ml_inst_t *mli_assign_run(ml_inst_t *Inst, ml_frame_t *Frame);
ml_inst_t *mli_jump_run(ml_inst_t *Inst, ml_frame_t *Frame);
ml_inst_t *mli_if_run(ml_inst_t *Inst, ml_frame_t *Frame);
ml_inst_t *mli_if_var_run(ml_inst_t *Inst, ml_frame_t *Frame);
ml_inst_t *mli_if_def_run(ml_inst_t *Inst, ml_frame_t *Frame);
ml_inst_t *mli_for_run(ml_inst_t *Inst, ml_frame_t *Frame);
ml_inst_t *mli_until_run(ml_inst_t *Inst, ml_frame_t *Frame);
ml_inst_t *mli_while_run(ml_inst_t *Inst, ml_frame_t *Frame);
ml_inst_t *mli_and_run(ml_inst_t *Inst, ml_frame_t *Frame);
ml_inst_t *mli_and_var_run(ml_inst_t *Inst, ml_frame_t *Frame);
ml_inst_t *mli_and_def_run(ml_inst_t *Inst, ml_frame_t *Frame);
ml_inst_t *mli_or_run(ml_inst_t *Inst, ml_frame_t *Frame);
ml_inst_t *mli_exists_run(ml_inst_t *Inst, ml_frame_t *Frame);
ml_inst_t *mli_next_run(ml_inst_t *Inst, ml_frame_t *Frame);
ml_inst_t *mli_key_run(ml_inst_t *Inst, ml_frame_t *Frame);
ml_inst_t *mli_local_run(ml_inst_t *Inst, ml_frame_t *Frame);
ml_inst_t *mli_list_run(ml_inst_t *Inst, ml_frame_t *Frame);
ml_inst_t *mli_append_run(ml_inst_t *Inst, ml_frame_t *Frame);
ml_inst_t *mli_closure_run(ml_inst_t *Inst, ml_frame_t *Frame);

ml_value_t *ml_closure_call(ml_value_t *Value, int Count, ml_value_t **Args);

void ml_closure_debug(ml_closure_info_t *Info);

#ifdef	__cplusplus
}
#endif

#endif
