#include "../../ml_library.h"
#include "../../ml_macros.h"
#include <libwebsockets.h>
#include <uv.h>

typedef struct {
	ml_type_t *Type;
	struct lws_context *Handle;
} ml_lws_context_t;

ML_TYPE(LWSContextT, (), "lws-context");

typedef ml_value_t *(*lws_context_creation_info_fn)(struct lws_context_creation_info *Info, ml_value_t *Arg);

static stringmap_t CreationInfoFns[1] = {STRINGMAP_INIT};

extern uv_loop_t *ml_libuv_loop();

ML_METHOD(LWSContextT, MLNamesT) {
	struct lws_context_creation_info Info = {0,};
	uv_loop_t *Loop = ml_libuv_loop();
	Info.options |= LWS_SERVER_OPTION_LIBUV;
	Info.foreign_loops = (void **)&Loop;
	ML_NAMES_FOREACH(Args[0], Iter) {
		const char *Name = ml_string_value(Iter->Value);
		lws_context_creation_info_fn Fn = (lws_context_creation_info_fn)stringmap_search(CreationInfoFns, Name);
		if (!Fn) return ml_error("NameError", "Unknown option: %s", Name);
		ml_value_t *Error = Fn(&Info, *++Args);
		if (Error) return Error;
	}
	ml_lws_context_t *Context = new(ml_lws_context_t);
	Context->Type = LWSContextT;
	Context->Handle = lws_create_context(&Info);
	return (ml_value_t *)Context;
}

void ml_library_entry0(ml_value_t *Module) {
#include "ml_libwebsockets_init.c"
	ml_module_export(Module, "context", (ml_value_t *)LWSContextT);
}
