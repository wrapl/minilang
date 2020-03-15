#include "ml_module.h"
#include "ml_macros.h"
#include <gc/gc.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "ml_runtime.h"

typedef struct ml_module_state_t {
	ml_state_t Base;
	ml_value_t *Module;
	ml_value_t *Args[1];
} ml_module_state_t;

static ml_type_t *MLModuleStateT;

typedef struct ml_mini_module_t {
	const ml_type_t *Type;
	const char *FileName;
	stringmap_t Exports[1];
} ml_mini_module_t;

ml_type_t MLMiniModuleT[1] = {{
	MLTypeT,
	MLModuleT, "mini-module",
	ml_default_hash,
	ml_default_call,
	ml_default_deref,
	ml_default_assign,
	NULL, 0, 0
}};

ML_METHODX("::", MLMiniModuleT, MLStringT) {
	ml_mini_module_t *Module = (ml_mini_module_t *)Args[0];
	const char *Name = ml_string_value(Args[1]);
	ml_value_t **Slot = (ml_value_t **)stringmap_slot(Module->Exports, Name);
	ml_value_t *Value = Slot[0];
	if (!Value) {
		ml_uninitialized_t *Uninitialized = new(ml_uninitialized_t);
		Uninitialized->Type = MLUninitializedT;
		Value = Slot[0] = (ml_value_t *)Uninitialized;
	}
	ML_CONTINUE(Caller, Value);
}

typedef struct ml_export_function_t {
	const ml_type_t *Type;
	ml_mini_module_t *Module;
} ml_export_function_t;

static ml_value_t *ml_export_function_call(ml_state_t *Caller, ml_export_function_t *ExportFunction, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(2);
	ml_value_t *NameValue = Args[0]->Type->deref(Args[0]);
	if (NameValue->Type != MLStringT) ML_CHECKX_ARG_TYPE(0, MLStringT);
	const char *Name = ml_string_value(NameValue);
	ml_mini_module_t *Module = ExportFunction->Module;
	ml_value_t *Value = Args[1];
	ml_value_t **Slot = (ml_value_t **)stringmap_slot(Module->Exports, Name);
	if (Slot[0]) {
		ml_uninitialized_t *Uninitialized = (ml_uninitialized_t *)Slot[0];
		if (Uninitialized->Type != MLUninitializedT) {
			ML_CONTINUE(Caller, ml_error("ExportError", "Duplicate export %s", Name));
		}
		for (ml_slot_t *Slot = Uninitialized->Slots; Slot; Slot = Slot->Next) Slot->Value[0] = Value;
	}
	ML_CONTINUE(Caller, Slot[0] = Value);
}

ml_type_t MLExportFunctionT[1] = {{
	MLTypeT,
	MLFunctionT, "export-function",
	ml_default_hash,
	(void *)ml_export_function_call,
	ml_default_deref,
	ml_default_assign,
	NULL, 0, 0
}};

static ml_value_t *ml_module_state_run(ml_module_state_t *State, ml_value_t *Value) {
	if (Value->Type == MLErrorT) ML_CONTINUE(State->Base.Caller, Value);
	ML_CONTINUE(State->Base.Caller, State->Module);
}

ml_value_t *ml_module_load_file(ml_state_t *Caller, const char *FileName, ml_getter_t GlobalGet, void *Globals, ml_value_t **Slot) {
	static const char *Parameters[] = {"export", NULL};
	ml_mini_module_t *Module = new(ml_mini_module_t);
	Module->Type = MLMiniModuleT;
	Module->FileName = FileName;
	Slot[0] = (ml_value_t *)Module;
	ml_export_function_t *ExportFunction = new(ml_export_function_t);
	ExportFunction->Type = MLExportFunctionT;
	ExportFunction->Module = Module;
	ml_value_t *Init = ml_load(GlobalGet, Globals, FileName, Parameters);
	ml_module_state_t *State = new(ml_module_state_t);
	State->Base.Type = MLModuleStateT;
	State->Base.run = (void *)ml_module_state_run;
	State->Base.Caller = Caller;
	State->Module = Module;
	State->Args[0] = (ml_value_t *)ExportFunction;
	return Init->Type->call((ml_state_t *)State, Init, 1, State->Args);
}

void ml_module_init(stringmap_t *_Globals) {
	MLModuleStateT = ml_type(MLAnyT, "module-state");
#include "ml_module_init.c"
}
