#include "ml_module.h"
#include "ml_macros.h"
#include <gc/gc.h>
#include <string.h>
#include <stdio.h>
#include "ml_runtime.h"

ML_TYPE(MLMiniModuleT, (MLModuleT), "module");

ML_METHODX("::", MLMiniModuleT, MLStringT) {
	ml_module_t *Module = (ml_module_t *)Args[0];
	const char *Name = ml_string_value(Args[1]);
	ml_value_t **Slot = (ml_value_t **)stringmap_slot(Module->Exports, Name);
	ml_value_t *Value = Slot[0];
	if (!Value) {
		Value = Slot[0] = ml_uninitialized(Name);
	}
	ML_RETURN(Value);
}

static void ml_export(ml_state_t *Caller, ml_module_t *Module, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLNamesT);
	int Index = 0;
	ml_value_t *Value = MLNil;
	ML_NAMES_FOREACH(Args[0], Iter) {
		const char *Name = ml_string_value(Iter->Value);
		Value = Args[++Index];
		ml_value_t **Slot = (ml_value_t **)stringmap_slot(Module->Exports, Name);
		if (Slot[0]) {
			if (ml_typeof(Slot[0]) != MLUninitializedT) {
				ML_RETURN(ml_error("ExportError", "Duplicate export %s", Name));
			}
			ml_uninitialized_set(Slot[0], Value);
		}
		Slot[0] = Value;
	}
	ML_RETURN(Value);
}

typedef struct ml_module_state_t {
	ml_state_t Base;
	ml_value_t *Module;
	ml_value_t *Args[1];
} ml_module_state_t;

ML_TYPE(MLModuleStateT, (), "module-state");

static void ml_module_done_run(ml_module_state_t *State, ml_value_t *Value) {
	ML_CONTINUE(State->Base.Caller, State->Module);
}

static void ml_module_init_run(ml_module_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Base.run = (ml_state_fn)ml_module_done_run;
	return ml_call(State, Value, 1, State->Args);
}

void ml_module_compile(ml_state_t *Caller, ml_compiler_t *Scanner, ml_value_t **Slot) {
	static const char *Parameters[] = {"export", NULL};
	ml_module_t *Module = new(ml_module_t);
	Module->Type = MLMiniModuleT;
	Module->Path = ml_compiler_name(Scanner);
	Slot[0] = (ml_value_t *)Module;
	ml_module_state_t *State = new(ml_module_state_t);
	State->Base.Type = MLModuleStateT;
	State->Base.run = (void *)ml_module_init_run;
	State->Base.Context = Caller->Context;
	State->Base.Caller = Caller;
	State->Module = (ml_value_t *)Module;
	State->Args[0] = ml_cfunctionz(Module, (ml_callbackx_t)ml_export);
	ml_function_compile((ml_state_t *)State, Scanner, Parameters);
}

void ml_module_load_file(ml_state_t *Caller, const char *FileName, ml_getter_t GlobalGet, void *Globals, ml_value_t **Slot) {
	static const char *Parameters[] = {"export", NULL};
	ml_module_t *Module = new(ml_module_t);
	Module->Type = MLMiniModuleT;
	Module->Path = FileName;
	Slot[0] = (ml_value_t *)Module;
	ml_module_state_t *State = new(ml_module_state_t);
	State->Base.Type = MLModuleStateT;
	State->Base.run = (void *)ml_module_init_run;
	State->Base.Context = Caller->Context;
	State->Base.Caller = Caller;
	State->Module = (ml_value_t *)Module;
	State->Args[0] = ml_cfunctionz(Module, (ml_callbackx_t)ml_export);
	return ml_load_file((ml_state_t *)State, GlobalGet, Globals, FileName, Parameters);
}

void ml_module_init(stringmap_t *_Globals) {
#include "ml_module_init.c"
}
