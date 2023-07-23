#include "ml_library.h"
#include "ml_macros.h"
#include "ml_module.h"
#include <string.h>
#ifdef Mingw
#else
#include <dlfcn.h>
#endif

#undef ML_CATEGORY
#define ML_CATEGORY "library"

static ml_value_t *LibraryPath;
static int MaxLibraryPathLength = 0;
static stringmap_t Modules[1] = {STRINGMAP_INIT};
static stringmap_t *Globals;

void ml_library_register(const char *Name, ml_value_t *Module) {
	stringmap_insert(Modules, Name, Module);
}

ml_module_t *ml_library_internal(const char *Name) {
	ml_module_t *Module = (ml_module_t *)ml_module(Name, NULL);
	stringmap_insert(Modules, Name, Module);
	return Module;
}

typedef struct ml_library_loader_t ml_library_loader_t;

struct ml_library_loader_t {
	ml_library_loader_t *Next;
	const char *Extension;
	int (*test)(const char *FileName);
	void (*load)(ml_state_t *Caller, const char *FileName, ml_value_t **Slot);
	ml_value_t *(*load0)(const char *FileName, ml_value_t **Slot);
};

static ml_library_loader_t *Loaders = NULL;
static int MaxLibraryExtensionLength = 8;

typedef struct {
	ml_library_loader_t *Loader;
	const char *FileName;
} ml_library_info_t;

static void internal_load(ml_state_t *Caller, const char *FileName, ml_value_t **Slot) {
}

static ml_value_t *internal_load0(const char *FileName, ml_value_t **Slot) {
	return Slot[0];
}

static ml_library_loader_t InternalLoader = {
	.load = internal_load,
	.load0 = internal_load0
};

extern ml_value_t *SymbolMethod;

static char *path_join(const char *Base, const char *Rest, int Remove, int Space) {
	while (Rest[0] == '.') {
		if (Rest[1] == '/') {
			Rest += 2;
		} else if (Rest[1] == '.' && Rest[2] == '/') {
			Rest += 3;
			++Remove;
		} else if (Rest[1] == 0) {
			Rest += 1;
		} else {
			break;
		}
	}
	int BaseLength = strlen(Base);
	while (Remove && --BaseLength > 0) {
		if (Base[BaseLength] == '/') --Remove;
	}
	if (Remove) return NULL;
	char *Path = snew(BaseLength + 2 + strlen(Rest) + Space);
	char *End = mempcpy(Path, Base, BaseLength);
	if (Rest[0]) {
		*End++ = '/';
		strcpy(End, Rest);
	}
	return Path;
}

static ml_library_info_t ml_library_find(const char *Path, const char *Name) {
	ml_library_loader_t *Loader = NULL;
	if (Path) {
		char *FileName = path_join(Path, Name, 0, MaxLibraryExtensionLength);
		if (!FileName) return (ml_library_info_t){NULL, NULL};
		char *End = FileName + strlen(FileName);
		Loader = Loaders;
		while (Loader) {
			strcpy(End, Loader->Extension);
			if (Loader->test(FileName)) return (ml_library_info_t){Loader, FileName};
			Loader = Loader->Next;
		}
	} else {
		char *FileName = snew(MaxLibraryPathLength + 1 + strlen(Name) + MaxLibraryExtensionLength);
		ML_LIST_FOREACH(LibraryPath, Iter) {
			char *End = stpcpy(FileName, ml_string_value(Iter->Value));
			*End++ = '/';
			End = stpcpy(End, Name);
			Loader = Loaders;
			while (Loader) {
				strcpy(End, Loader->Extension);
				if (Loader->test(FileName)) return (ml_library_info_t){Loader, FileName};
				Loader = Loader->Next;
			}
		}
		ml_value_t *Internal = stringmap_search(Modules, Name);
		if (Internal) return (ml_library_info_t){&InternalLoader, Name};
	}
	return (ml_library_info_t){NULL, NULL};
}

typedef struct {
	ml_state_t Base;
	const char *Path;
	char *Name, *Import;
} ml_import_state_t;

static void ml_parent_state_run(ml_import_state_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Base.Caller;
	*State->Import = '/';
	if (ml_is(Value, MLErrorT)) ML_RETURN(Value);
	if (ml_is(Value, MLModuleT)) {
		ml_value_t *Import = ml_module_import(Value, State->Import + 1);
		if (Import) {
			ML_RETURN(Import);
		} else {
			ML_ERROR("ModuleError", "Module %s/%s not found in %s", State->Name, State->Import, State->Path ?: "<library>");
		}
	} else {
		ml_value_t **Args = ml_alloc_args(2);
		Args[0] = Value;
		Args[1] = ml_string(State->Import + 1, -1);
		return ml_call(Caller, SymbolMethod, 2, Args);
	}
}

void ml_library_load(ml_state_t *Caller, const char *Path, const char *Name) {
	ml_library_info_t Info = ml_library_find(Path, Name);
	if (!Info.Loader) {
		char *NameCopy = GC_strdup(Name);
		char *Import = strrchr(NameCopy, '/');
		if (Import) {
			ml_import_state_t *State = new(ml_import_state_t);
			State->Base.Caller = Caller;
			State->Base.Context = Caller->Context;
			State->Base.run = (ml_state_fn)ml_parent_state_run;
			State->Path = Path;
			State->Name = NameCopy;
			State->Import = Import;
			*Import = 0;
			return ml_library_load((ml_state_t *)State, Path, NameCopy);
		}
		ML_ERROR("ModuleError", "Module %s not found in %s", Name, Path ?: "<library>");
	}
	ml_value_t **Slot = (ml_value_t **)stringmap_slot(Modules, Info.FileName);
	if (!Slot[0]) return Info.Loader->load(Caller, Info.FileName, Slot);
	ML_RETURN(Slot[0]);
}

ml_value_t *ml_library_load0(const char *Path, const char *Name) {
	ml_library_info_t Info = ml_library_find(Path, Name);
	if (!Info.Loader) {
		char *Import = strrchr(Name, '/');
		if (Import) {
			*Import = 0;
			ml_value_t *Parent = ml_library_load0(Path, Name);
			*Import = '/';
			if (ml_is_error(Parent)) return Parent;
			if (ml_is(Parent, MLModuleT)) {
				ml_value_t *Value = ml_module_import(Parent, Import + 1);
				if (Value) return Value;
			}
		}
		return ml_error("ModuleError", "Module %s not found in %s", Name, Path ?: "<library>");
	}
	ml_value_t **Slot = (ml_value_t **)stringmap_slot(Modules, Info.FileName);
	if (!Slot[0]) return Info.Loader->load0(Info.FileName, Slot);
	return Slot[0];
}

#include <sys/stat.h>

static int ml_library_test_file(const char *FileName) {
	struct stat Stat[1];
	return !lstat(FileName, Stat);
}

static ml_value_t *ml_library_default_load0(const char *FileName, ml_value_t **Slot) {
	return ml_error("ModuleError", "Module %s loaded incorrectly", FileName);
}

typedef struct {
	ml_type_t *Type;
	const char *Path;
} ml_importer_t;

static void ml_importer_call(ml_state_t *Caller, ml_importer_t *Importer, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	ml_value_t *NameArg = ml_deref(Args[0]);
	if (!ml_is(NameArg, MLStringT)) ML_ERROR("TypeError", "Expected string for argument 1");
	const char *Name = ml_string_value(NameArg);
	if (Name[0] == '.') {
		return ml_library_load(Caller, Importer->Path, Name);
	} else {
		return ml_library_load(Caller, NULL, Name);
	}
}

ML_TYPE(MLImporterT, (MLFunctionT), "importer",
	.call = (void *)ml_importer_call
);

ML_METHOD("append", MLStringBufferT, MLImporterT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_importer_t *Importer = (ml_importer_t *)Args[1];
	if (!Importer->Path) return MLNil;
	ml_stringbuffer_write(Buffer, Importer->Path, strlen(Importer->Path));
	return MLSome;
}

extern const char *ml_load_file_read(void *Data);

static void ml_library_mini_load(ml_state_t *Caller, const char *FileName, ml_value_t **Slot) {
	FILE *File = fopen(FileName, "r");
	if (!File) ML_ERROR("LoadError", "error opening %s", FileName);
	ml_parser_t *Parser = ml_parser(ml_load_file_read, File);
	int LineNo = 1;
	const char *Line = ml_load_file_read(File);
	if (!Line) ML_ERROR("LoadError", "empty file %s", FileName);
	if (Line[0] == '#' && Line[1] == '!') {
		LineNo = 2;
		Line = ml_load_file_read(File);
		if (!Line) ML_ERROR("LoadError", "empty file %s", FileName);
	}
	ml_parser_source(Parser, (ml_source_t){FileName, LineNo});
	ml_parser_input(Parser, Line);
	ml_compiler_t *Compiler = ml_compiler((ml_getter_t)stringmap_global_get, Globals);
	ml_importer_t *Importer = new(ml_importer_t);
	Importer->Type = MLImporterT;
	int FileNameLength = strlen(FileName);
	for (int I = FileNameLength; --I >= 0;) {
		if (FileName[I] == '/') {
			char *Path = snew(I + 1);
			memcpy(Path, FileName, I);
			Path[I] = 0;
			Importer->Path = Path;
			break;
		}
	}
	ml_compiler_define(Compiler, "import", (ml_value_t *)Importer);
	return ml_module_compile(Caller, Parser, Compiler, Slot);
}

static void ml_library_so_load(ml_state_t *Caller, const char *FileName, ml_value_t **Slot) {
	void *Handle = dlopen(FileName, RTLD_GLOBAL | RTLD_LAZY);
	if (Handle) {
		ml_library_entry0_t init0 = dlsym(Handle, "ml_library_entry0");
		if (init0) {
			Slot[0] = ml_error("ModuleError", "Library %s loaded recursively", FileName);
			init0(Slot);
			ML_RETURN(Slot[0]);
		}
		ml_library_entry_t init = dlsym(Handle, "ml_library_entry");
		if (init) {
			Slot[0] = ml_error("ModuleError", "Library %s loaded recursively", FileName);
			init(Caller, Slot);
			ML_RETURN(Slot[0]);
		}
		dlclose(Handle);
		ML_ERROR("LibraryError", "init function missing from %s", FileName);
	} else {
		ML_ERROR("LibraryError", "Failed to load %s: %s", FileName, dlerror());
	}
}

static ml_value_t *ml_library_so_load0(const char *FileName, ml_value_t **Slot) {
	void *Handle = dlopen(FileName, RTLD_GLOBAL | RTLD_LAZY);
	if (Handle) {
		ml_library_entry0_t init0 = dlsym(Handle, "ml_library_entry0");
		if (init0) {
			Slot[0] = ml_error("ModuleError", "Library %s loaded recursively", FileName);
			init0(Slot);
			return Slot[0];
		}
		ml_library_entry_t init = dlsym(Handle, "ml_library_entry");
		if (init) return ml_library_default_load0(FileName, Slot);
		dlclose(Handle);
		return ml_error("LibraryError", "init function missing from %s", FileName);
	} else {
		return ml_error("LibraryError", "Failed to load %s: %s", FileName, dlerror());
	}
}

typedef struct {
	ml_type_t *Type;
	const char *Path;
} ml_dir_module_t;

ML_TYPE(MLModuleDirT, (), "module::directory");

ML_METHODX("::", MLModuleDirT, MLStringT) {
	ml_dir_module_t *Module = (ml_dir_module_t *)Args[0];
	const char *Name = ml_string_value(Args[1]);
	return ml_library_load(Caller, Module->Path, Name);
}

static int ml_library_dir_test(const char *FileName) {
	struct stat Stat[1];
	return !lstat(FileName, Stat) && S_ISDIR(Stat->st_mode);
}

static void ml_library_dir_load(ml_state_t *Caller, const char *FileName, ml_value_t **Slot) {
	ml_dir_module_t *Module = new(ml_dir_module_t);
	Module->Type = MLModuleDirT;
	Module->Path = FileName;
	Slot[0] = (ml_value_t *)Module;
	ML_RETURN(Module);
}

static ml_value_t *ml_library_dir_load0(const char *FileName, ml_value_t **Slot) {
	ml_dir_module_t *Module = new(ml_dir_module_t);
	Module->Type = MLModuleDirT;
	Module->Path = FileName;
	Slot[0] = (ml_value_t *)Module;
	return Slot[0];
}

#include "whereami.h"

void ml_library_path_add(const char *Path) {
	if (Path[0] != '/') {
		char *Cwd = getcwd(NULL, 0);
		Path = path_join(Cwd, Path, 0, 0);
		free(Cwd);
	}
	int PathLength = strlen(Path);
	ml_list_push(LibraryPath, ml_string(Path, PathLength));
	if (MaxLibraryPathLength < PathLength) MaxLibraryPathLength = PathLength;
}

ML_FUNCTION(AddPath) {
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	ml_library_path_add(ml_string_value(Args[0]));
	return MLNil;
}

ML_FUNCTION(GetPath) {
	ml_value_t *Path = ml_list();
	ML_LIST_FOREACH(LibraryPath, Iter) ml_list_put(Path, Iter->Value);
	return Path;
}

static void ml_library_path_add_default(void) {
	int ExecutablePathLength = wai_getExecutablePath(NULL, 0, NULL);
	char *ExecutablePath = snew(ExecutablePathLength + 1);
	wai_getExecutablePath(ExecutablePath, ExecutablePathLength + 1, &ExecutablePathLength);
	ExecutablePath[ExecutablePathLength] = 0;
	for (int I = ExecutablePathLength - 1; I > 0; --I) {
		if (ExecutablePath[I] == '/') {
			ExecutablePath[I] = 0;
			ExecutablePathLength = I;
			break;
		}
	}
	int LibPathLength = ExecutablePathLength + strlen("/lib/minilang");
	char *LibPath = snew(LibPathLength + 1);
	memcpy(LibPath, ExecutablePath, ExecutablePathLength);
	strcpy(LibPath + ExecutablePathLength, "/lib/minilang");
	//printf("Looking for library path at %s\n", LibPath);
	struct stat Stat[1];
	if (!lstat(LibPath, Stat) && S_ISDIR(Stat->st_mode)) {
		ml_library_path_add(LibPath);
	}
}

void ml_library_loader_add(
	const char *Extension, int (*Test)(const char *),
	void (*Load)(ml_state_t *, const char *, ml_value_t **),
	ml_value_t *(*Load0)(const char *, ml_value_t **)
) {
	ml_library_loader_t *Loader = new(ml_library_loader_t);
	Loader->Next = Loaders;
	Loaders = Loader;
	Loader->Extension = Extension;
	Loader->test = Test ?: ml_library_test_file;
	Loader->load = Load;
	Loader->load0 = Load0 ?: ml_library_default_load0;
}

ML_FUNCTION(Unload) {
//<Path
//>nil
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	ml_library_info_t Info = ml_library_find(NULL, ml_string_value(Args[0]));
	if (!Info.FileName) return ml_error("ModuleError", "File %s not found", ml_string_value(Args[0]));
	stringmap_remove(Modules, Info.FileName);
	return MLNil;
}

static ml_importer_t Importer[1] = {{MLImporterT, NULL}};

void ml_library_init(stringmap_t *_Globals) {
	Globals = _Globals;
	LibraryPath = ml_list();
	ml_library_path_add_default();
	ml_library_loader_add(".mini", NULL, ml_library_mini_load, NULL);
	ml_library_loader_add(".so", NULL, ml_library_so_load, ml_library_so_load0);
	//ml_library_loader_add("", ml_library_dir_test, ml_library_dir_load, ml_library_dir_load0);
#include "ml_library_init.c"
	stringmap_insert(Globals, "import", Importer);
	stringmap_insert(Globals, "library",  ml_module("library",
		"unload", Unload,
		"add_path", AddPath,
		"get_path", GetPath,
	NULL));
}
