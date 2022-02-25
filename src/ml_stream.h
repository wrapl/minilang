#ifndef ML_STREAM_H
#define ML_STREAM_H

#include "minilang.h"

#ifdef __cplusplus
extern "C" {
#endif

void ml_stream_init(stringmap_t *Globals);

extern ml_type_t MLStreamT[];

void ml_stream_read(ml_state_t *Caller, ml_value_t *Value, void *Address, int Count);
void ml_stream_write(ml_state_t *Caller, ml_value_t *Value, const void *Address, int Count);
void ml_stream_flush(ml_state_t *Caller, ml_value_t *Value);

void ml_stream_read_method(ml_state_t *Caller, ml_value_t *Value, void *Address, int Count);
void ml_stream_write_method(ml_state_t *Caller, ml_value_t *Value, const void *Address, int Count);
void ml_stream_flush_method(ml_state_t *Caller, ml_value_t *Value);

ml_value_t *ml_stream_buffered(ml_value_t *Stream, size_t Size);

extern ml_type_t MLStreamFdT[];

ml_value_t *ml_fd_stream(int Fd);

#ifdef __cplusplus
}
#endif

#endif
