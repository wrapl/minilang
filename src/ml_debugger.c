#include "ml_debugger.h"
#include "ml_runtime.h"
#include "ml_compiler.h"
#include "ml_macros.h"
#include "ml_logging.h"
#include "ml_json.h"
#include "ml_minijs.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>

#undef ML_CATEGORY
#define ML_CATEGORY "debugger"

typedef struct interactive_debugger_info_t interactive_debugger_info_t;

struct interactive_debugger_info_t {
	void (*enter)(void *Data, ml_interactive_debugger_t *Debugger, ml_source_t Source, int Index);
	void (*exit)(void *Data, ml_interactive_debugger_t *Debugger, ml_state_t *Caller, int Index);
	void (*log)(void *Data, ml_value_t *Value);
	void *Data;
	ml_getter_t GlobalGet;
	void *Globals;
};

typedef struct {
	ml_state_t *State, *Active;
	ml_value_t *Value;
} debug_thread_t;

typedef struct {
	size_t Count;
	size_t Bits[];
} breakpoints_t;

static size_t *stringmap_breakpoints(stringmap_t *Modules, const char *Source, int Max) {
	breakpoints_t **Slot = (breakpoints_t **)stringmap_slot(Modules, Source);
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

struct ml_interactive_debugger_t {
	ml_debugger_t Base;
	interactive_debugger_info_t *Info;
	debug_thread_t *Threads;
	debug_thread_t *ActiveThread;
	stringmap_t Globals[1];
	stringmap_t Modules[1];
	int MaxThreads, NumThreads;
};

static size_t *debugger_breakpoints(ml_interactive_debugger_t *Debugger, const char *Source, int Max) {
	return stringmap_breakpoints(Debugger->Modules, Source, Max);
}

ml_value_t *ml_interactive_debugger_get(ml_interactive_debugger_t *Debugger, const char *Name) {
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

ml_source_t ml_interactive_debugger_switch(ml_interactive_debugger_t *Debugger, int Index) {
	if (Index < 0 || Index >= Debugger->MaxThreads) return (ml_source_t){"Invalid thread", Index};
	debug_thread_t *Thread = Debugger->Threads + Index;
	if (!Thread) return (ml_source_t){"Invalid thread", Index};
	Debugger->ActiveThread = Thread;
	return ml_debugger_source(Thread->Active);
}

void ml_interactive_debugger_resume(ml_interactive_debugger_t *Debugger, int Index) {
	debug_thread_t *Thread;
	if (Index == -1) {
		Thread = Debugger->ActiveThread;
		Debugger->ActiveThread = NULL;
	} else if (0 <= Index && Index < Debugger->MaxThreads) {
		Thread = Debugger->Threads + Index;
	} else {
		return;
	}
	ml_state_t *State = Thread->State;
	if (State) {
		ml_value_t *Value = Thread->Value;
		Thread->Active = Thread->State = NULL;
		Thread->Value = NULL;
		--Debugger->NumThreads;
		return State->run(State, Value);
	}
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
	ml_map_insert(Result, ml_string(Module, -1), Lines);
	return 0;
}

static ml_value_t *debugger_breakpoints_list(ml_interactive_debugger_t *Debugger, int Count, ml_value_t **Args) {
	ml_value_t *Result = ml_map();
	stringmap_foreach(Debugger->Modules, Result, (void *)debugger_breakpoints_fn);
	return Result;
}

static ml_value_t *debugger_breakpoint_set(ml_interactive_debugger_t *Debugger, int Count, ml_value_t **Args) {
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

static ml_value_t *debugger_breakpoint_clear(ml_interactive_debugger_t *Debugger, int Count, ml_value_t **Args) {
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

static ml_value_t *debugger_breakpoint_toggle(ml_interactive_debugger_t *Debugger, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(2);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	ML_CHECK_ARG_TYPE(1, MLIntegerT);
	const char *Source = ml_string_value(Args[0]);
	size_t LineNo = ml_integer_value_fast(Args[1]);
	size_t *Breakpoints = debugger_breakpoints(Debugger, Source, LineNo);
	size_t Bit = 1L << (LineNo % SIZE_BITS);
	Breakpoints[LineNo / SIZE_BITS] ^= Bit;
	++Debugger->Base.Revision;
	return ml_integer(Breakpoints[LineNo / SIZE_BITS] & Bit);
}

static void debugger_continue(ml_state_t *Caller, ml_interactive_debugger_t *Debugger, int Count, ml_value_t **Args) {
	debug_thread_t *Thread = Debugger->ActiveThread;
	if (!Thread) return;
	int Index = Thread - Debugger->Threads;
	Debugger->Base.StepIn = 0;
	ml_debugger_step_mode(Thread->State, 0, 0);
	return Debugger->Info->exit(Debugger->Info->Data, Debugger, Caller, Index);
}

static void debugger_continue_all(ml_state_t *Caller, ml_interactive_debugger_t *Debugger, int Count, ml_value_t **Args) {
	debug_thread_t *Thread = Debugger->Threads;
	int MaxThreads = Debugger->MaxThreads;
	Debugger->Base.StepIn = 0;
	for (int I = 0; I < MaxThreads; ++I, ++Thread) {
		if (Thread->State) {
			ml_debugger_step_mode(Thread->State, 0, 0);
			Debugger->Info->exit(Debugger->Info->Data, Debugger, Caller, I);
		}
	}
}

static void debugger_step_in(ml_state_t *Caller, ml_interactive_debugger_t *Debugger, int Count, ml_value_t **Args) {
	debug_thread_t *Thread = Debugger->ActiveThread;
	if (!Thread) return;
	Debugger->Base.StepIn = 1;
	int Index = Thread - Debugger->Threads;
	ml_debugger_step_mode(Thread->State, 0, 0);
	return Debugger->Info->exit(Debugger->Info->Data, Debugger, Caller, Index);
}

static void debugger_step_over(ml_state_t *Caller, ml_interactive_debugger_t *Debugger, int Count, ml_value_t **Args) {
	debug_thread_t *Thread = Debugger->ActiveThread;
	if (!Thread) return;
	Debugger->Base.StepIn = 0;
	int Index = Thread - Debugger->Threads;
	ml_debugger_step_mode(Thread->State, 1, 0);
	return Debugger->Info->exit(Debugger->Info->Data, Debugger, Caller, Index);
}

static void debugger_step_out(ml_state_t *Caller, ml_interactive_debugger_t *Debugger, int Count, ml_value_t **Args) {
	debug_thread_t *Thread = Debugger->ActiveThread;
	if (!Thread) return;
	Debugger->Base.StepIn = 0;
	int Index = Thread - Debugger->Threads;
	ml_debugger_step_mode(Thread->State, 0, 1);
	return Debugger->Info->exit(Debugger->Info->Data, Debugger, Caller, Index);
}

static ml_value_t *debugger_locals(ml_interactive_debugger_t *Debugger, int Count, ml_value_t **Args) {
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

static ml_value_t *debugger_frames(ml_interactive_debugger_t *Debugger, int Count, ml_value_t **Args) {
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

static ml_value_t *debugger_frame_up(ml_interactive_debugger_t *Debugger, int Count, ml_value_t **Args) {
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

static ml_value_t *debugger_frame_down(ml_interactive_debugger_t *Debugger, int Count, ml_value_t **Args) {
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

static ml_value_t *debugger_threads(ml_interactive_debugger_t *Debugger, int Count, ml_value_t **Args) {
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

static ml_value_t *debugger_thread(ml_interactive_debugger_t *Debugger, int Count, ml_value_t **Args) {
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

static void debugger_run(ml_interactive_debugger_t *Debugger, ml_state_t *State, ml_value_t *Value) {
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

	ml_interactive_debugger_t *Debugger = new(ml_interactive_debugger_t);
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
	stringmap_insert(Debugger->Globals, "breakpoint_toggle", ml_cfunction(Debugger, (void *)debugger_breakpoint_toggle));
	stringmap_insert(Debugger->Globals, "breakpoints", ml_cfunction(Debugger, (void *)debugger_breakpoints_list));
	stringmap_insert(Debugger->Globals, "continue", ml_cfunctionx(Debugger, (void *)debugger_continue));
	stringmap_insert(Debugger->Globals, "continue_all", ml_cfunctionx(Debugger, (void *)debugger_continue_all));
	stringmap_insert(Debugger->Globals, "step_in", ml_cfunctionx(Debugger, (void *)debugger_step_in));
	stringmap_insert(Debugger->Globals, "step_over", ml_cfunctionx(Debugger, (void *)debugger_step_over));
	stringmap_insert(Debugger->Globals, "step_out", ml_cfunctionx(Debugger, (void *)debugger_step_out));
	stringmap_insert(Debugger->Globals, "locals", ml_cfunction(Debugger, (void *)debugger_locals));
	stringmap_insert(Debugger->Globals, "frames", ml_cfunction(Debugger, (void *)debugger_frames));
	stringmap_insert(Debugger->Globals, "frame_up", ml_cfunction(Debugger, (void *)debugger_frame_up));
	stringmap_insert(Debugger->Globals, "frame_down", ml_cfunction(Debugger, (void *)debugger_frame_down));
	stringmap_insert(Debugger->Globals, "threads", ml_cfunction(Debugger, (void *)debugger_threads));
	stringmap_insert(Debugger->Globals, "thread", ml_cfunction(Debugger, (void *)debugger_thread));

	ml_state_t *State = ml_state(Caller);
	ml_context_set_static(State->Context, ML_DEBUGGER_INDEX, Debugger);
	if (ml_is(Args[0], MLStringT)) {
		ml_call_state_t *State2 = ml_call_state(State, 1);
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

ml_value_t *ml_interactive_debugger(
	void (*enter)(void *Data, ml_interactive_debugger_t *Debugger, ml_source_t Source, int Index),
	void (*exit)(void *Data, ml_interactive_debugger_t *Debugger, ml_state_t *Caller, int Index),
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

typedef struct {
	ml_state_t Base;
	ml_debugger_t Debugger;
	debug_thread_t *Threads;
	json_decoder_t *Decoder;
	pthread_t Thread;
	stringmap_t Modules[1];
	ml_stringbuffer_t Buffer[1];
	int MaxThreads, NumThreads;
	int Socket;
} ml_remote_debugger_t;

typedef enum {
	ML_DEBUGGER_COMMAND_CONFIG = 0,
	ML_DEBUGGER_COMMAND_BREAKPOINT_SET = 1,
	ML_DEBUGGER_COMMAND_BREAKPOINT_CLEAR = 2,
	ML_DEBUGGER_COMMAND_BREAKPOINTS = 3,
	ML_DEBUGGER_COMMAND_CONTINUE_ALL = 4,
	ML_DEBUGGER_COMMAND_CONTINUE = 5,
	ML_DEBUGGER_COMMAND_STEP_IN = 6,
	ML_DEBUGGER_COMMAND_STEP_OVER = 7,
	ML_DEBUGGER_COMMAND_STEP_OUT = 8,
	ML_DEBUGGER_COMMAND_LOCALS = 9,
	ML_DEBUGGER_COMMAND_FRAMES = 10,
	ML_DEBUGGER_COMMAND_THREADS = 11,
	ML_DEBUGGER_COMMAND_EVALUATE = 12
} ml_debugger_command_t;

typedef enum {
	ML_DEBUGGER_CONFIG_BREAK_ON_ERROR = 0
} ml_debugger_config_t;

typedef enum {
	ML_DEBUGGER_EVENT_READY = 0,
	ML_DEBUGGER_EVENT_BREAK = 1,
	ML_DEBUGGER_EVENT_ERROR = 2,
	ML_DEBUGGER_EVENT_HALT = 3
} ml_debugger_event_t;

static int ml_remote_debugger_send(ml_remote_debugger_t *Remote, const char *Buffer, size_t Size) {
	fprintf(stderr, "< %.*s\n", (int)Size, Buffer);
	while (Size) {
		ssize_t Write = write(Remote->Socket, Buffer, Size);
		if (Write < 0) return Write;
		Buffer += Write;
		Size -= Write;
	}
	return 0;
}

static size_t *ml_remote_debugger_breakpoints(ml_debugger_t *Debugger, const char *Source, int Max) {
	ml_remote_debugger_t *Remote = (ml_remote_debugger_t *)((void *)Debugger - offsetof(ml_remote_debugger_t, Debugger));
	return stringmap_breakpoints(Remote->Modules, Source, Max);
}

static void ml_remote_debugger_command(ml_remote_debugger_t *Remote, ml_value_t *Command) {
	size_t Index = ml_integer_value(ml_list_pop(Command));
	ml_stringbuffer_printf(Remote->Buffer, "[%ld,", Index);
	ml_value_t *Result = (ml_value_t *)MLTrue;
	ml_debugger_command_t Cmd = ml_integer_value(ml_list_pop(Command));
	switch (Cmd) {
	case ML_DEBUGGER_COMMAND_CONFIG: {
		ml_debugger_config_t Config = ml_integer_value(ml_list_pop(Command));
		switch (Config) {
		case ML_DEBUGGER_CONFIG_BREAK_ON_ERROR: {
			Remote->Debugger.BreakOnError = ml_list_pop(Command) == (ml_value_t *)MLTrue;
			break;
		}
		}
		break;
	}
	case ML_DEBUGGER_COMMAND_BREAKPOINT_SET: {
		const char *Source = ml_string_value(ml_list_pop(Command));
		while (ml_list_length(Command)) {
			size_t LineNo = ml_integer_value_fast(ml_list_pop(Command));
			size_t *Breakpoints = stringmap_breakpoints(Remote->Modules, Source, LineNo);
			Breakpoints[LineNo / SIZE_BITS] |= 1L << (LineNo % SIZE_BITS);
		}
		++Remote->Debugger.Revision;
		break;
	}
	case ML_DEBUGGER_COMMAND_BREAKPOINT_CLEAR: {
		const char *Source = ml_string_value(ml_list_pop(Command));
		while (ml_list_length(Command)) {
			size_t LineNo = ml_integer_value_fast(ml_list_pop(Command));
			size_t *Breakpoints = stringmap_breakpoints(Remote->Modules, Source, LineNo);
			Breakpoints[LineNo / SIZE_BITS] &= ~(1L << (LineNo % SIZE_BITS));
		}
		++Remote->Debugger.Revision;
		break;
	}
	case ML_DEBUGGER_COMMAND_BREAKPOINTS: {
		Result = ml_map();
		stringmap_foreach(Remote->Modules, Result, (void *)debugger_breakpoints_fn);
		break;
	}
	case ML_DEBUGGER_COMMAND_CONTINUE_ALL: {
		Remote->Debugger.StepIn = 0;
		debug_thread_t *Thread = Remote->Threads;
		int MaxThreads = Remote->MaxThreads;
		Remote->Debugger.StepIn = 0;
		for (int I = 0; I < MaxThreads; ++I, ++Thread) {
			ml_state_t *State = Thread->State;
			if (State) {
				ml_value_t *Value = Thread->Value;
				ml_debugger_step_mode(State, 0, 0);
				Thread->Active = Thread->State = NULL;
				Thread->Value = NULL;
				--Remote->NumThreads;
				ml_state_schedule(State, Value);
			}
		}
		break;
	}
	case ML_DEBUGGER_COMMAND_CONTINUE: {
		int Index = ml_integer_value(ml_list_pop(Command));
		if (0 <= Index && Index < Remote->MaxThreads) {
			debug_thread_t *Thread = Remote->Threads + Index;
			Remote->Debugger.StepIn = 0;
			ml_state_t *State = Thread->State;
			ml_value_t *Value = Thread->Value;
			ml_debugger_step_mode(State, 0, 0);
			Thread->Active = Thread->State = NULL;
			Thread->Value = NULL;
			--Remote->NumThreads;
			ml_state_schedule(State, Value);
		} else {
			Result = (ml_value_t *)MLFalse;
		}
		break;
	}
	case ML_DEBUGGER_COMMAND_STEP_IN: {
		int Index = ml_integer_value(ml_list_pop(Command));
		if (0 <= Index && Index < Remote->MaxThreads) {
			debug_thread_t *Thread = Remote->Threads + Index;
			Remote->Debugger.StepIn = 1;
			ml_state_t *State = Thread->State;
			ml_value_t *Value = Thread->Value;
			ml_debugger_step_mode(State, 0, 0);
			Thread->Active = Thread->State = NULL;
			Thread->Value = NULL;
			--Remote->NumThreads;
			ml_state_schedule(State, Value);
		} else {
			Result = (ml_value_t *)MLFalse;
		}
		break;
	}
	case ML_DEBUGGER_COMMAND_STEP_OVER: {
		int Index = ml_integer_value(ml_list_pop(Command));
		if (0 <= Index && Index < Remote->MaxThreads) {
			debug_thread_t *Thread = Remote->Threads + Index;
			Remote->Debugger.StepIn = 0;
			ml_state_t *State = Thread->State;
			ml_value_t *Value = Thread->Value;
			ml_debugger_step_mode(State, 1, 0);
			Thread->Active = Thread->State = NULL;
			Thread->Value = NULL;
			--Remote->NumThreads;
			ml_state_schedule(State, Value);
		} else {
			Result = (ml_value_t *)MLFalse;
		}
		break;
	}
	case ML_DEBUGGER_COMMAND_STEP_OUT: {
		int Index = ml_integer_value(ml_list_pop(Command));
		if (0 <= Index && Index < Remote->MaxThreads) {
			debug_thread_t *Thread = Remote->Threads + Index;
			Remote->Debugger.StepIn = 0;
			ml_state_t *State = Thread->State;
			ml_value_t *Value = Thread->Value;
			ml_debugger_step_mode(State, 0, 1);
			Thread->Active = Thread->State = NULL;
			Thread->Value = NULL;
			--Remote->NumThreads;
			ml_state_schedule(State, Value);
		} else {
			Result = (ml_value_t *)MLFalse;
		}
		break;
	}
	case ML_DEBUGGER_COMMAND_LOCALS: {
		Result = ml_list();
		int Index = ml_integer_value(ml_list_pop(Command));
		if (0 <= Index && Index < Remote->MaxThreads) {
			debug_thread_t *Thread = Remote->Threads + Index;
			ml_state_t *Frame = Thread->State;
			int Depth = ml_integer_value(ml_list_pop(Command));
			for (;;) {
				while (!ml_debugger_check(Frame)) {
					Frame = Frame->Caller;
					if (!Frame) goto invalid;
				}
				if (--Depth < 0) break;
				Frame = Frame->Caller;
				if (!Frame) goto invalid;
			}
			ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
			for (ml_decl_t *Decl = ml_debugger_decls(Frame); Decl; Decl = Decl->Next) {
				ml_value_t *Value = Decl->Value ?: ml_debugger_local(Frame, Decl->Index);
				if (!Value) continue;
				ml_value_t *Variable = ml_list();
				ml_list_put(Variable, ml_string(Decl->Ident, -1));
				ml_value_t *Error = ml_stringbuffer_simple_append(Buffer, Value);
				if (ml_is_error(Error)) {
					ml_stringbuffer_clear(Buffer);
					ml_stringbuffer_printf(Buffer, "<%s>", ml_typeof(Value)->Name);
				}
				ml_list_put(Variable, ml_stringbuffer_get_value(Buffer));
				ml_list_put(Variable, ml_integer(Decl->Source.Line));
				ml_list_put(Result, Variable);
			}
		}
		invalid:;
		break;
	}
	case ML_DEBUGGER_COMMAND_FRAMES: {
		Result = ml_list();
		int Index = ml_integer_value(ml_list_pop(Command));
		if (0 <= Index && Index < Remote->MaxThreads) {
			debug_thread_t *Thread = Remote->Threads + Index;
			ml_state_t *Frame = Thread->State;
			while (Frame) {
				if (ml_debugger_check(Frame)) {
					ml_source_t Source = ml_debugger_source(Frame);
					ml_value_t *Location = ml_list();
					ml_list_put(Location, ml_string(Source.Name, -1));
					ml_list_put(Location, ml_integer(Source.Line));
					ml_list_put(Result, Location);
				}
				Frame = Frame->Caller;
			}
		}
		break;
	}
	case ML_DEBUGGER_COMMAND_THREADS: {
		Result = ml_list();
		debug_thread_t *Thread = Remote->Threads;
		for (int I = 0; I < Remote->MaxThreads; ++I, ++Thread) {
			if (Thread->State) {
				ml_source_t Source = ml_debugger_source(Thread->State);
				ml_value_t *Location = ml_list();
				ml_list_put(Location, ml_integer(I));
				ml_list_put(Location, ml_string(Source.Name, -1));
				ml_list_put(Location, ml_integer(Source.Line));
				ml_list_put(Result, Location);
			}
		}
		break;
	}
	case ML_DEBUGGER_COMMAND_EVALUATE: {
		break;
	}
	}
	ml_value_t *Error = ml_json_encode(Remote->Buffer, Result);
	if (Error) ML_LOG_ERROR(Error, "Error encoding response to JSON");
	ml_stringbuffer_write(Remote->Buffer, "]\n", strlen("]\n"));
	ml_stringbuffer_drain(Remote->Buffer, Remote, (void *)ml_remote_debugger_send);
}

static void ml_remote_debugger_run(ml_debugger_t *Debugger, ml_state_t *State, ml_value_t *Value) {
	ml_remote_debugger_t *Remote = (ml_remote_debugger_t *)((void *)Debugger - offsetof(ml_remote_debugger_t, Debugger));
	if (Remote->NumThreads == Remote->MaxThreads) {
		int MaxThreads = 2 * Remote->MaxThreads;
		debug_thread_t *Threads = anew(debug_thread_t, MaxThreads);
		memcpy(Threads, Remote->Threads, Remote->MaxThreads * sizeof(debug_thread_t));
		Remote->Threads = Threads;
		Remote->MaxThreads = MaxThreads;
	}
	++Remote->NumThreads;
	debug_thread_t *Thread = Remote->Threads;
	while (Thread->State) ++Thread;
	Thread->State = State;
	Thread->Value = Value;
	int Index = Thread - Remote->Threads;
	if (ml_is_error(Value)) {
		ml_stringbuffer_printf(Remote->Buffer, "[null,%d,%d,\"", ML_DEBUGGER_EVENT_ERROR, Index);
		ml_stringbuffer_escape_string(Remote->Buffer, ml_error_type(Value), -1);
		ml_stringbuffer_write(Remote->Buffer, "\",\"", strlen("\",\""));
		ml_stringbuffer_escape_string(Remote->Buffer, ml_error_message(Value), -1);
		ml_stringbuffer_write(Remote->Buffer, "\"]\n", strlen("\"]\n"));
	} else {
		ml_stringbuffer_printf(Remote->Buffer, "[null,%d,%d]\n", ML_DEBUGGER_EVENT_BREAK, Index);;
	}
	ml_stringbuffer_drain(Remote->Buffer, Remote, (void *)ml_remote_debugger_send);
}

static void ml_remote_debugger_json(json_decoder_t *Decoder, ml_value_t *Value) {
	ml_state_t *Remote = json_decoder_data(Decoder);
	ml_state_schedule(Remote, Value);
}

static void *ml_remote_debugger_fn(void *Arg) {
	ml_remote_debugger_t *Remote = (ml_remote_debugger_t *)Arg;
	char Buffer[64];
	for (;;) {
		ssize_t Read = read(Remote->Socket, Buffer, 64);
		if (Read < 0) return NULL;
		fprintf(stderr, "> %.*s\n", (int)Read, Buffer);
		if (Read == 0) return NULL;
		json_decoder_parse(Remote->Decoder, Buffer, Read);
	}
	exit(0);
	return NULL;
}

void ml_remote_debugger_init(ml_context_t *Context, const char *Address) {
	const char *Port = strchr(Address, ':');
	int Socket;
	if (Port) {
		ML_LOG_FATAL(NULL, "Not implemented yet!");
		exit(-1);
	} else {
		Socket = socket(PF_UNIX, SOCK_STREAM, 0);
		struct sockaddr_un Name;
		Name.sun_family = AF_LOCAL;
		strncpy(Name.sun_path, Address, sizeof(Name.sun_path));
		Name.sun_path[sizeof(Name.sun_path) - 1] = 0;
		if (connect(Socket, (struct sockaddr *)&Name, SUN_LEN(&Name)) < 0) {
			ML_LOG_FATAL(NULL, "Error connecting to %s: %s", Address, strerror(errno));
			exit(-1);
		}
	}
	ml_remote_debugger_t *Remote = new(ml_remote_debugger_t);
	Remote->Base.run = (void *)ml_remote_debugger_command;
	Remote->Base.Context = Context;
	Remote->Debugger.run = (void *)ml_remote_debugger_run;
	Remote->Debugger.breakpoints = (void *)ml_remote_debugger_breakpoints;
	Remote->Debugger.Revision = 1;
	Remote->Debugger.StepIn = 1;
	Remote->Debugger.BreakOnError = 1;
	Remote->MaxThreads = 16;
	Remote->NumThreads = 0;
	Remote->Threads = anew(debug_thread_t, 16);
	Remote->Socket = Socket;
	Remote->Decoder = json_decoder(ml_remote_debugger_json, Remote);
	Remote->Buffer[0] = ML_STRINGBUFFER_INIT;
	ml_stringbuffer_printf(Remote->Buffer, "[null,0,%d]\n", ML_DEBUGGER_EVENT_READY);
	ml_stringbuffer_drain(Remote->Buffer, Remote, (void *)ml_remote_debugger_send);
	GC_pthread_create(&Remote->Thread, NULL, ml_remote_debugger_fn, Remote);
	ml_context_set_static(Context, ML_DEBUGGER_INDEX, &Remote->Debugger);
}
