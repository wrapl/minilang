#include "ml_debugger.h"
#include "ml_runtime.h"
#include "ml_compiler.h"
#include "ml_macros.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#undef ML_CATEGORY
#define ML_CATEGORY "debugger"

typedef struct interactive_debugger_info_t interactive_debugger_info_t;

struct interactive_debugger_info_t {
	void (*enter)(void *Data, interactive_debugger_t *Debugger, ml_source_t Source, int Index);
	void (*exit)(void *Data, interactive_debugger_t *Debugger, ml_state_t *Caller, int Index);
	void (*log)(void *Data, ml_value_t *Value);
	void *Data;
	ml_getter_t GlobalGet;
	void *Globals;
};

typedef struct {
	ml_state_t *State, *Active;
	ml_value_t *Value;
} debug_thread_t;

struct interactive_debugger_t {
	ml_debugger_t Base;
	interactive_debugger_info_t *Info;
	debug_thread_t *Threads;
	debug_thread_t *ActiveThread;
	stringmap_t Globals[1];
	stringmap_t Modules[1];
	int MaxThreads, NumThreads;
};

typedef struct {
	size_t Count;
	size_t Bits[];
} breakpoints_t;

static size_t *debugger_breakpoints(interactive_debugger_t *Debugger, const char *Source, int Max) {
	breakpoints_t **Slot = (breakpoints_t **)stringmap_slot(Debugger->Modules, Source);
	size_t Count = (Max + SIZE_BITS) / SIZE_BITS;
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
	debug_thread_t *Thread = Debugger->ActiveThread;
	if (!Thread) return NULL;
	for (ml_decl_t *Decl = ml_debugger_decls(Thread->Active); Decl; Decl = Decl->Next) {
		if (!strcmp(Decl->Ident, Name)) {
			if (Decl->Value) return Decl->Value;
			return ml_debugger_local(Thread->Active, Decl->Index);
		}
	}
	return NULL;
}

ml_source_t interactive_debugger_switch(interactive_debugger_t *Debugger, int Index) {
	if (Index < 0 || Index >= Debugger->MaxThreads) return (ml_source_t){"Invalid thread", Index};
	debug_thread_t *Thread = Debugger->Threads + Index;
	if (!Thread) return (ml_source_t){"Invalid thread", Index};
	Debugger->ActiveThread = Thread;
	return ml_debugger_source(Thread->Active);
}

void interactive_debugger_resume(interactive_debugger_t *Debugger) {
	debug_thread_t *Thread = Debugger->ActiveThread;
	ml_state_t *State = Thread->State;
	ml_value_t *Value = Thread->Value;
	Debugger->ActiveThread = NULL;
	Thread->Active = Thread->State = NULL;
	Thread->Value = NULL;
	--Debugger->NumThreads;
	return State->run(State, Value);
}

static int debugger_breakpoints_fn(const char *Module, breakpoints_t *Breakpoints, ml_value_t *Result) {
	ml_value_t *Lines = ml_list();
	for (int I = 0; I < Breakpoints->Count; ++I) {
		size_t Bits = Breakpoints->Bits[I];
		int J = 0;
		while (Bits) {
			if (Bits % 2) ml_list_put(Lines, ml_integer(SIZE_BITS * I + J));
			++J;
			Bits >>= 1;
		}
	}
	ml_map_insert(Result, ml_cstring(Module), Lines);
	return 0;
}

static ml_value_t *debugger_breakpoints_list(interactive_debugger_t *Debugger, int Count, ml_value_t **Args) {
	ml_value_t *Result = ml_map();
	stringmap_foreach(Debugger->Modules, Result, (void *)debugger_breakpoints_fn);
	return Result;
}

static ml_value_t *debugger_breakpoint_set(interactive_debugger_t *Debugger, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(2);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	ML_CHECK_ARG_TYPE(1, MLIntegerT);
	const char *Source = ml_string_value(Args[0]);
	size_t LineNo = ml_integer_value_fast(Args[1]);
	size_t *Breakpoints = debugger_breakpoints(Debugger, Source, LineNo);
	Breakpoints[LineNo / SIZE_BITS] |= 1L << (LineNo % SIZE_BITS);
	++Debugger->Base.Revision;
	return debugger_breakpoints_list(Debugger, 0, NULL);
}

static ml_value_t *debugger_breakpoint_clear(interactive_debugger_t *Debugger, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(2);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	ML_CHECK_ARG_TYPE(1, MLIntegerT);
	const char *Source = ml_string_value(Args[0]);
	size_t LineNo = ml_integer_value_fast(Args[1]);
	size_t *Breakpoints = debugger_breakpoints(Debugger, Source, LineNo);
	Breakpoints[LineNo / SIZE_BITS] &= ~(1L << (LineNo % SIZE_BITS));
	++Debugger->Base.Revision;
	return debugger_breakpoints_list(Debugger, 0, NULL);
}

static void debugger_continue(ml_state_t *Caller, interactive_debugger_t *Debugger, int Count, ml_value_t **Args) {
	debug_thread_t *Thread = Debugger->ActiveThread;
	if (!Thread) return;
	int Index = Thread - Debugger->Threads;
	Debugger->Base.StepIn = 0;
	ml_debugger_step_mode(Thread->State, 0, 0);
	return Debugger->Info->exit(Debugger->Info->Data, Debugger, Caller, Index);
}

static void debugger_step_in(ml_state_t *Caller, interactive_debugger_t *Debugger, int Count, ml_value_t **Args) {
	debug_thread_t *Thread = Debugger->ActiveThread;
	if (!Thread) return;
	Debugger->Base.StepIn = 1;
	int Index = Thread - Debugger->Threads;
	ml_debugger_step_mode(Thread->State, 0, 0);
	return Debugger->Info->exit(Debugger->Info->Data, Debugger, Caller, Index);
}

static void debugger_step_over(ml_state_t *Caller, interactive_debugger_t *Debugger, int Count, ml_value_t **Args) {
	debug_thread_t *Thread = Debugger->ActiveThread;
	if (!Thread) return;
	Debugger->Base.StepIn = 0;
	int Index = Thread - Debugger->Threads;
	ml_debugger_step_mode(Thread->State, 1, 0);
	return Debugger->Info->exit(Debugger->Info->Data, Debugger, Caller, Index);
}

static void debugger_step_out(ml_state_t *Caller, interactive_debugger_t *Debugger, int Count, ml_value_t **Args) {
	debug_thread_t *Thread = Debugger->ActiveThread;
	if (!Thread) return;
	Debugger->Base.StepIn = 0;
	int Index = Thread - Debugger->Threads;
	ml_debugger_step_mode(Thread->State, 0, 1);
	return Debugger->Info->exit(Debugger->Info->Data, Debugger, Caller, Index);
}

static ml_value_t *debugger_locals(interactive_debugger_t *Debugger, int Count, ml_value_t **Args) {
	debug_thread_t *Thread = Debugger->ActiveThread;
	if (!Thread) return ml_error("DebugError", "No active thread");
	ml_state_t *Frame = Thread->Active;
	if (Count > 0) {
		ML_CHECK_ARG_TYPE(0, MLIntegerT);
		int Depth = ml_integer_value(Args[0]);
		while (--Depth >= 0) {
			do {
				Frame = Frame->Caller;
				if (!Frame) return ml_error("DebugError", "Invalid frame depth");
			} while (!ml_debugger_check(Frame));
		}
	}
	ml_value_t *Locals = ml_map();
	for (ml_decl_t *Decl = ml_debugger_decls(Frame); Decl; Decl = Decl->Next) {
		ml_value_t *Value = Decl->Value ?: ml_debugger_local(Frame, Decl->Index);
		if (!Value) continue;
		ml_map_insert(Locals, ml_string(Decl->Ident, -1), Value);
	}
	return Locals;
}

static ml_value_t *debugger_frames(interactive_debugger_t *Debugger, int Count, ml_value_t **Args) {
	debug_thread_t *Thread = Debugger->ActiveThread;
	if (!Thread) return ml_error("DebugError", "No active thread");
	ml_value_t *Backtrace = ml_list();
	ml_state_t *Frame = Thread->Active;
	while (Frame) {
		if (ml_debugger_check(Frame)) {
			ml_source_t Source = ml_debugger_source(Frame);
			ml_value_t *Location = ml_tuplev(2, ml_string(Source.Name, -1), ml_integer(Source.Line));
			ml_list_put(Backtrace, Location);
		}
		Frame = Frame->Caller;
	}
	return Backtrace;
}

static ml_value_t *debugger_frame_up(interactive_debugger_t *Debugger, int Count, ml_value_t **Args) {
	debug_thread_t *Thread = Debugger->ActiveThread;
	if (!Thread) return ml_error("DebugError", "No active thread");
	ml_state_t *Frame = Thread->Active;
	do {
		Frame = Frame->Caller;
	} while (Frame && !ml_debugger_check(Frame));
	if (Frame) {
		Thread->Active = Frame;
		ml_source_t Source = ml_debugger_source(Frame);
		return ml_tuplev(2, ml_string(Source.Name, -1), ml_integer(Source.Line));
	} else {
		return ml_error("TraceError", "Reached top of debugging stack");
	}
}

static ml_value_t *debugger_frame_down(interactive_debugger_t *Debugger, int Count, ml_value_t **Args) {
	debug_thread_t *Thread = Debugger->ActiveThread;
	if (!Thread) return ml_error("DebugError", "No active thread");
	ml_state_t *Frame = NULL;
	ml_state_t *Current = Thread->State;
	ml_state_t *Active = Thread->Active;
	while (Current != Active) {
		if (ml_debugger_check(Current)) {
			Frame = Current;
		}
		Current = Current->Caller;
	}
	if (Frame) {
		Thread->Active = Frame;
		ml_source_t Source = ml_debugger_source(Frame);
		return ml_tuplev(2, ml_string(Source.Name, -1), ml_integer(Source.Line));
	} else {
		return ml_error("TraceError", "Reached bottom of debugging stack");
	}
}

static ml_value_t *debugger_threads(interactive_debugger_t *Debugger, int Count, ml_value_t **Args) {
	ml_value_t *Threads = ml_list();
	debug_thread_t *Thread = Debugger->Threads;
	for (int I = 0; I < Debugger->MaxThreads; ++I, ++Thread) {
		if (Thread->State) {
			ml_source_t Source = ml_debugger_source(Thread->Active);
			ml_value_t *Location = ml_tuplev(3, ml_integer(I), ml_string(Source.Name, -1), ml_integer(Source.Line));
			ml_list_put(Threads, Location);
		}
	}
	return Threads;
}

static ml_value_t *debugger_thread(interactive_debugger_t *Debugger, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLIntegerT);
	int Index = ml_integer_value_fast(Args[0]);
	if (Index < 0 || Index >= Debugger->MaxThreads) return ml_error("IndexError", "Invalid thread number");
	debug_thread_t *Thread = Debugger->Threads + Index;
	if (!Thread->State) return ml_error("IndexError", "Invalid thread number");
	Debugger->ActiveThread = Thread;
	ml_source_t Source = ml_debugger_source(Thread->Active);
	return ml_tuplev(2, ml_string(Source.Name, -1), ml_integer(Source.Line));
}

static void debugger_run(interactive_debugger_t *Debugger, ml_state_t *State, ml_value_t *Value) {
	if (Debugger->NumThreads == Debugger->MaxThreads) {
		int MaxThreads = 2 * Debugger->MaxThreads;
		debug_thread_t *Threads = anew(debug_thread_t, MaxThreads);
		memcpy(Threads, Debugger->Threads, Debugger->MaxThreads * sizeof(debug_thread_t));
		int Index =  Debugger->ActiveThread - Debugger->Threads;
		Debugger->Threads = Threads;
		Debugger->MaxThreads = MaxThreads;
		Debugger->ActiveThread = Threads + Index;
	}
	++Debugger->NumThreads;
	debug_thread_t *Thread = Debugger->Threads;
	while (Thread->State) ++Thread;
	Thread->State = Thread->Active = State;
	Thread->Value = Value;
	Debugger->ActiveThread = Thread;
	ml_source_t Source = ml_debugger_source(State);
	if (ml_is_error(Value)) {
		Debugger->Info->log(Debugger->Info->Data, Value);
	}
	Debugger->Info->enter(Debugger->Info->Data, Debugger, Source, Thread - Debugger->Threads);
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
	Debugger->MaxThreads = 16;
	Debugger->NumThreads = 0;
	Debugger->Threads = anew(debug_thread_t, 16);
	stringmap_insert(Debugger->Globals, "breakpoint_set", ml_cfunction(Debugger, (void *)debugger_breakpoint_set));
	stringmap_insert(Debugger->Globals, "breakpoint_clear", ml_cfunction(Debugger, (void *)debugger_breakpoint_clear));
	stringmap_insert(Debugger->Globals, "breakpoints", ml_cfunction(Debugger, (void *)debugger_breakpoints_list));
	stringmap_insert(Debugger->Globals, "continue", ml_cfunctionx(Debugger, (void *)debugger_continue));
	stringmap_insert(Debugger->Globals, "step_in", ml_cfunctionx(Debugger, (void *)debugger_step_in));
	stringmap_insert(Debugger->Globals, "step_over", ml_cfunctionx(Debugger, (void *)debugger_step_over));
	stringmap_insert(Debugger->Globals, "step_out", ml_cfunctionx(Debugger, (void *)debugger_step_out));
	stringmap_insert(Debugger->Globals, "locals", ml_cfunction(Debugger, (void *)debugger_locals));
	stringmap_insert(Debugger->Globals, "frames", ml_cfunction(Debugger, (void *)debugger_frames));
	stringmap_insert(Debugger->Globals, "frame_up", ml_cfunction(Debugger, (void *)debugger_frame_up));
	stringmap_insert(Debugger->Globals, "frame_down", ml_cfunction(Debugger, (void *)debugger_frame_down));
	stringmap_insert(Debugger->Globals, "threads", ml_cfunction(Debugger, (void *)debugger_threads));
	stringmap_insert(Debugger->Globals, "thread", ml_cfunction(Debugger, (void *)debugger_thread));

	ml_state_t *State = ml_state_new(Caller);
	ml_context_set(State->Context, ML_DEBUGGER_INDEX, Debugger);
	if (ml_is(Args[0], MLStringT)) {
		ml_call_state_t *State2 = ml_call_state_new(State, 1);
		ml_value_t *Args2 = ml_list();
		for (int I = 1; I < Count; ++I) ml_list_put(Args2, Args[I]);
		State2->Args[0] = Args2;
		const char *FileName = ml_string_value(Args[0]);
		ml_load_file((ml_state_t *)State2, Info->GlobalGet, Info->Globals, FileName, NULL);
	} else {
		ml_value_t *Function = Args[0];
		return ml_call(State, Function, Count - 1, Args + 1);
	}
}

ml_value_t *interactive_debugger(
	void (*enter)(void *Data, interactive_debugger_t *Debugger, ml_source_t Source, int Index),
	void (*exit)(void *Data, interactive_debugger_t *Debugger, ml_state_t *Caller, int Index),
	void (*log)(void *Data, ml_value_t *Value),
	void *Data,
	ml_getter_t GlobalGet,
	void *Globals
) {
	interactive_debugger_info_t *Info = new(interactive_debugger_info_t);
	Info->enter = enter;
	Info->exit = exit;
	Info->log = log;
	Info->Data = Data;
	Info->GlobalGet = GlobalGet;
	Info->Globals = Globals;
	return ml_cfunctionx(Info, (void *)interactive_debugger_fnx);
}
