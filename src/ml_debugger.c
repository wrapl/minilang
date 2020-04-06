#include "ml_debugger.h"
#include "ml_macros.h"

#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <jansson.h>
#include "stringmap.h"
#include "ml_bytecode.h"

const int BREAKPOINT_BUFFER_SIZE = 128;

typedef struct debug_local_t debug_local_t;
typedef struct debug_global_t debug_global_t;
typedef struct ml_frame_debug_t ml_frame_debug_t;

struct debugger_t {
	const ml_type_t *Type;
	pthread_mutex_t Lock[1];
	debug_module_t *Modules;
	stringmap_t Commands[1];
	int Socket;
	int NextModuleId;
	const char *SocketPath;
	const char *EvaluateBuffer;
	ml_frame_debug_t *EvaluateState;
	int BreakOnSend, BreakOnMessage;
};

struct debug_global_t {
	debug_global_t *Next;
	ml_value_t **Address;
	ml_value_t *Value;
	const char *Name;
};

struct debug_module_t {
	debug_module_t *Next;
	int Id, NumBreakpointBuffers;
	unsigned char **BreakpointBuffers;
	const char *Name;
	debug_global_t *Globals;
};

struct debug_local_t {
	debug_local_t *Next;
	int Index;
	ml_value_t *Value;
	const char *Name;
};

struct debug_function_t {
	debug_module_t *Module;
	int LineNo, LocalsOffset, NoOfLocals;
	debug_local_t *Locals;
};

typedef json_t *debugger_command_func(json_t *);

static debug_module_t *debugger_get_module(json_t *Args) {
	int Id;
	if (json_unpack(Args, "{si}", "module", &Id)) return 0;
	for (debug_module_t *Module = Debugger->Modules; Module; Module = Module->Next) {
		if (Module->Id == Id) return Module;
	}
	return 0;
}

static int StepIn = 0;
static int RunTo = 0;
static ml_frame_debug_t *StepOverInstance = NULL;
static ml_frame_debug_t *StepOutInstance = NULL;
static int Paused = 0;
static pthread_cond_t *Resume;
static ml_frame_debug_t *State;
static int Enters = 0;
static int Exits = 0;

static json_t *debugger_command_pause(debugger_t *Debugger, json_t *Args) {
	StepIn = 1;
	RunTo = 0;
	StepOverInstance = NULL;
	StepOutInstance = NULL;
	if (Paused) {
		Paused = 0;
		pthread_cond_signal(Resume);
	}
	return json_true();
}

static json_t *debugger_command_continue(debugger_t *Debugger, json_t *Args) {
	StepIn = 0;
	RunTo = 0;
	StepOverInstance = NULL;
	StepOutInstance = NULL;
	if (Paused) {
		Paused = 0;
		pthread_cond_signal(Resume);
	}
	return json_true();
}

static json_t *debugger_command_step_in(debugger_t *Debugger, json_t *Args) {
	StepIn = 1;
	RunTo = 0;
	StepOverInstance = NULL;
	StepOutInstance = NULL;
	if (Paused) {
		Paused = 0;
		pthread_cond_signal(Resume);
	}
	return json_true();
}

static json_t *debugger_command_step_out(debugger_t *Debugger, json_t *Args) {
	if (Paused) {
		StepIn = 0;
		RunTo = 0;
		StepOverInstance = 0;
		StepOutInstance = State;
		Paused = 0;
		pthread_cond_signal(Resume);
	}
	return json_true();
}

static json_t *debugger_command_step_over(debugger_t *Debugger, json_t *Args) {
	if (Paused) {
		StepIn = 0;
		RunTo = 0;
		StepOverInstance = State;
		StepOutInstance = State;
		Paused = 0;
		pthread_cond_signal(Resume);
	}
	return json_true();
}

static json_t *debugger_command_run_to(debugger_t *Debugger, json_t *Args) {
	debug_module_t *Module = debugger_get_module(Args);
	if (!Module) return json_pack("{ss}", "error", "invalid module");
	int Line;
	if (json_unpack(Args, "{si}", "line", &Line)) return json_pack("{ss}", "error", "invalid arguments");
	if (Paused) {
		StepIn = 0;
		RunTo = Module->Id << 16 + Line;
		StepOverInstance = 0;
		StepOutInstance = 0;
		Paused = 0;
		pthread_cond_signal(Resume);
	}
	return json_true();
}

static json_t *debugger_command_list_modules(debugger_t *Debugger, json_t *Command) {
	return json_true();
}

static void debugger_describe_variable(json_t *Result, ml_value_t *Value) {
	ml_type_t *Type = Value->Type;
	if (Value == MLNil) {
		json_object_set(Result, "type", json_string("nil"));
		json_object_set(Result, "value", json_null());
	} else if (Type == MLIntegerT) {
		json_object_set(Result, "type", json_string("integer"));
		json_object_set(Result, "value", json_integer(ml_integer_value(Value)));
	} else if (Type == MLRealT) {
		json_object_set(Result, "type", json_string("real"));
		json_object_set(Result, "value", json_real(ml_real_value(Value)));
	} else if (Type == MLStringT) {
		json_object_set(Result, "type", json_string("string"));
		json_object_set(Result, "value", json_string(ml_string_value(Value)));
	} else if (Type == MLMethodT) {
		json_object_set(Result, "type", json_string("symbol"));
		json_object_set(Result, "value", json_string(ml_method_name(Value)));
	} else if (Type == MLMapT) {
		json_object_set(Result, "type", json_string("table"));
		json_object_set(Result, "ref", json_real((double)(size_t)Value));
		json_object_set(Result, "size", json_integer(2 * ml_map_size(Value)));
	} else if (Type == MLListT) {
		json_object_set(Result, "type", json_string("list"));
		json_object_set(Result, "ref", json_real((double)(size_t)Value));
		json_object_set(Result, "size", json_integer(ml_list_length(Value)));
	} else {
		/*json_object_set(Result, "type", json_string("object"));
		char *Description;
		const char *ModuleName, *SymbolName;
		if (!Riva$Module$lookup((void *)Value->Type, &ModuleName, &SymbolName)) {
			json_object_set(Result, "value", json_string("object"));
		} else {
			const char *ShortName = strrchr(ModuleName, '/');
			if (ShortName) {
				++ShortName;
			} else {
				ShortName = ModuleName;
			}
			asprintf(&Description, "%s.%s", ShortName, SymbolName);
			json_object_set(Result, "value", json_string(Description));
		}
		const Std$Array$t *Fields = Value->Type->Fields;
		json_object_set(Result, "ref", json_real((double)(size_t)Value));
		json_object_set(Result, "size", json_integer(Fields->Length.Value));*/
	}
}

static void debugger_append_variable(json_t *Variables, const char *Name, ml_value_t *Value) {
	json_t *Description = json_pack("{ss}", "name", Name);
	debugger_describe_variable(Description, Value);
	json_array_append(Variables, Description);
}

static json_t *debugger_command_get_variable(debugger_t *Debugger, json_t *Args) {
	json_t *Variables = json_array();
	if (json_object_get(Args, "state")) {
		ml_frame_debug_t *State = (ml_frame_debug_t *)(size_t)json_number_value(json_object_get(Args, "state"));
		debug_function_t *Function = State->Debug;
		ml_value_t ***Locals = (void *)State + Function->LocalsOffset;
		for (debug_local_t *Local = State->Debug->Locals; Local; Local = Local->Next) {
			if (Local->Value) {
				debugger_append_variable(Variables, Local->Name, Local->Value);
			} else {
				ml_value_t **Ref = Locals[Local->Index];
				if (Ref) {
					debugger_append_variable(Variables, Local->Name, *Ref);
				} else {
					json_array_append(Variables, json_pack("{ssss}", "name", Local->Name, "type", "none"));
				}
			}
		}
		return Variables;
	}
	if (json_object_get(Args, "module")) {
		debug_module_t *Module = debugger_get_module(Args);
		if (!Module) return json_pack("{ss}", "error", "invalid module");
		for (debug_global_t *Global = Module->Globals; Global; Global = Global->Next) {
			if (Global->Value) {
				debugger_append_variable(Variables, Global->Name, Global->Value);
			} else {
				ml_value_t *Value = Global->Address[0];
				if (Value) {
					debugger_append_variable(Variables, Global->Name, Value);
				} else {
					json_array_append(Variables, json_pack("{ssss}", "name", Global->Name, "type", "none"));
				}
			}
		}
		return Variables;
	}
	ml_value_t *Value = (ml_value_t *)(size_t)json_number_value(json_object_get(Args, "ref"));
	ml_type_t *Type = Value->Type;
	if (Type == MLMapT) {
		ML_MAP_FOREACH(Value, Node) {
			char *KeyName, *ValueName;
			asprintf(&KeyName, "key%d", json_array_size(Variables) / 2 + 1);
			asprintf(&ValueName, "value%d", json_array_size(Variables) / 2 + 1);
			debugger_append_variable(Variables, KeyName, Node->Key);
			debugger_append_variable(Variables, ValueName, Node->Value);
		}
	} else if (Type == MLListT) {
		int Index = 0;
		ML_LIST_FOREACH(Value, Node) {
			char *Name;
			asprintf(&Name, "%d", ++Index);
			debugger_append_variable(Variables, Name, Node->Value);
		}
	} else {
		/*const Std$Array$t *Fields = Value->Type->Fields;
		for (int I = 0; I < Fields->Length.Value; ++I) {
			debugger_append_variable(Variables, ml_string_value((ml_value_t *)Std$Symbol$get_name(Fields->Values[I])), *((ml_value_t **)Value + I + 1));
		}*/
	}
	return Variables;
}

static unsigned char *debug_module_breakpoints(debug_module_t *Module, int LineNo) {
	int Index = LineNo / BREAKPOINT_BUFFER_SIZE;
	if (Index >= Module->NumBreakpointBuffers) {
		unsigned char **BreakpointBuffers = (void **)snew((Index + 1) * sizeof(void *));
		for (int I = 0; I < Module->NumBreakpointBuffers; ++I) BreakpointBuffers[I] = Module->BreakpointBuffers[I];
		Module->BreakpointBuffers = BreakpointBuffers;
		Module->NumBreakpointBuffers = Index + 1;
	}
	if (Module->BreakpointBuffers[Index]) return Module->BreakpointBuffers[Index] + (LineNo % BREAKPOINT_BUFFER_SIZE) / 8;
	unsigned char *BreakpointBuffer = (unsigned char *)snew(BREAKPOINT_BUFFER_SIZE / 8);
	memset(BreakpointBuffer, 0, BREAKPOINT_BUFFER_SIZE / 8);
	Module->BreakpointBuffers[Index] = BreakpointBuffer;
	return BreakpointBuffer + (LineNo % BREAKPOINT_BUFFER_SIZE) / 8;
}

static json_t *debugger_command_set_breakpoints(debugger_t *Debugger, json_t *Args) {
	debug_module_t *Module = debugger_get_module(Args);
	if (!Module) return json_pack("{ss}", "error", "invalid module");
	json_t *Enable = json_object_get(Args, "enable");
	json_t *Disable = json_object_get(Args, "disable");
	if (!Enable || !Disable) return json_pack("{ss}", "error", "invalid arguments");
	for (int I = 0; I < json_array_size(Enable); ++I) {
		int LineNo = json_integer_value(json_array_get(Enable, I));
		debug_module_breakpoints(Module, LineNo)[0] |= 1 << (LineNo % 8);
	}
	for (int I = 0; I < json_array_size(Disable); ++I) {
		int LineNo = json_integer_value(json_array_get(Disable, I));
		debug_module_breakpoints(Module, LineNo)[0] &= ~(1 << (LineNo % 8));
	}
	return json_true();
}

unsigned char *debug_break_on_send() {
	return (unsigned char *)&Debugger->BreakOnSend;
}

unsigned char *debug_break_on_message() {
	return (unsigned char *)&Debugger->BreakOnMessage;
}

static json_t *debugger_command_set_message_breakpoints(debugger_t *Debugger, json_t *Args) {
	int BreakOnSend = 0;
	int BreakOnMessage = 0;
	for (int I = 0; I < json_array_size(Args); ++I) {
		const char *Type = json_string_value(json_array_get(Args, I));
		if (!strcmp(Type, "send")) BreakOnSend = -1;
		if (!strcmp(Type, "message")) BreakOnMessage = -1;
	}
	Debugger->BreakOnSend = BreakOnSend;
	Debugger->BreakOnMessage = BreakOnMessage;
	return json_true();
}

/*LOCAL_FUNCTION(DebuggerIDFunc) {
	const char *Name = ml_string_value(Args[0].Val);
	printf("Looking up variable %s\n", Name);
	if (!Debugger->EvaluateState) return FAILURE;
	debug_function_t *Function = Debugger->EvaluateState->Function;
	if (!Function) return FAILURE;
	ml_value_t ***Locals = (void *)Debugger->EvaluateState + Function->LocalsOffset;
	for (debug_local_t *Local = Function->Locals; Local; Local = Local->Next) {
		if (!strcmp(Local->Name, Name)) {
			if (Local->Value) {
				Result->Val = Local->Value;
				return SUCCESS;
			}
			ml_value_t **Ref = Locals[Local->Index];
			if (Ref) {
				Result->Val = Ref[0];
				Result->Ref = Ref;
				return SUCCESS;
			}
		}
	}
	debug_module_t *Module = Function->Module;
	for (debug_global_t *Global = Module->Globals; Global; Global = Global->Next) {
		if (!strcmp(Global->Name, Name)) {
			if (Global->Value) {
				Result->Val = Global->Value;
				return SUCCESS;
			}
			Result->Val = Global->Address[0];
			return SUCCESS;
		}
	}
	return FAILURE;
}*/

static json_t *debugger_command_evaluate(debugger_t *Debugger, json_t *Args) {
	if (!Paused) return json_pack("{ss}", "error", "invalid thread");
	if (json_unpack(Args, "{ss}", "expression", &Debugger->EvaluateBuffer)) return json_pack("{ss}", "error", "missing expression");
	if (json_object_get(Args, "state")) {
		Debugger->EvaluateState = (ml_frame_debug_t *)(size_t)json_number_value(json_object_get(Args, "state"));
	} else {
		Debugger->EvaluateState = State;
	}
	/*if (setjmp(Debugger->Scanner->Error.Handler)) {
		Debugger->Scanner->flush();
		Debugger->EvaluateState = 0;
		return json_pack("{sssi}", "error", Debugger->Scanner->Error.Message, "line", Debugger->Scanner->Error.LineNo);
	}
	command_expr_t *Command = accept_command(Debugger->Scanner);
#ifdef PARSER_LISTING
	Command->print(0);
#endif
	if (setjmp(Debugger->Compiler->Error.Handler)) {
		Debugger->Compiler->flush();
		Debugger->EvaluateState = 0;
		return json_pack("{sssi}", "error", Debugger->Compiler->Error.Message, "line", Debugger->Compiler->Error.LineNo);
	}
	Std$Function$result Result[1];
	switch (Command->compile(Debugger->Compiler, Result)) {
	case SUSPEND: case SUCCESS: {
		json_t *Output = json_object();
		debugger_describe_variable(Output, Result->Val);
		Debugger->EvaluateState = 0;
		return Output;
	}
	case FAILURE: {
		Debugger->EvaluateState = 0;
		return json_pack("{sb}", "failed", 1);
	}
	case MESSAGE: {
		json_t *Output = json_pack("{sb}", "message", 1);
		debugger_describe_variable(Output, Result->Val);
		Debugger->EvaluateState = 0;
		return Output;
	}
	}*/
	return json_pack("{sb}", "failed", 1);
}

static void debugger_update(json_t *UpdateJson) {
	json_dumpfd(UpdateJson, Debugger->Socket, JSON_COMPACT);
	write(Debugger->Socket, "\n", strlen("\n"));
	//printf("Update: %s\n", json_dumps(UpdateJson, JSON_INDENT(4)));
}

static void *debugger_thread_func(debugger_t *Debugger) {
	json_t *CommandJson = json_pack("[is]", 0, "ready");
	json_error_t JsonError;
	debugger_update(CommandJson);
	while ((CommandJson = json_loadfd(Debugger->Socket, JSON_DISABLE_EOF_CHECK, &JsonError))) {
		int Index;
		const char *Command;
		json_t *Args;
		json_t *ResultJson;
		pthread_mutex_lock(Debugger->Lock);
			//printf("Received: %s\n", json_dumps(CommandJson, JSON_INDENT(4)));
			if (!json_unpack(CommandJson, "[iso]", &Index, &Command, &Args)) {
				debugger_command_func *CommandFunc = (debugger_command_func *)stringmap_search(Debugger->Commands, Command);
				if (CommandFunc) {
					ResultJson = json_pack("[io?]", Index, CommandFunc(Args));
				} else {
					ResultJson = json_pack("[i{ss}]", Index, "error", "invalid command");
				}
			} else {
				ResultJson = json_pack("[i{sssssi}]", Index, "error", "parse error", "message", JsonError.text, "position", JsonError.position);
			}
			json_dumpfd(ResultJson, Debugger->Socket, JSON_COMPACT);
			write(Debugger->Socket, "\n", strlen("\n"));
			//printf("Reply: %s\n", json_dumps(ResultJson, JSON_INDENT(4)));
		pthread_mutex_unlock(Debugger->Lock);
	}
	// For now, just kill the program once the debugger disconnects
	exit(1);
}

debug_module_t *debug_module(const char *Name) {
	debug_module_t *Module = new(debug_module_t);
	Module->Name = Name;
	pthread_mutex_lock(Debugger->Lock);
		Module->Id = ++Debugger->NextModuleId;
		Module->Next = Debugger->Modules;
		Debugger->Modules = Module;
		debugger_update(json_pack("[is{siss}]", 0, "module_add", "module", Module->Id, "name", Name));
		Paused = 1;
		do pthread_cond_wait(Resume, Debugger->Lock); while (Paused);
	pthread_mutex_unlock(Debugger->Lock);
	return Module;
}

unsigned char *debug_breakpoints(debug_function_t *Function, int LineNo) {
	return debug_module_breakpoints(Function->Module, LineNo);
}

void debug_add_line(debug_module_t *Module, const char *Line) {
	pthread_mutex_lock(Debugger->Lock);
		debugger_update(json_pack("[is{siss}]", 0, "line_add", "module", Module->Id, "line", Line));
	pthread_mutex_unlock(Debugger->Lock);
}

void debug_add_global_variable(debug_module_t *Module, const char *Name, ml_value_t **Address) {
	debug_global_t *Global = new(debug_global_t);
	Global->Name = Name;
	Global->Address = Address;
	debug_global_t **Slot = &Module->Globals;
	while (*Slot) Slot = &Slot[0]->Next;
	*Slot = Global;
	pthread_mutex_lock(Debugger->Lock);
		debugger_update(json_pack("[is{siss}]", 0, "global_add", "module", Module->Id, "name", Name));
	pthread_mutex_unlock(Debugger->Lock);
}

void debug_add_global_constant(debug_module_t *Module, const char *Name, ml_value_t *Value) {
	debug_global_t *Global = new(debug_global_t);
	Global->Name = Name;
	Global->Value = Value;
	debug_global_t **Slot = &Module->Globals;
	while (*Slot) Slot = &Slot[0]->Next;
	*Slot = Global;
	pthread_mutex_lock(Debugger->Lock);
		debugger_update(json_pack("[is{siss}]", 0, "global_add", "module", Module->Id, "name", Name));
	pthread_mutex_unlock(Debugger->Lock);
}

debug_function_t *debug_function(debug_module_t *Module, int LineNo) {
	debug_function_t *Function = new(debug_function_t);
	Function->Module = Module;
	Function->LineNo = LineNo;
	return Function;
}

int debug_module_id(debug_function_t *Function) {
	return Function->Module->Id;
}

void debug_add_local_var(debug_function_t *Function, const char *Name, int Index) {
	debug_local_t *Local = new(debug_local_t);
	Local->Name = Name;
	Local->Index = Index;
	debug_local_t **Slot = &Function->Locals;
	while (*Slot) Slot = &Slot[0]->Next;
	*Slot = Local;
}

void debug_add_local_def(debug_function_t *Function, const char *Name, ml_value_t *Value) {
	debug_local_t *Local = new(debug_local_t);
	Local->Name = Name;
	Local->Value = Value;
	debug_local_t **Slot = &Function->Locals;
	while (*Slot) Slot = &Slot[0]->Next;
	*Slot = Local;
}

void debug_set_locals(debug_function_t *Function, int LocalsOffset, int NoOfLocals) {
	Function->LocalsOffset = LocalsOffset;
	Function->NoOfLocals = NoOfLocals;
}

debugger_t *Debugger = 0;

void debug_break_impl(ml_frame_debug_t *State, int LineNo) {
	pthread_mutex_lock(Debugger->Lock);
		Paused = 1;
		json_t *States = json_array();
		ml_frame_debug_t *EnterState = State;
		for (int I = 0; I < Enters; ++I) {
			debug_function_t *Function = EnterState->Debug;
			json_array_insert_new(States, 0, json_pack("{sisisfsi}",
				"module", Function->Module->Id,
				"line", Function->LineNo,
				"state", (double)(size_t)EnterState,
				"size", Function->NoOfLocals
			));
			EnterState = EnterState->UpState;
		}
		debugger_update(json_pack("[is{sisisO}]", 0, "break",
			"exits", Exits,
			"line", LineNo,
			"enters", States
		));
		Exits = 0;
		Enters = 0;
		do pthread_cond_wait(Resume, Debugger->Lock); while (Paused);
	pthread_mutex_unlock(Debugger->Lock);
}

void debug_message_impl(ml_frame_debug_t *State, int LineNo, ml_value_t *Message) {
	pthread_mutex_lock(Debugger->Lock);
		Paused = 1;
		json_t *Enters = json_array();
		ml_frame_debug_t *EnterState = State;
		for (int I = 0; I < Enters; ++I) {
			debug_function_t *Function = EnterState->Debug;
			json_array_insert_new(Enters, 0, json_pack("{sisisfsi}",
				"module", Function->Module->Id,
				"line", Function->LineNo,
				"state", (double)(size_t)EnterState,
				"size", Function->NoOfLocals
			));
			EnterState = EnterState->UpState;
		}
		const char *MessageString;
		asprintf(&MessageString, "%s: %s", ml_error_type(Message), ml_error_message(Message));
		debugger_update(json_pack("[is{sisisisOss}]", 0, "message",
			"exits", Exits,
			"line", LineNo,
			"enters", Enters,
			"message", MessageString
		));
		Exits = 0;
		Enters = 0;
		do pthread_cond_wait(Resume, Debugger->Lock); while (Paused);
	pthread_mutex_unlock(Debugger->Lock);
}

void debug_enter_impl(ml_frame_debug_t *NewState) {
	++Enters;
	NewState->UpState = State;
	State = NewState;
}

void debug_exit_impl(ml_frame_debug_t *State) {
	ml_frame_debug_t *UpState = State->UpState;
	State = UpState;
	if (Enters > 0) {
		--Enters;
	} else {
		++Exits;
	}
	if (State == StepOverInstance || State == StepOutInstance) {
		StepIn = 1;
	}
}

static void debug_shutdown() {
	if (Debugger->SocketPath) unlink(Debugger->SocketPath);
	debugger_update(json_pack("[is]", 0, "shutdown"));
}

void debug_enable(const char *SocketPath, int ClientMode) {
	int Socket = socket(PF_UNIX, SOCK_STREAM, 0);
	Debugger = new(debugger_t);
	//Debugger->Type = T;
	//Debugger->Scanner = new scanner_t((IO$Stream$t *)Debugger);
	//Debugger->Compiler = new compiler_t("debugger");
	//Debugger->Compiler->MissingIDFunc = (ml_value_t *)DebuggerIDFunc;
	stringmap_insert(Debugger->Commands, "pause", debugger_command_pause);
	stringmap_insert(Debugger->Commands, "continue", debugger_command_continue);
	stringmap_insert(Debugger->Commands, "step_in", debugger_command_step_in);
	stringmap_insert(Debugger->Commands, "step_out", debugger_command_step_out);
	stringmap_insert(Debugger->Commands, "step_over", debugger_command_step_over);
	stringmap_insert(Debugger->Commands, "run_to", debugger_command_run_to);
	stringmap_insert(Debugger->Commands, "list_modules", debugger_command_list_modules);
	stringmap_insert(Debugger->Commands, "get_variable", debugger_command_get_variable);
	stringmap_insert(Debugger->Commands, "set_breakpoints", debugger_command_set_breakpoints);
	stringmap_insert(Debugger->Commands, "set_message_breakpoints", debugger_command_set_message_breakpoints);
	stringmap_insert(Debugger->Commands, "evaluate", debugger_command_evaluate);
	struct sockaddr_un Name;
	Name.sun_family = AF_LOCAL;
	strcpy(Name.sun_path, SocketPath);
	if (ClientMode) {
		if (connect(Socket, (struct sockaddr *)&Name, SUN_LEN(&Name))) {
			printf("Error connecting debugger socket to %s\n", SocketPath);
			exit(1);
		}
		Debugger->Socket = Socket;
		printf("Connected debugger socket to %s\n", SocketPath);
	} else {
		Debugger->SocketPath = SocketPath;
		if (bind(Socket, (struct sockaddr *)&Name, SUN_LEN(&Name))) {
			printf("Error binding debugger socket on %s\n", SocketPath);
			exit(1);
		}
		listen(Socket, 1);
		struct sockaddr Addr;
		socklen_t Length = sizeof(Addr);
		printf("Started debugger socket on %s\n", SocketPath);
		Debugger->Socket = accept(Socket, &Addr, &Length);
		printf("Debugger connected\n");
	}
	pthread_mutex_init(Debugger->Lock, 0);
	pthread_t Thread;
	pthread_create(&Thread, 0, debugger_thread_func, Debugger);
	atexit(debug_shutdown);
}
