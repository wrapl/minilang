#ifndef ML_STREAM_H
#define ML_STREAM_H

/// \defgroup streams
/// @{
///

#include "minilang.h"

#ifdef __cplusplus
extern "C" {
#endif

void ml_stream_init(stringmap_t *Globals);

extern ml_type_t MLStreamT[];

void ml_stream_read(ml_state_t *Caller, ml_value_t *Value, void *Address, int Count);
void ml_stream_write(ml_state_t *Caller, ml_value_t *Value, const void *Address, int Count);
void ml_stream_flush(ml_state_t *Caller, ml_value_t *Value);
void ml_stream_seek(ml_state_t *Caller, ml_value_t *Value, int64_t Offset, int Mode);
void ml_stream_tell(ml_state_t *Caller, ml_value_t *Value);

void ml_stream_read_method(ml_state_t *Caller, ml_value_t *Value, void *Address, int Count);
void ml_stream_write_method(ml_state_t *Caller, ml_value_t *Value, const void *Address, int Count);
void ml_stream_flush_method(ml_state_t *Caller, ml_value_t *Value);
void ml_stream_seek_method(ml_state_t *Caller, ml_value_t *Value, int64_t Offset, int Mode);
void ml_stream_tell_method(ml_state_t *Caller, ml_value_t *Value);

ml_value_t *ml_stream_buffered(ml_value_t *Stream, size_t Size);

extern ml_type_t MLStreamFdT[];

ml_value_t *ml_fd_stream(ml_type_t *Type, int Fd);
int ml_fd_stream_fd(ml_value_t *Stream);

#ifdef __cplusplus
}
#endif

/// @}

#endif
