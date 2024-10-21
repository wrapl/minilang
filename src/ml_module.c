#include "ml_module.h"
#include "ml_macros.h"
#include <string.h>
#include <stdio.h>
#include "ml_runtime.h"

#undef ML_CATEGORY
#define ML_CATEGORY "module"
//!internal

typedef struct {
	ml_module_t Base;
	int Flags;
} ml_mini_module_t;

ML_TYPE(MLMiniModuleT, (MLModuleT), "module");

ML_METHODX("::", MLMiniModuleT, MLStringT) {
	ml_mini_module_t *Module = (ml_mini_module_t *)Args[0];
	const char *Name = ml_string_value(Args[1]);
	ml_value_t **Slot = (ml_value_t **)stringmap_slot(Module->Base.Exports, Name);
	if (!Slot[0]) {
		char *FullName = snew(strlen(Module->Base.Path) + strlen(Name) + 3);
		strcpy(stpcpy(stpcpy(FullName, Module->Base.Path), "::"), Name);
		Slot[0] = ml_uninitialized(FullName, ml_debugger_source(Caller));
	}
	ML_RETURN(Slot[0]);
}

static void ml_mini_module_export(ml_state_t *Caller, ml_mini_module_t *Module, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLNamesT);
	ML_NAMES_CHECKX_ARG_COUNT(0);
	int Index = 0;
	ml_value_t *Value = MLNil;
	ML_NAMES_FOREACH(Args[0], Iter) {
		const char *Name = ml_string_value(Iter->Value);
		Value = Args[++Index];
		ml_value_t **Slot = (ml_value_t **)stringmap_slot(Module->Base.Exports, Name);
		if (Module->Flags & MLMF_USE_GLOBALS) {
			if (Slot[0]) {
				if (ml_typeof(Slot[0]) == MLGlobalT) {
				} else if (ml_typeof(Slot[0]) == MLUninitializedT) {
					ml_value_t *Global = ml_global(Name);
					ml_uninitialized_set(Slot[0], Global);
					Slot[0] = Global;
				} else {
					ML_ERROR("ExportError", "Duplicate export %s", Name);
				}
			} else {
				Slot[0] = ml_global(Name);
			}
			ml_global_set(Slot[0], Value);
		} else {
			if (Slot[0]) {
				if (ml_typeof(Slot[0]) != MLUninitializedT) {
					ML_ERROR("ExportError", "Duplicate export %s", Name);
				}
				ml_uninitialized_set(Slot[0], Value);
			}
			Slot[0] = Value;
		}
	}
	ML_RETURN(Value);
}

typedef struct {
	ml_state_t Base;
	ml_value_t *Module;
} ml_module_state_t;

ML_TYPE(MLModuleStateT, (), "module-state");

static void ml_module_done_run(ml_module_state_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Value)) ML_RETURN(Value);
	ML_RETURN(State->Module);
}

static void ml_module_init_run(ml_module_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Base.run = (ml_state_fn)ml_module_done_run;
	return ml_call(State, Value, 0, NULL);
}

void ml_module_compile2(ml_state_t *Caller, const char *Path, const mlc_expr_t *Expr, ml_compiler_t *Compiler, ml_value_t **Slot, int Flags) {
	ml_mini_module_t *Module;
	if ((Flags & MLMF_USE_GLOBALS) && Slot[0] && ml_typeof(Slot[0]) == MLMiniModuleT) {
		Module = (ml_mini_module_t *)Slot[0];
	} else {
		Module = new(ml_mini_module_t);
		Module->Base.Type = MLMiniModuleT;
		Module->Base.Path = Path;
		Module->Flags = Flags;
		Slot[0] = (ml_value_t *)Module;
	}
	ml_compiler_define(Compiler, "export", ml_cfunctionz(Module, (ml_callbackx_t)ml_mini_module_export));
	ml_module_state_t *State = new(ml_module_state_t);
	State->Base.Type = MLModuleStateT;
	State->Base.run = (void *)ml_module_init_run;
	State->Base.Context = Caller->Context;
	State->Base.Caller = Caller;
	State->Module = (ml_value_t *)Module;
	return ml_function_compile((ml_state_t *)State, Expr, Compiler, NULL);
}


void ml_module_compile(ml_state_t *Caller, const char *Path, const mlc_expr_t *Expr, ml_compiler_t *Compiler, ml_value_t **Slot) {
	return ml_module_compile2(Caller, Path, Expr, Compiler, Slot, 0);
}

static void ml_module_export0(ml_state_t *Caller, ml_module_t *Module, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLNamesT);
	ML_NAMES_CHECKX_ARG_COUNT(0);
	int Index = 0;
	ml_value_t *Value = MLNil;
	ML_NAMES_FOREACH(Args[0], Iter) {
		const char *Name = ml_string_value(Iter->Value);
		Value = Args[++Index];
		ml_value_t **Slot = (ml_value_t **)stringmap_slot(Module->Exports, Name);
		if (Slot[0]) {
			if (ml_typeof(Slot[0]) != MLUninitializedT) {
				ML_ERROR("ExportError", "Duplicate export %s", Name);
			}
			ml_uninitialized_set(Slot[0], Value);
		}
		Slot[0] = Value;
	}
	ML_RETURN(Value);
}

static const char *DefaultModulePath = "<internal>";

ML_METHODX(MLModuleT, MLFunctionT) {
	ml_module_t *Module = new(ml_module_t);
	Module->Type = MLModuleT;
	Module->Path = DefaultModulePath;
	ml_module_state_t *State = new(ml_module_state_t);
	State->Base.Type = MLModuleStateT;
	State->Base.run = (void *)ml_module_done_run;
	State->Base.Context = Caller->Context;
	State->Base.Caller = Caller;
	State->Module = (ml_value_t *)Module;
	ml_value_t *Body = Args[0];
	Args[0] = ml_cfunctionz(Module, (ml_callbackx_t)ml_module_export0);
	return ml_call((ml_state_t *)State, Body, 1, Args);
}

static void ML_TYPED_FN(ml_value_set_name, MLModuleT, ml_module_t *Module, const char *Name) {
	if (Module->Path == DefaultModulePath) Module->Path = Name;
}

typedef struct {
	ml_module_t Base;
	ml_value_t *Lookup;
} ml_dynamic_module_t;

ML_TYPE(MLModuleDynamicT, (), "module::dynamic");

ML_METHOD(MLModuleDynamicT, MLStringT, MLFunctionT) {
//@module::dynamic
//<Path:string
//<Lookup:function
//>module
// Returns a generic module which calls resolves :mini:`Module::Import` by calling :mini:`Lookup(Import)`, caching results for future use.
	ml_dynamic_module_t *Module = new(ml_dynamic_module_t);
	Module->Base.Type = MLModuleDynamicT;
	Module->Base.Path = ml_string_value(Args[0]);
	Module->Lookup = Args[1];
	return (ml_value_t *)Module;
}

typedef struct {
	ml_state_t Base;
	ml_dynamic_module_t *Module;
	const char *Name;
} ml_module_lookup_state_t;

static void ml_module_lookup_run(ml_module_lookup_state_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Base.Caller;
	if (!ml_is_error(Value)) {
		stringmap_insert(State->Module->Base.Exports, State->Name, Value);
	}
	ML_RETURN(Value);
}

ML_METHODX("::", MLModuleDynamicT, MLStringT) {
//<Module
//<Name
//>MLAnyT
// Imports a symbol from a module.
	ml_dynamic_module_t *Module = (ml_dynamic_module_t *)Args[0];
	const char *Name = ml_string_value(Args[1]);
	ml_value_t *Value = stringmap_search(Module->Base.Exports, Name);
	if (!Value) {
		ml_module_lookup_state_t *State = new(ml_module_lookup_state_t);
		State->Base.Caller = Caller;
		State->Base.Context = Caller->Context;
		State->Base.run = (ml_state_fn)ml_module_lookup_run;
		State->Module = Module;
		State->Name = Name;
		return ml_call(State, Module->Lookup, 1, Args + 1);
	}
	ML_RETURN(Value);
}

ML_METHODZ("export", MLModuleDynamicT, MLStringT, MLAnyT) {
	ml_dynamic_module_t *Module = (ml_dynamic_module_t *)ml_deref(Args[0]);
	const char *Name = ml_string_value(ml_deref(Args[1]));
	stringmap_insert(Module->Base.Exports, Name, Args[2]);
	ML_RETURN(Args[2]);
}

void ml_module_init(stringmap_t *_Globals) {
	stringmap_insert(MLModuleT->Exports, "dynamic", MLModuleDynamicT);
#include "ml_module_init.c"
}
