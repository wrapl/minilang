#include "ml_library.h"
#ifdef Mingw
#else
#include <dlfcn.h>
#endif

void ml_library_load_file(ml_state_t *Caller, const char *FileName, ml_getter_t GlobalGet, void *Globals, ml_value_t **Slot) {
#if defined(Linux)
	void *Handle = dlopen(FileName, RTLD_GLOBAL | RTLD_LAZY);
	if (Handle) {
		int (*init)(ml_value_t *, ml_getter_t, void *) = dlsym(Handle, "ml_library_entry");
		if (!init) {
			dlclose(Handle);
			ML_RETURN(ml_error("LibraryError", "init function missing from %s", FileName));
		}
		ml_value_t *Library = Slot[0] = ml_module(FileName, NULL);
		init(Library, GlobalGet, Globals);
		ML_RETURN(Library);
	} else {
		ML_RETURN(ml_error("LibraryError", "Failed to load %s: %s", FileName, dlerror()));
	}
#else
	ML_RETURN(ml_error("PlatformError", "Dynamic libraries not supported"));
#endif
}

void ml_library_init(stringmap_t *Globals) {
#include "ml_library_init.c"
}
