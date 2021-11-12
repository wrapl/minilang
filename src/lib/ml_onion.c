#include "../ml_library.h"
#include "../ml_macros.h"
#define HAVE_PTHREADS
#include <onion/onion.h>
#include <onion/block.h>
#include <onion/low.h>
#include <onion/log.h>
#include <onion/exportlocal.h>
#include <onion/static.h>
#include <onion/websocket.h>
#include <gc/gc_pthread_redirects.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

typedef struct ml_onion_t {
	const ml_type_t *Type;
	onion *Handle;
} ml_onion_t;

static ml_type_t *OnionT;

static ml_value_t *ml_onion_new_fn(void *Data, int Count, ml_value_t **Args) {
	ml_onion_t *Onion = new(ml_onion_t);
	Onion->Type = OnionT;
	Onion->Handle = onion_new(Count >= 1 ? ml_integer_value(Args[0]) : 0);
	onion_set_client_data(Onion->Handle, Onion, NULL);
	return (ml_value_t *)Onion;
}

ML_METHOD("listen", OnionT) {
	ml_onion_t *Onion = (ml_onion_t *)Args[0];
	return ml_integer(onion_listen(Onion->Handle));
}

ML_METHOD("listen_stop", OnionT) {
	ml_onion_t *Onion = (ml_onion_t *)Args[0];
	onion_listen_stop(Onion->Handle);
	return MLNil;
}

typedef struct ml_onion_handler_t {
	const ml_type_t *Type;
	onion_handler *Handle;
} ml_onion_handler_t;

static ml_type_t *OnionHandlerT;

ML_METHOD("get_root_handler", OnionT) {
	ml_onion_t *Onion = (ml_onion_t *)Args[0];
	ml_onion_handler_t *Handler = new(ml_onion_handler_t);
	Handler->Type = OnionHandlerT;
	Handler->Handle = onion_get_root_handler(Onion->Handle);
	return (ml_value_t *)Handler;
}

ML_METHOD("set_root_handler", OnionT, OnionHandlerT) {
	ml_onion_t *Onion = (ml_onion_t *)Args[0];
	ml_onion_handler_t *Handler = (ml_onion_handler_t *)Args[1];
	onion_set_root_handler(Onion->Handle, Handler->Handle);
	return Args[0];
}

ML_METHOD("set_internal_error_handler", OnionT, OnionHandlerT) {
	ml_onion_t *Onion = (ml_onion_t *)Args[0];
	ml_onion_handler_t *Handler = (ml_onion_handler_t *)Args[1];
	onion_set_internal_error_handler(Onion->Handle, Handler->Handle);
	return Args[0];
}

ML_METHOD("set_hostname", OnionT, MLStringT) {
	ml_onion_t *Onion = (ml_onion_t *)Args[0];
	onion_set_hostname(Onion->Handle, ml_string_value(Args[1]));
	return Args[0];
}

ML_METHOD("set_port", OnionT, MLStringT) {
	ml_onion_t *Onion = (ml_onion_t *)Args[0];
	onion_set_port(Onion->Handle, ml_string_value(Args[1]));
	return Args[0];
}

typedef struct ml_onion_request_t {
	const ml_type_t *Type;
	onion_request *Handle;
} ml_onion_request_t;

static ml_type_t *OnionRequestT;

ML_METHOD("get_path", OnionRequestT) {
	ml_onion_request_t *Request = (ml_onion_request_t *)Args[0];
	return ml_string(onion_request_get_path(Request->Handle), -1);
}

ML_METHOD("get_fullpath", OnionRequestT) {
	ml_onion_request_t *Request = (ml_onion_request_t *)Args[0];
	return ml_string(onion_request_get_fullpath(Request->Handle), -1);
}

ML_METHOD("get_header", OnionRequestT, MLStringT) {
	ml_onion_request_t *Request = (ml_onion_request_t *)Args[0];
	return ml_string(onion_request_get_header(Request->Handle, ml_string_value(Args[1])), -1);
}

ML_METHOD("get_query", OnionRequestT, MLStringT) {
	ml_onion_request_t *Request = (ml_onion_request_t *)Args[0];
	return ml_string(onion_request_get_query(Request->Handle, ml_string_value(Args[1])), -1);
}

ML_METHOD("get_post", OnionRequestT, MLStringT) {
	ml_onion_request_t *Request = (ml_onion_request_t *)Args[0];
	return ml_string(onion_request_get_post(Request->Handle, ml_string_value(Args[1])), -1);
}

ML_METHOD("get_file", OnionRequestT, MLStringT) {
	ml_onion_request_t *Request = (ml_onion_request_t *)Args[0];
	return ml_string(onion_request_get_file(Request->Handle, ml_string_value(Args[1])), -1);
}

ML_METHOD("get_cookie", OnionRequestT, MLStringT) {
	ml_onion_request_t *Request = (ml_onion_request_t *)Args[0];
	return ml_string(onion_request_get_cookie(Request->Handle, ml_string_value(Args[1])), -1);
}

ML_METHOD("get_data", OnionRequestT) {
	ml_onion_request_t *Request = (ml_onion_request_t *)Args[0];
	const onion_block *Block = onion_request_get_data(Request->Handle);
	if (!Block) return MLNil;
	return ml_string(onion_block_data(Block), onion_block_size(Block));
}

typedef struct ml_onion_response_t {
	const ml_type_t *Type;
	onion_response *Handle;
} ml_onion_response_t;

static ml_type_t *OnionResponseT;

ML_METHOD("set_code", OnionResponseT, MLIntegerT) {
	ml_onion_response_t *Response = (ml_onion_response_t *)Args[0];
	onion_response_set_code(Response->Handle, ml_integer_value(Args[1]));
	return Args[0];
}

ML_METHOD("set_header", OnionResponseT, MLStringT, MLStringT) {
	ml_onion_response_t *Response = (ml_onion_response_t *)Args[0];
	onion_response_set_header(Response->Handle, ml_string_value(Args[1]), ml_string_value(Args[2]));
	return Args[0];
}

ML_METHOD("add_cookie", OnionResponseT, MLStringT, MLStringT) {
	time_t Validity = 0;
	const char *Path = NULL, *Domain = NULL;
	int Flags = 0;
	ml_onion_response_t *Response = (ml_onion_response_t *)Args[0];
	onion_response_add_cookie(Response->Handle, ml_string_value(Args[1]), ml_string_value(Args[2]), Validity, Path, Domain, Flags);
	return Args[0];
}

ML_METHOD("write", OnionResponseT, MLStringT) {
	ml_onion_response_t *Response = (ml_onion_response_t *)Args[0];
	onion_response_write(Response->Handle, ml_string_value(Args[1]), ml_string_length(Args[1]));
	return Args[0];
}

ML_METHOD("flush", OnionResponseT) {
	ml_onion_response_t *Response = (ml_onion_response_t *)Args[0];
	onion_response_flush(Response->Handle);
	return Args[0];
}

typedef struct ml_onion_state_t {
	ml_state_t Base;
	ml_value_t *Args[2];
	onion_connection_status Status;
} ml_onion_state_t;

static ml_type_t *OnionStateT;

typedef struct ml_onion_connection_status_t {
	const ml_type_t *Type;
	const char *Name;
	onion_connection_status Value;
} ml_onion_connection_status_t;

static ml_type_t *OnionConnectionStatusT;

ML_METHOD("append", MLStringBufferT, OnionConnectionStatusT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_onion_connection_status_t *Status = (ml_onion_connection_status_t *)Args[1];
	ml_stringbuffer_write(Buffer, Status->Name, strlen(Status->Name));
	return MLSome;
}

static ml_value_t *ml_onion_connection_status(const char *Name, onion_connection_status Value) {
	ml_onion_connection_status_t *Status = new(ml_onion_connection_status_t);
	Status->Type = OnionConnectionStatusT;
	Status->Name = Name;
	Status->Value = Value;
	return (ml_value_t *)Status;
}

static ml_value_t *ml_onion_state_run(ml_onion_state_t *State, ml_value_t *Result) {
	if (Result->Type == MLErrorT) {
		printf("Error: %s\n", ml_error_message(Result));
		ml_source_t Source;
		int Level = 0;
		while (ml_error_source(Result, Level++, &Source)) {
			printf("\t%s:%d\n", Source.Name, Source.Line);
		}
		State->Status = OCS_INTERNAL_ERROR;
		return Result;
	}
	if (State->Status == OCS_NOT_PROCESSED) {
		if (Result->Type == OnionConnectionStatusT) {
			State->Status = ((ml_onion_connection_status_t *)Result)->Value;
		} else {
			State->Status = OCS_PROCESSED;
		}
	}
	//ml_onion_request_t *Request = (ml_onion_request_t *)State->Args[0];
	//ml_onion_request_t *Response = (ml_onion_request_t *)State->Args[1];
	//onion_request_free(Request->Handle);
	//onion_response_free(Response->Handle);
	return Result;
}

static onion_connection_status ml_onion_handler(ml_value_t *Callback, onion_request *Req, onion_response *Res) {
	ml_onion_request_t *Request = new(ml_onion_request_t);
	Request->Type = OnionRequestT;
	Request->Handle = Req;
	ml_onion_response_t *Response = new(ml_onion_response_t);
	Response->Type = OnionResponseT;
	Response->Handle = Res;
	ml_onion_state_t *State = new(ml_onion_state_t);
	State->Base.Type = OnionStateT;
	State->Base.run = (void *)ml_onion_state_run;
	State->Base.Context = &MLRootContext;
	// TODO: Replace this with the proper context
	State->Args[0] = (ml_value_t *)Request;
	State->Args[1] = (ml_value_t *)Response;
	State->Status = OCS_NOT_PROCESSED;
	Callback->Type->call((ml_state_t *)State, Callback, 2, State->Args);
	if (State->Status == OCS_NOT_PROCESSED) {
		return (State->Status = OCS_YIELD);
	} else {
		return State->Status;
	}
}

static ml_value_t *ml_onion_handler_new_fn(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(1);
	ml_onion_handler_t *Handler = new(ml_onion_handler_t);
	Handler->Type = OnionHandlerT;
	Handler->Handle = onion_handler_new((void *)ml_onion_handler, Args[0], NULL);
	return (ml_value_t*)Handler;
}

static ml_value_t *ml_onion_export_local_fn(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	ml_onion_handler_t *Handler = new(ml_onion_handler_t);
	Handler->Type = OnionHandlerT;
	Handler->Handle = onion_handler_export_local_new(ml_string_value(Args[0]));
	return (ml_value_t *)Handler;
}

static ml_value_t *ml_onion_handler_static_fn(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(2);
	ML_CHECK_ARG_TYPE(0, MLIntegerT);
	ML_CHECK_ARG_TYPE(1, MLStringT);
	ml_onion_handler_t *Handler = new(ml_onion_handler_t);
	Handler->Type = OnionHandlerT;
	Handler->Handle = onion_handler_static(ml_string_value(Args[1]), ml_integer_value(Args[0]));
	return (ml_value_t *)Handler;
}

ML_METHOD("add", OnionHandlerT, OnionHandlerT) {
	ml_onion_handler_t *Base = (ml_onion_handler_t *)Args[0];
	ml_onion_handler_t *New = (ml_onion_handler_t *)Args[1];
	onion_handler_add(Base->Handle, New->Handle);
	return Args[0];
}

typedef struct ml_onion_url_t {
	const ml_type_t *Type;
	onion_url *Handle;
} ml_onion_url_t;

static ml_type_t *OnionUrlT;

ML_METHOD("root_url", OnionT) {
	ml_onion_t *Onion = (ml_onion_t *)Args[0];
	ml_onion_url_t *Url = new(ml_onion_url_t);
	Url->Type = OnionUrlT;
	Url->Handle = onion_root_url(Onion->Handle);
	return (ml_value_t *)Url;
}

static ml_value_t *ml_onion_url_new_fn(void *Data, int Count, ml_value_t **Args) {
	ml_onion_url_t *Url = new(ml_onion_url_t);
	Url->Type = OnionUrlT;
	Url->Handle = onion_url_new();
	return (ml_value_t *)Url;
}

ML_METHOD("add", OnionUrlT, MLStringT, MLFunctionT) {
	ml_onion_url_t *Url = (ml_onion_url_t *)Args[0];
	onion_url_add_with_data(Url->Handle, ml_string_value(Args[1]), ml_onion_handler, Args[2], NULL);
	return Args[0];
}

ML_METHOD("add", OnionUrlT, MLStringT, OnionHandlerT) {
	ml_onion_url_t *Url = (ml_onion_url_t *)Args[0];
	ml_onion_handler_t *Handler = (ml_onion_handler_t *)Args[1];
	onion_url_add_handler(Url->Handle, ml_string_value(Args[1]), Handler->Handle);
	return Args[0];
}

ML_METHOD("add_static", OnionUrlT, MLStringT, MLIntegerT, MLStringT) {
	ml_onion_url_t *Url = (ml_onion_url_t *)Args[0];
	onion_url_add_static(Url->Handle, ml_string_value(Args[1]), ml_string_value(Args[3]), ml_integer_value(Args[2]));
	return Args[0];
}

ML_METHOD("to_handler", OnionUrlT) {
	ml_onion_url_t *Url = (ml_onion_url_t *)Args[0];
	ml_onion_handler_t *Handler = new(ml_onion_handler_t);
	Handler->Type = OnionHandlerT;
	Handler->Handle = onion_url_to_handler(Url->Handle);
	return (ml_value_t *)Handler;
}

typedef struct ml_onion_websocket_t {
	const ml_type_t *Type;
	onion_websocket *Handle;
	ml_value_t *Callback;
	ml_stringbuffer_t Buffer[1];
} ml_onion_websocket_t;

static ml_type_t *OnionWebsocketT;

static onion_connection_status ml_onion_websocket_callback(ml_onion_websocket_t *Websocket, onion_websocket *Handle, ssize_t DataReadyLength) {
	if (DataReadyLength >= 0) {
		char Buffer[128];
		ml_value_t *Result = MLNil;
		while (DataReadyLength > 128) {
			int Size = onion_websocket_read(Handle, Buffer, 128);
			if (Size < 0) {
				Result = ml_error("ReadError", "Error reading from websocket: %s", strerror(errno));
				goto done;
			}
			ml_stringbuffer_write(Websocket->Buffer, Buffer, Size);
			DataReadyLength -= Size;
		}
		while (DataReadyLength > 0) {
			int Size = onion_websocket_read(Handle, Buffer, DataReadyLength);
			if (Size < 0) {
				Result = ml_error("ReadError", "Error reading from websocket: %s", strerror(errno));
				goto done;
			}
			ml_stringbuffer_write(Websocket->Buffer, Buffer, Size);
			DataReadyLength -= Size;
		}
	done:
		if (Result->Type == MLErrorT) {
			printf("Error: %s\n", ml_error_message(Result));
			ml_source_t Source;
			int Level = 0;
			while (ml_error_source(Result, Level++, &Source)) {
				printf("\t%s:%d\n", Source.Name, Source.Line);
			}
			return OCS_INTERNAL_ERROR;
		}
		if (Result->Type == OnionConnectionStatusT) {
			return ((ml_onion_connection_status_t *)Result)->Value;
		} else {
			return OCS_NEED_MORE_DATA;
		}
	} else {
		ml_value_t *Result = ml_simple_inline(Websocket->Callback, 1, MLNil);
		if (Result->Type == OnionConnectionStatusT) {
			return ((ml_onion_connection_status_t *)Result)->Value;
		} else {
			return OCS_CLOSE_CONNECTION;
		}
	}
}

static ml_value_t *ml_websocket_new_fn(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(2);
	ML_CHECK_ARG_TYPE(0, OnionRequestT);
	ML_CHECK_ARG_TYPE(1, OnionResponseT);
	ml_onion_request_t *Request = (ml_onion_request_t *)Args[0];
	ml_onion_response_t *Response = (ml_onion_response_t *)Args[1];
	onion_websocket *Handle = onion_websocket_new(Request->Handle, Response->Handle);
	if (!Handle) return MLNil;
	ml_onion_websocket_t *Websocket = new(ml_onion_websocket_t);
	Websocket->Type = OnionWebsocketT;
	Websocket->Handle = Handle;
	onion_websocket_set_userdata(Handle, Websocket, NULL);
	onion_websocket_set_callback(Websocket->Handle, (void *)ml_onion_websocket_callback);
	return (ml_value_t *)Websocket;
}

ML_METHOD("set_callback", OnionWebsocketT, MLFunctionT) {
	ml_onion_websocket_t *Websocket = (ml_onion_websocket_t *)Args[0];
	Websocket->Callback = Args[1];
	return Args[0];
}

typedef struct ml_onion_websocket_opcode_t {
	const ml_type_t *Type;
	const char *Name;
	onion_websocket_opcode Value;
} ml_onion_websocket_opcode_t;

static ml_type_t *OnionWebsocketOpcodeT;

static ml_value_t *ml_onion_websocket_opcode(const char *Name, onion_websocket_opcode Value) {
	ml_onion_websocket_opcode_t *Opcode = new(ml_onion_websocket_opcode_t);
	Opcode->Type = OnionWebsocketOpcodeT;
	Opcode->Name = Name;
	Opcode->Value = Value;
	return (ml_value_t *)Opcode;
}

ML_METHOD("set_opcode", OnionWebsocketT, OnionWebsocketOpcodeT) {
	ml_onion_websocket_t *Websocket = (ml_onion_websocket_t *)Args[0];
	ml_onion_websocket_opcode_t *Opcode = (ml_onion_websocket_opcode_t *)Args[1];
	onion_websocket_set_opcode(Websocket->Handle, Opcode->Value);
	return Args[0];
}

ML_METHOD("write", OnionWebsocketT, MLStringT) {
	ml_onion_websocket_t *Websocket = (ml_onion_websocket_t *)Args[0];
	int Result = onion_websocket_write(Websocket->Handle, ml_string_value(Args[1]), ml_string_length(Args[1]));
	return ml_integer(Result);
}

void memory_allocation_error(const char *Message) {
	ONION_ERROR("memory failure: %s (%s)", Message, strerror(errno));
	exit(EXIT_FAILURE);
}

void *GC_calloc(size_t N, size_t Size) {
	return GC_malloc(N * Size);
}

void ml_library_entry0(ml_value_t *Module) {
	onion_low_initialize_memory_allocation(
		GC_malloc, GC_malloc_atomic, GC_calloc,
		GC_realloc, GC_strdup, GC_free,
		memory_allocation_error
	);
	onion_low_initialize_threads(
		GC_pthread_create, GC_pthread_join,
		GC_pthread_cancel, GC_pthread_detach,
		GC_pthread_exit, GC_pthread_sigmask
	);

	OnionT = ml_type(MLAnyT, "onion");
	OnionHandlerT = ml_type(MLAnyT, "onion-handler");
	OnionUrlT = ml_type(MLAnyT, "onion-url");
	OnionRequestT = ml_type(MLAnyT, "onion-request");
	OnionResponseT = ml_type(MLAnyT, "onion-response");
	OnionStateT = ml_type(MLStateT, "onion-state");
	OnionWebsocketT = ml_type(MLAnyT, "onion-websocket");
#include "ml_onion_init.c"
	ml_module_export(Module, "new", ml_cfunction(NULL, ml_onion_new_fn));
	ml_module_export(Module, "handler", ml_cfunction(NULL, ml_onion_handler_new_fn));
	ml_module_export(Module, "local", ml_cfunction(NULL, ml_onion_export_local_fn));
	ml_module_export(Module, "static", ml_cfunction(NULL, ml_onion_handler_static_fn));
	ml_module_export(Module, "websocket", ml_cfunction(NULL, ml_websocket_new_fn));
	ml_module_export(Module, "connection_status", ml_module("connection_status",
		"NotProcessed", ml_onion_connection_status("NotProcessed", OCS_NOT_PROCESSED),
		"NeedMoreData", ml_onion_connection_status("NeedMoreData", OCS_NEED_MORE_DATA),
		"Processed", ml_onion_connection_status("Processed", OCS_PROCESSED),
		"CloseConnection", ml_onion_connection_status("CloseConnection", OCS_CLOSE_CONNECTION),
		"KeepAlive", ml_onion_connection_status("KeepAlive", OCS_KEEP_ALIVE),
		"Websocket", ml_onion_connection_status("Websocket", OCS_WEBSOCKET),
		"RequestReady", ml_onion_connection_status("RequestReady", OCS_REQUEST_READY),
		"InternalError", ml_onion_connection_status("InternalError", OCS_INTERNAL_ERROR),
		"NotImplemented", ml_onion_connection_status("NotImplemented", OCS_NOT_IMPLEMENTED),
		"Forbidden", ml_onion_connection_status("Forbidden", OCS_FORBIDDEN),
		"Yield", ml_onion_connection_status("Yield", OCS_YIELD),
	NULL));
	ml_module_export(Module, "websocket_opcode", ml_module("websocket_opcode",
		"Text", ml_onion_websocket_opcode("Text", OWS_TEXT),
		"Binary", ml_onion_websocket_opcode("Text", OWS_BINARY),
		"ConnectionClose", ml_onion_websocket_opcode("Text", OWS_CONNECTION_CLOSE),
		"Ping", ml_onion_websocket_opcode("Text", OWS_PING),
		"Pong", ml_onion_websocket_opcode("Text", OWS_PONG),
	NULL));
}
