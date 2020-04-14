#include "../ml_file.h"
#include "../ml_library.h"
#include "../ml_macros.h"
#include <gc/gc.h>
#include <event2/event.h>
#include <event2/http.h>

typedef struct ml_event_base_t {
	const ml_type_t *Type;
	struct event_base *Handle;
} ml_event_base_t;

static ml_type_t *EventBaseT;

static ml_value_t *ml_event_base_new(void *Data, int Count, ml_value_t **Args) {
	ml_event_base_t *EventBase = new(ml_event_base_t);
	EventBase->Type = EventBaseT;
	EventBase->Handle = event_base_new();
	return (ml_value_t *)EventBase;
}

typedef struct ml_event_t {
	const ml_type_t *Type;
	struct event *Handle;
} ml_event_t;

static ml_type_t *EventT;

ML_METHOD("new", EventBaseT, MLFileT) {
	ml_event_base_t *EventBase = (ml_event_base_t *)Args[0];

	return MLNil;
}

ML_METHOD("dispatch", EventBaseT) {
	ml_event_base_t *EventBase = (ml_event_base_t *)Args[0];
	switch (event_base_dispatch(EventBase->Handle)) {
	case 1: return MLNil;
	case -1: return ml_error("EventError", "Event error occurerd");
	default: return Args[0];
	}
}

typedef struct ml_evhttp_t {
	const ml_type_t *Type;
	struct evhttp *Handle;
} ml_evhttp_t;

static ml_type_t *EventHttpT;

ML_METHOD("http", EventBaseT) {
	ml_event_base_t *EventBase = (ml_event_base_t *)Args[0];
	ml_evhttp_t *Http = new(ml_evhttp_t);
	Http->Type = EventHttpT;
	Http->Handle = evhttp_new(EventBase->Handle);
	return (ml_value_t *)Http;
}

void ml_library_entry(ml_value_t *Module, ml_getter_t GlobalGet, void *Globals) {
	EventBaseT = ml_type(MLAnyT, "event-base");
	EventT = ml_type(MLAnyT, "event");
	EventHttpT = ml_type(MLAnyT, "event-http");
#include "ml_libevent_init.c"
	ml_module_export(Module, "new", ml_function(NULL, ml_event_base_new));
}
