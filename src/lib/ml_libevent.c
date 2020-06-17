#include "../ml_file.h"
#include "../ml_library.h"
#include "../ml_macros.h"
#include <gc/gc.h>
#include <event2/event.h>
#include <event2/http.h>

typedef struct event_base_t {
	const ml_type_t *Type;
	struct event_base *Handle;
} event_base_t;

ML_TYPE(EventBaseT, (), "event-base");

ML_FUNCTION(EventBaseNew) {
	event_base_t *EventBase = new(event_base_t);
	EventBase->Type = EventBaseT;
	EventBase->Handle = event_base_new();
	return (ml_value_t *)EventBase;
}

typedef struct event_t {
	const ml_type_t *Type;
	struct event *Handle;
} ml_event_t;

ML_TYPE(EventT, (), "event");

ML_METHOD("new", EventBaseT, MLFileT) {
	event_base_t *EventBase = (event_base_t *)Args[0];
	EventBase->Type = EventBaseT;
	return (ml_value_t *)EventBase;
}

ML_METHOD("dispatch", EventBaseT) {
	event_base_t *EventBase = (event_base_t *)Args[0];
	switch (event_base_dispatch(EventBase->Handle)) {
	case 1: return MLNil;
	case -1: return ml_error("EventError", "Event error occurerd");
	default: return Args[0];
	}
}

ML_FUNCTIONX(EventSleep) {
	ML_RETURN(MLNil);
}

typedef struct evhttp_t {
	const ml_type_t *Type;
	struct evhttp *Handle;
} evhttp_t;

ML_TYPE(EventHttpT, (), "event-http");

ML_METHOD("http", EventBaseT) {
	event_base_t *EventBase = (event_base_t *)Args[0];
	evhttp_t *Http = new(evhttp_t);
	Http->Type = EventHttpT;
	Http->Handle = evhttp_new(EventBase->Handle);
	return (ml_value_t *)Http;
}

void ml_library_entry(ml_value_t *Module, ml_getter_t GlobalGet, void *Globals) {
#include "ml_libevent_init.c"
	ml_module_export(Module, "new", (ml_value_t *)EventBaseNew);
	ml_module_export(Module, "sleep", (ml_value_t *)EventSleep);
}
