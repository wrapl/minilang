#ifndef ML_INTERNAL_H
#define ML_INTERNAL_H

#include "ml_runtime.h"

typedef struct ml_frame_t ml_frame_t;
typedef struct ml_inst_t ml_inst_t;

struct ml_closure_info_t {
	ml_inst_t *Entry;
	int FrameSize;
	int NumParams, NumUpValues;
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
	const char *Name;
	ml_closure_info_t *ClosureInfo;
} ml_param_t;

typedef enum {
	MLI_RETURN,
	MLI_SUSPEND,
	MLI_PUSH,
	MLI_POP,
	MLI_POP2,
	MLI_POP3,
	MLI_ENTER,
	MLI_VAR,
	MLI_DEF,
	MLI_EXIT,
	MLI_TRY,
	MLI_CATCH,
	MLI_CALL,
	MLI_CONST_CALL,
	MLI_ASSIGN,
	MLI_JUMP,
	MLI_IF,
	MLI_IF_VAR,
	MLI_IF_DEF,
	MLI_FOR,
	MLI_UNTIL,
	MLI_WHILE,
	MLI_AND,
	MLI_AND_VAR,
	MLI_AND_DEF,
	MLI_OR,
	MLI_EXISTS,
	MLI_NEXT,
	MLI_CURRENT,
	MLI_KEY,
	MLI_LOCAL,
	MLI_LIST,
	MLI_APPEND,
	MLI_CLOSURE
} ml_opcode_t;

struct ml_inst_t {
	ml_opcode_t Opcode;
	ml_source_t Source;
	ml_param_t Params[];
};

void ml_closure_info_debug(ml_closure_info_t *Info);

ml_value_t *ml_string_new(void *Data, int Count, ml_value_t **Args);
ml_value_t *ml_list_new(void *Data, int Count, ml_value_t **Args);
ml_value_t *ml_tree_new(void *Data, int Count, ml_value_t **Args);

#endif
