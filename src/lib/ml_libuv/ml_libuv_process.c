#include "ml_libuv.h"

static ML_METHOD_ANON(UVSpawnMethod, "uv::spawn");

typedef ml_value_t *(*ml_uv_process_option_fn)(uv_process_options_t *Options, ml_value_t *Value);

static stringmap_t UVProcessOptions[1] = {STRINGMAP_INIT};

static void ml_uv_process_exit_cb(uv_process_t *Request, int64_t Status, int Signal) {
	ml_state_t *Caller = (ml_state_t *)Request->data;
	uv_close((uv_handle_t *)Request, NULL);
	ml_default_queue_add(Caller, ml_integer(Status));
}

ML_METHODX(UVSpawnMethod, MLStringT, MLListT) {
	uv_process_options_t Options[1] = {{0,}};
	Options->file = ml_string_value(Args[0]);
	char **Command = Options->args = anew(char *, ml_list_length(Args[1]) + 1);
	ML_LIST_FOREACH(Args[1], Iter) {
		if (!ml_is(Iter->Value, MLStringT)) {
			ML_ERROR("TypeError", "Arguments must be strings");
		}
		*Command++ = (char *)ml_string_value(Iter->Value);
	}
	Options->exit_cb = ml_uv_process_exit_cb;
	uv_process_t *Request = new(uv_process_t);
	Request->data = Caller;
	int Result = uv_spawn(Loop, Request, Options);
	if (Result) ML_ERROR("SpawnError", "%s", uv_strerror(Result));
}

ML_METHODVX(UVSpawnMethod, MLStringT, MLListT, MLNamesT) {
	uv_process_options_t Options[1] = {{0,}};
	Options->file = ml_string_value(Args[0]);
	char **Command = Options->args = anew(char *, ml_list_length(Args[1]) + 1);
	ML_LIST_FOREACH(Args[1], Iter) {
		if (!ml_is(Iter->Value, MLStringT)) {
			ML_ERROR("TypeError", "Arguments must be strings");
		}
		*Command++ = (char *)ml_string_value(Iter->Value);
	}
	ml_value_t **Option = Args + 3;
	ML_NAMES_FOREACH(Args[2], Iter) {
		const char *Name = ml_string_value(Iter->Value);
		ml_uv_process_option_fn OptionFn = (ml_uv_process_option_fn)stringmap_search(UVProcessOptions, Name);
		if (!OptionFn) ML_ERROR("OptionError", "Unknown option: %s", Name);
		ml_value_t *Error = OptionFn(Options, *Option++);
		if (Error) ML_RETURN(Error);
	}
	uv_process_t *Request = new(uv_process_t);
	Request->data = Caller;
	uv_spawn(Loop, Request, Options);
}

ML_METHODVX(UVSpawnMethod, MLStringT, MLNamesT) {
	uv_process_options_t Options[1] = {{0,}};
	Options->file = ml_string_value(Args[0]);
	Options->args = anew(char *, 1);
	ml_value_t **Option = Args + 3;
	ML_NAMES_FOREACH(Args[1], Iter) {
		const char *Name = ml_string_value(Iter->Value);
		ml_uv_process_option_fn OptionFn = (ml_uv_process_option_fn)stringmap_search(UVProcessOptions, Name);
		if (!OptionFn) ML_ERROR("OptionError", "Unknown option: %s", Name);
		ml_value_t *Error = OptionFn(Options, *Option++);
		if (Error) ML_RETURN(Error);
	}
	uv_process_t *Request = new(uv_process_t);
	Request->data = Caller;
	uv_spawn(Loop, Request, Options);
}

static ml_value_t *ml_uv_process_option_cwd(uv_process_options_t *Options, ml_value_t *Value) {
	if (!ml_is(Value, MLStringT)) return ml_error("TypeError", "Expected string path");
	Options->cwd = ml_string_value(Value);
	return NULL;
}

static ml_value_t *ml_uv_process_option_stdio(uv_process_options_t *Options, ml_value_t *Value) {
	if (!ml_is(Value, MLListT)) return ml_error("TypeError", "Expected list of streams");
	int Count = Options->stdio_count = ml_list_length(Value);
	uv_stdio_container_t *Container = Options->stdio = anew(uv_stdio_container_t, Count);
	ML_LIST_FOREACH(Value, Iter) {
		if (Iter->Value == MLNil) {
			Container->flags = UV_IGNORE;
		} else if (ml_is(Iter->Value, UVFileT)) {
			Container->flags = UV_INHERIT_FD;
			Container->data.fd = ((ml_uv_file_t *)Iter->Value)->Handle;
		} else {
			return ml_error("TypeError", "Unsupported stream type: %s", ml_typeof(Iter->Value)->Name);
		}
		++Container;
	}
	return NULL;
}

void ml_libuv_process_init(stringmap_t *Globals) {
#include "ml_libuv_process_init.c"
	stringmap_insert(UVProcessOptions, "stdio", ml_uv_process_option_stdio);
	stringmap_insert(UVProcessOptions, "cwd", ml_uv_process_option_cwd);
	if (Globals) {
		stringmap_insert(Globals, "spawn", UVSpawnMethod);
	}
}
