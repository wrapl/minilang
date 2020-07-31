#include "ml_debugger.h"
#include "ml_runtime.h"
#include "ml_compiler.h"
#include "ml_macros.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct interactive_debugger_info_t interactive_debugger_info_t;

struct interactive_debugger_t {
	ml_debugger_t Base;
	interactive_debugger_info_t *Info;
	ml_state_t *Frame, *Active;
	ml_value_t *Value;
	stringmap_t Globals[1];
	stringmap_t Modules[1];
};

struct interactive_debugger_info_t {
	void (*enter)(void *Data, interactive_debugger_t *Debugger);
	void (*exit)(ml_state_t *Caller, void *Data);
	void (*log)(void *Data, ml_value_t *Value);
	void *Data;
	ml_getter_t GlobalGet;
	void *Globals;
};

typedef struct {
	size_t Count;
	size_t Bits[];
} breakpoints_t;

static size_t *debugger_breakpoints(interactive_debugger_t *Debugger, const char *Source, int Max) {
	breakpoints_t **Slot = (breakpoints_t **)stringmap_slot(Debugger->Modules, Source);
	size_t Count = (Max + SIZE_BITS - 1) / SIZE_BITS;
	if (!Slot[0]) {
		breakpoints_t *New = (breakpoints_t *)GC_MALLOC_ATOMIC(sizeof(breakpoints_t) + Count * sizeof(size_t));
		memset(New->Bits, 0, Count * sizeof(size_t));
		New->Count = Count;
		Slot[0] = New;
	} else if (Count > Slot[0]->Count) {
		breakpoints_t *Old = Slot[0];
		breakpoints_t *New = (breakpoints_t *)GC_MALLOC_ATOMIC(sizeof(breakpoints_t) + Count * sizeof(size_t));
		memset(New->Bits, 0, Count * sizeof(size_t));
		memcpy(New->Bits, Old->Bits, Old->Count * sizeof(size_t));
		New->Count = Count;
		Slot[0] = New;
	}
	return Slot[0]->Bits;
}

ml_value_t *interactive_debugger_get(interactive_debugger_t *Debugger, const char *Name) {
	ml_value_t *Value = (ml_value_t *)stringmap_search(Debugger->Globals, Name);
	if (Value) return Value;
	for (ml_decl_t *Decl = ml_debugger_decls(Debugger->Active); Decl; Decl = Decl->Next) {
		if (!strcmp(Decl->Ident, Name)) {
			if (Decl->Value) return Decl->Value;
			return ml_debugger_local(Debugger->Active, Decl->Index);
		}
	}
	return NULL;
}

void interactive_debugger_resume(interactive_debugger_t *Debugger) {
	ml_state_t *Frame = Debugger->Frame;
	Debugger->Frame = Debugger->Active = NULL;
	return Frame->run(Frame, Debugger->Value);
}

static ml_value_t *debugger_break(interactive_debugger_t *Debugger, int Count, ml_value_t **Args) {
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

static void debugger_continue(ml_state_t *Caller, interactive_debugger_t *Debugger, int Count, ml_value_t **Args) {
	Debugger->Base.StepIn = 0;
	Debugger->Base.StepOverFrame = NULL;
	Debugger->Base.StepOutFrame = NULL;
	return Debugger->Info->exit(Caller, Debugger->Info->Data);
}

static void debugger_step_in(ml_state_t *Caller, interactive_debugger_t *Debugger, int Count, ml_value_t **Args) {
	Debugger->Base.StepIn = 1;
	Debugger->Base.StepOverFrame = NULL;
	Debugger->Base.StepOutFrame = NULL;
	return Debugger->Info->exit(Caller, Debugger->Info->Data);
}

static void debugger_step_over(ml_state_t *Caller, interactive_debugger_t *Debugger, int Count, ml_value_t **Args) {
	Debugger->Base.StepIn = 0;
	Debugger->Base.StepOverFrame = Debugger->Frame;
	Debugger->Base.StepOutFrame = Debugger->Frame;
	return Debugger->Info->exit(Caller, Debugger->Info->Data);
}

static void debugger_step_out(ml_state_t *Caller, interactive_debugger_t *Debugger, int Count, ml_value_t **Args) {
	Debugger->Base.StepIn = 0;
	Debugger->Base.StepOverFrame = NULL;
	Debugger->Base.StepOutFrame = Debugger->Frame;
	return Debugger->Info->exit(Caller, Debugger->Info->Data);
}

static ml_value_t *debugger_locals(interactive_debugger_t *Debugger, int Count, ml_value_t **Args) {
	ml_value_t *Locals = ml_list();
	for (ml_decl_t *Decl = ml_debugger_decls(Debugger->Active); Decl; Decl = Decl->Next) {
		ml_list_put(Locals, ml_string(Decl->Ident, -1));
	}
	return Locals;
}

static ml_value_t *debugger_frames(interactive_debugger_t *Debugger, int Count, ml_value_t **Args) {
	ml_value_t *Backtrace = ml_list();
	ml_state_t *Frame = Debugger->Active;
	while (Frame) {
		if (ml_debugger_check(Frame)) {
			ml_source_t Source = ml_debugger_source(Frame);
			ml_value_t *Location = ml_tuple(2);
			ml_tuple_set(Location, 1, ml_string(Source.Name, -1));
			ml_tuple_set(Location, 2, ml_integer(Source.Line));
			ml_list_put(Backtrace, Location);
		}
		Frame = Frame->Caller;
	}
	return Backtrace;
}

static ml_value_t *debugger_frame_up(interactive_debugger_t *Debugger, int Count, ml_value_t **Args) {
	ml_state_t *Frame = Debugger->Active;
	do {
		Frame = Frame->Caller;
	} while (Frame && !ml_debugger_check(Frame));
	if (Frame) {
		Debugger->Active = Frame;
		ml_source_t Source = ml_debugger_source(Frame);
		ml_value_t *Location = ml_tuple(2);
		ml_tuple_set(Location, 1, ml_string(Source.Name, -1));
		ml_tuple_set(Location, 2, ml_integer(Source.Line));
		return Location;
	} else {
		return ml_error("TraceError", "Reached top of debugging stack");
	}
}

static ml_value_t *debugger_frame_down(interactive_debugger_t *Debugger, int Count, ml_value_t **Args) {
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
		ml_tuple_set(Location, 1, ml_string(Source.Name, -1));
		ml_tuple_set(Location, 2, ml_integer(Source.Line));
		return Location;
	} else {
		return ml_error("TraceError", "Reached bottom of debugging stack");
	}
}

static void debugger_run(interactive_debugger_t *Debugger, ml_state_t *Frame, ml_value_t *Value) {
	Debugger->Frame = Frame;
	Debugger->Value = Value;
	Debugger->Active = Frame;
	ml_source_t Source = ml_debugger_source(Frame);
	ml_value_t *Location = ml_tuple(2);
	ml_tuple_set(Location, 1, ml_string(Source.Name, -1));
	ml_tuple_set(Location, 2, ml_integer(Source.Line));
	Debugger->Info->log(Debugger->Info->Data, Location);
	if (ml_is_error(Value)) {
		Debugger->Info->log(Debugger->Info->Data, Value);
	}
	Debugger->Info->enter(Debugger->Info->Data, Debugger);
}

static void debugger_state_load(ml_state_t *State, ml_value_t *Function) {
	State->run = ml_default_state_run;
	return ml_typeof(Function)->call(State, Function, 0, NULL);
}

static void interactive_debugger_fnx(ml_state_t *Caller, interactive_debugger_info_t *Info, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);

	interactive_debugger_t *Debugger = new(interactive_debugger_t);
	Debugger->Base.run = (void *)debugger_run;
	Debugger->Base.breakpoints = (void *)debugger_breakpoints;
	Debugger->Base.Revision = 1;
	Debugger->Base.StepIn = 1;
	Debugger->Base.BreakOnError = 1;
	Debugger->Info = Info;
	stringmap_insert(Debugger->Globals, "break", ml_cfunction(Debugger, (void *)debugger_break));
	stringmap_insert(Debugger->Globals, "continue", ml_cfunctionx(Debugger, (void *)debugger_continue));
	stringmap_insert(Debugger->Globals, "step_in", ml_cfunctionx(Debugger, (void *)debugger_step_in));
	stringmap_insert(Debugger->Globals, "step_over", ml_cfunctionx(Debugger, (void *)debugger_step_over));
	stringmap_insert(Debugger->Globals, "step_out", ml_cfunctionx(Debugger, (void *)debugger_step_out));
	stringmap_insert(Debugger->Globals, "locals", ml_cfunction(Debugger, (void *)debugger_locals));
	stringmap_insert(Debugger->Globals, "frames", ml_cfunction(Debugger, (void *)debugger_frames));
	stringmap_insert(Debugger->Globals, "frame_up", ml_cfunction(Debugger, (void *)debugger_frame_up));
	stringmap_insert(Debugger->Globals, "frame_down", ml_cfunction(Debugger, (void *)debugger_frame_down));

	ml_state_t *State = ml_state_new(Caller);
	ml_context_set(State->Context, ML_DEBUGGER_INDEX, Debugger);
	if (ml_is(Args[0], MLStringT)) {
		State->run = debugger_state_load;
		const char *FileName = ml_string_value(Args[0]);
		ml_load(State, Info->GlobalGet, Info->Globals, FileName, NULL);
	} else {
		ml_value_t *Function = Args[0];
		return ml_typeof(Function)->call(State, Function, Count - 1, Args + 1);
	}
}

ml_value_t *interactive_debugger(
	void (*Enter)(void *Data, interactive_debugger_t *Debugger),
	void (*Exit)(ml_state_t *Caller, void *Data),
	void (*Log)(void *Data, ml_value_t *Value),
	void *Data,
	ml_getter_t GlobalGet,
	void *Globals
) {
	interactive_debugger_info_t *Info = new(interactive_debugger_info_t);
	Info->enter = Enter;
	Info->exit = Exit;
	Info->log = Log;
	Info->Data = Data;
	Info->GlobalGet = GlobalGet;
	Info->Globals = Globals;
	return ml_cfunctionx(Info, (void *)interactive_debugger_fnx);
}
