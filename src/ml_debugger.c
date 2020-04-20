#include "ml_debugger.h"
#include "ml_runtime.h"
#include "ml_macros.h"
#include "ml_console.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static void ml_debug_state_fn(ml_state_t *State, ml_value_t *Value) {
	ML_CONTINUE(State->Caller, Value);
}

typedef struct {
	size_t Count;
	size_t Bits[];
} breakpoints_t;

typedef struct {
	ml_debugger_t Base;
	ml_state_t *Frame, *Active;
	stringmap_t Globals[1];
	stringmap_t Modules[1];
} debugger_t;

static size_t *debugger_breakpoints(debugger_t *Debugger, const char *Source, int Max) {
	breakpoints_t **Slot = (breakpoints_t **)stringmap_slot(Debugger->Modules, Source);
	size_t Count = (Max + SIZE_BITS - 1) / SIZE_BITS;
	if (!Slot[0]) {
		breakpoints_t *New = (breakpoints_t *)GC_malloc_atomic(sizeof(breakpoints_t) + Count * sizeof(size_t));
		memset(New->Bits, 0, Count * sizeof(size_t));
		New->Count = Count;
		Slot[0] = New;
	} else if (Count > Slot[0]->Count) {
		breakpoints_t *Old = Slot[0];
		breakpoints_t *New = (breakpoints_t *)GC_malloc_atomic(sizeof(breakpoints_t) + Count * sizeof(size_t));
		memset(New->Bits, 0, Count * sizeof(size_t));
		memcpy(New->Bits, Old->Bits, Old->Count * sizeof(size_t));
		New->Count = Count;
		Slot[0] = New;
	}
	return Slot[0]->Bits;
}

static stringmap_t *MainGlobals;

static ml_value_t *debugger_get(debugger_t *Debugger, const char *Name) {
	ml_value_t *Value = stringmap_search(Debugger->Globals, Name);
	if (Value) return Value;
	Value = stringmap_search(MainGlobals, Name);
	if (Value) return Value;
	for (mlc_decl_t *Decl = ml_debugger_decls(Debugger->Active); Decl; Decl = Decl->Next) {
		if (!strcmp(Decl->Ident, Name)) {
			if (Decl->Value) return Decl->Value;
			return ml_debugger_local(Debugger->Active, Decl->Index);
		}
	}
	return NULL;
}

static ml_value_t *debugger_break(debugger_t *Debugger, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(2);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	ML_CHECK_ARG_TYPE(1, MLIntegerT);
	const char *Source = ml_string_value(Args[0]);
	int LineNo = ml_integer_value(Args[1]);
	size_t *Breakpoints = debugger_breakpoints(Debugger, Source, LineNo);
	Breakpoints[LineNo / SIZE_BITS] |= 1 << (LineNo % SIZE_BITS);
	++Debugger->Base.Revision;
	return MLNil;
}

static void debugger_continue(ml_state_t *Caller, debugger_t *Debugger, int Count, ml_value_t **Args) {
	Debugger->Base.StepIn = 0;
	Debugger->Base.StepOverFrame = NULL;
	Debugger->Base.StepOutFrame = NULL;
	ML_RETURN(MLConsoleBreak);
}

static void debugger_step_in(ml_state_t *Caller, debugger_t *Debugger, int Count, ml_value_t **Args) {
	Debugger->Base.StepIn = 1;
	Debugger->Base.StepOverFrame = NULL;
	Debugger->Base.StepOutFrame = NULL;
	ML_RETURN(MLConsoleBreak);
}

static void debugger_step_over(ml_state_t *Caller, debugger_t *Debugger, int Count, ml_value_t **Args) {
	Debugger->Base.StepIn = 0;
	Debugger->Base.StepOverFrame = Debugger->Frame;
	Debugger->Base.StepOutFrame = Debugger->Frame;
	ML_RETURN(MLConsoleBreak);
}

static void debugger_step_out(ml_state_t *Caller, debugger_t *Debugger, int Count, ml_value_t **Args) {
	Debugger->Base.StepIn = 0;
	Debugger->Base.StepOverFrame = NULL;
	Debugger->Base.StepOutFrame = Debugger->Frame;
	ML_RETURN(MLConsoleBreak);
}

static ml_value_t *debugger_locals(debugger_t *Debugger, int Count, ml_value_t **Args) {
	ml_value_t *Locals = ml_list();
	for (mlc_decl_t *Decl = ml_debugger_decls(Debugger->Active); Decl; Decl = Decl->Next) {
		ml_list_put(Locals, ml_string(Decl->Ident, -1));
	}
	return Locals;
}

static ml_value_t *debugger_frames(debugger_t *Debugger, int Count, ml_value_t **Args) {
	ml_value_t *Backtrace = ml_list();
	ml_state_t *Frame = Debugger->Active;
	while (Frame) {
		if (ml_debugger_check(Frame)) {
			ml_source_t Source = ml_debugger_source(Frame);
			ml_value_t *Location = ml_tuple(2);
			ml_tuple_set(Location, 0, ml_string(Source.Name, -1));
			ml_tuple_set(Location, 1, ml_integer(Source.Line));
			ml_list_put(Backtrace, Location);
		}
		Frame = Frame->Caller;
	}
	return Backtrace;
}

static ml_value_t *debugger_frame_up(debugger_t *Debugger, int Count, ml_value_t **Args) {
	ml_state_t *Frame = Debugger->Active;
	do {
		Frame = Frame->Caller;
	} while (Frame && !ml_debugger_check(Frame));
	if (Frame) {
		Debugger->Active = Frame;
		ml_source_t Source = ml_debugger_source(Frame);
		ml_value_t *Location = ml_tuple(2);
		ml_tuple_set(Location, 0, ml_string(Source.Name, -1));
		ml_tuple_set(Location, 1, ml_integer(Source.Line));
		return Location;
	} else {
		return ml_error("TraceError", "Reached top of debugging stack");
	}
}

static ml_value_t *debugger_frame_down(debugger_t *Debugger, int Count, ml_value_t **Args) {
	ml_state_t *Frame = NULL;
	ml_state_t *Current = Debugger->Frame;
	ml_state_t *Active = Debugger->Active;
	while (Current != Active) {
		if (ml_debugger_check(Current)) {
			Frame = Current;
		}
		Current = Current->Caller;
	}
	if (Frame) {
		Debugger->Active = Frame;
		ml_source_t Source = ml_debugger_source(Frame);
		ml_value_t *Location = ml_tuple(2);
		ml_tuple_set(Location, 0, ml_string(Source.Name, -1));
		ml_tuple_set(Location, 1, ml_integer(Source.Line));
		return Location;
	} else {
		return ml_error("TraceError", "Reached bottom of debugging stack");
	}
}

static void debugger_run(debugger_t *Debugger, ml_state_t *Frame, ml_value_t *Value) {
	Debugger->Frame = Frame;
	Debugger->Active = Frame;
	ml_source_t Source = ml_debugger_source(Frame);
	printf("\e[34m%s\e[0m:%d\n", Source.Name, Source.Line);
	if (Value->Type == MLErrorT) {
		printf("Error: %s\n", ml_error_message(Value));
		const char *Source;
		int Line;
		for (int I = 0; ml_error_trace(Value, I, &Source, &Line); ++I) printf("\t%s:%d\n", Source, Line);
	}
	ml_console(debugger_get, Debugger, "\e[34m>>>\e[0m ", "\e[34m...\e[0m ");
	return Frame->run(Frame, Value);
}

static void debugger_start(ml_state_t *State, ml_value_t *Function) {
	State->run = ml_debug_state_fn;
	return Function->Type->call(State, Function, 0, NULL);
}

ML_FUNCTIONX(Debug) {
	ML_CHECKX_ARG_COUNT(1);
	ml_context_t *Context = ml_context_new(Caller->Context);
	debugger_t *Debugger = new(debugger_t);
	stringmap_insert(Debugger->Globals, "break", ml_function(Debugger, debugger_break));
	stringmap_insert(Debugger->Globals, "continue", ml_functionx(Debugger, debugger_continue));
	stringmap_insert(Debugger->Globals, "step_in", ml_functionx(Debugger, debugger_step_in));
	stringmap_insert(Debugger->Globals, "step_over", ml_functionx(Debugger, debugger_step_over));
	stringmap_insert(Debugger->Globals, "step_out", ml_functionx(Debugger, debugger_step_out));

	stringmap_insert(Debugger->Globals, "locals", ml_function(Debugger, debugger_locals));
	stringmap_insert(Debugger->Globals, "frames", ml_function(Debugger, debugger_frames));
	stringmap_insert(Debugger->Globals, "frame_up", ml_function(Debugger, debugger_frame_up));
	stringmap_insert(Debugger->Globals, "frame_down", ml_function(Debugger, debugger_frame_down));


	Debugger->Base.run = debugger_run;
	Debugger->Base.breakpoints = debugger_breakpoints;
	Debugger->Base.Revision = 1;
	Debugger->Base.StepIn = 1;
	Debugger->Base.BreakOnError = 1;
	ml_context_set(Context, ML_DEBUGGER_INDEX, Debugger);
	ml_state_t *State = new(ml_state_t);
	State->Caller = Caller;
	State->run = ml_debug_state_fn;
	State->Context = Context;
	if (Args[0]->Type == MLStringT) {
		State->run = debugger_start;
		const char *FileName = ml_string_value(Args[0]);
		ml_load(State, stringmap_search, MainGlobals, FileName, NULL);
	} else {
		ml_value_t *Function = Args[0];
		return Function->Type->call(State, Function, Count - 1, Args + 1);
	}
}

void ml_debugger_init(stringmap_t *Globals) {
#include "ml_debugger_init.c"
	if (Globals) {
		MainGlobals = Globals;
		stringmap_insert(Globals, "debug", Debug);
	}
}
