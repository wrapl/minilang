#include "ml_libevent.h"
#include "ml_file.h"
#include "ml_macros.h"
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

void ml_event_init(stringmap_t *Globals) {
	EventBaseT = ml_type(MLAnyT, "event-base");
	EventT = ml_type(MLAnyT, "event");
	EventHttpT = ml_type(MLAnyT, "event-http");
	if (Globals) {
		ml_value_t *Event = ml_map();
		ml_map_insert(Event, ml_string("new", -1), ml_function(NULL, ml_event_base_new));
		stringmap_insert(Globals, "event", Event);
	}
#include "ml_libevent_init.c"
}
