#include "ml_module.h"
#include "ml_macros.h"
#include <gc/gc.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "ml_runtime.h"

struct ml_module_t {
	const ml_type_t *Type;
	ml_value_t *Name;
	stringmap_t Exports[1];
};

typedef struct ml_module_state_t {
	ml_state_t Base;
	ml_module_t *Module;
	ml_value_t *Args[1];
} ml_module_state_t;

static ml_type_t *MLModuleT, *MLModuleStateT;
static stringmap_t Modules[1] = {STRINGMAP_INIT};
static stringmap_t *Globals = 0;

typedef struct ml_export_function_t {
	const ml_type_t *Type;
	ml_module_t *Module;
} ml_export_function_t;

static ml_value_t *ml_export_function_call(ml_state_t *Caller, ml_export_function_t *ExportFunction, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(2);
	ml_value_t *NameValue = Args[0]->Type->deref(Args[0]);
	if (NameValue->Type != MLStringT) ML_CHECKX_ARG_TYPE(0, MLStringT);
	const char *Name = ml_string_value(NameValue);
	ml_module_t *Module = ExportFunction->Module;
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

static ml_value_t *ml_import_fnx(ml_state_t *Caller, void *Data, int Count, ml_value_t **Args) {
	static const char *Parameters[] = {"export", NULL};
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLStringT);
	const char *FileName = ml_string_value(Args[0]);
	ml_module_t **Slot = (ml_module_t **)stringmap_slot(Modules, FileName);
	ml_module_t *Module = Slot[0];
	if (Module) ML_CONTINUE(Caller, Module);
	Module = Slot[0] = new(ml_module_t);
	Module->Type = MLModuleT;
	Module->Name = ml_string(FileName, -1);
	ml_export_function_t *ExportFunction = new(ml_export_function_t);
	ExportFunction->Type = MLExportFunctionT;
	ExportFunction->Module = Module;
	ml_value_t *Init = ml_load((ml_getter_t)stringmap_search, Globals, FileName, Parameters);
	ml_module_state_t *State = new(ml_module_state_t);
	State->Base.Type = MLModuleStateT;
	State->Base.run = (void *)ml_module_state_run;
	State->Base.Caller = Caller;
	State->Module = Module;
	State->Args[0] = (ml_value_t *)ExportFunction;
	return Init->Type->call((ml_state_t *)State, Init, 1, State->Args);
}

ML_METHODX("::", MLModuleT, MLStringT) {
	ml_module_t *Module = (ml_module_t *)Args[0];
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

ml_module_t *ml_module(const char *Name, ...) {
	ml_module_t *Module = new(ml_module_t);
	Module->Type = MLModuleT;
	Module->Name = ml_string(Name, -1);
	va_list Args;
	va_start(Args, Name);
	const char *Export;
	while ((Export = va_arg(Args, const char *))) {
		stringmap_insert(Module->Exports, Export, va_arg(Args, ml_value_t *));
	}
	va_end(Args);
	return Module;
}

void ml_module_export(ml_module_t *Module, const char *Name, ml_value_t *Value) {
	stringmap_insert(Module->Exports, Name, Value);
}

static ml_value_t *ml_module_fn(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	ml_module_t *Module = new(ml_module_t);
	Module->Type = MLModuleT;
	Module->Name = Args[0];
	return (ml_value_t *)Module;
}

ML_METHOD("export", MLModuleT, MLStringT, MLAnyT) {
	ml_module_t *Module = (ml_module_t *)Args[0];
	stringmap_insert(Module->Exports, ml_string_value(Args[1]), Args[2]);
	return Args[2];
}

void ml_module_init(stringmap_t *_Globals) {
	MLModuleT = ml_type(MLAnyT, "module");
	MLModuleStateT = ml_type(MLAnyT, "module-state");
	Globals = _Globals;
	if (Globals) {
		stringmap_insert(Globals, "import", ml_functionx(NULL, ml_import_fnx));
		stringmap_insert(Globals, "module", ml_function(NULL, ml_module_fn));
	}
#include "ml_module_init.c"
}
