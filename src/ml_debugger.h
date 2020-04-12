#ifndef DEBUGGER_H
#define DEBUGGER_H

#include "ml_types.h"

typedef struct ml_debugger_t ml_debugger_t;
typedef struct ml_debug_module_t ml_debug_module_t;
typedef struct ml_debug_function_t ml_debug_function_t;

ml_debug_module_t *debug_module(const char *Name);
void debug_add_line(ml_debug_module_t *Module, const char *Line);
void debug_add_global_variable(ml_debug_module_t *Module, const char *Name, ml_value_t **Address);
void debug_add_global_constant(ml_debug_module_t *Module, const char *Name, ml_value_t *Value);
ml_debug_function_t *debug_function(ml_debug_module_t *Module, int LineNo);

unsigned char *debug_breakpoints(ml_debug_function_t *Function, int LineNo);
unsigned char *debug_break_on_send();
unsigned char *debug_break_on_message();
int debug_module_id(ml_debug_function_t *Function);
void debug_add_local_var(ml_debug_function_t *Function, const char *Name, int Index);
void debug_add_local_def(ml_debug_function_t *Function, const char *Name, ml_value_t *Value);

void debug_enable(const char *SocketPath, int ClientMode);

/*
void debug_break_impl(ml_frame_t *State, int LineNo);
void debug_error_impl(ml_frame_t *State, int LineNo, ml_value_t *Message);
void debug_enter_impl(ml_frame_t *State);
void debug_exit_impl(ml_frame_t *State);
*/

#endif
