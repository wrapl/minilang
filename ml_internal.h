#ifndef ML_INTERNAL_H
#define ML_INTERNAL_H

#include "ml_types.h"

typedef struct ml_inst_t ml_inst_t;
typedef struct ml_frame_t ml_frame_t;

struct ml_tuple_t {
	const ml_type_t *Type;
	size_t Size;
	ml_value_t *Values[];
};

struct ml_closure_info_t {
	ml_inst_t *Entry, *Return;
	int FrameSize;
	int NumParams, NumUpValues, CanSuspend;
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
	MLI_ENTER,
	MLI_EXIT,
	MLI_LOOP,
	MLI_TRY,
	MLI_CATCH,
	MLI_LOAD,
	MLI_VAR,
	MLI_LET,
	MLI_FOR,
	MLI_NEXT,
	MLI_VALUE,
	MLI_KEY,
	MLI_PUSH_RESULT,
	MLI_CALL,
	MLI_CONST_CALL,
	MLI_RESULT,
	MLI_ASSIGN,
	MLI_LOCAL,
	MLI_TUPLE,
	MLI_CLOSURE
} ml_opcode_t;

struct ml_inst_t {
	ml_opcode_t Opcode;
	ml_source_t Source;
	ml_param_t Params[];
};

struct ml_frame_t {
	ml_state_t Base;
	ml_inst_t *Inst;
	ml_value_t **Top;
	ml_inst_t *OnError;
	ml_value_t **UpValues;
	ml_value_t *Stack[];
};

void ml_closure_info_debug(ml_closure_info_t *Info);

ml_value_t *ml_string_new(void *Data, int Count, ml_value_t **Args);
ml_value_t *ml_list_new(void *Data, int Count, ml_value_t **Args);
ml_value_t *ml_map_new(void *Data, int Count, ml_value_t **Args);

void ml_runtime_init();

#endif
