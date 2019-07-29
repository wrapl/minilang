#include <gc.h>
#include <string.h>
#include "minilang.h"
#include "ml_macros.h"
#include "ml_iterfns.h"

static ml_value_t *all_fn(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(1);
	ml_value_t *Iterator = Args[0]->Type->iterate(Args[0]);
	if (Iterator->Type == MLErrorT) return Iterator;
	ml_value_t *List = ml_list();
	while (Iterator != MLNil) {
		ml_value_t *Value = Iterator->Type->current(Iterator);
		Value = Value->Type->deref(Value);
		if (Value->Type == MLErrorT) return Value;
		ml_list_append(List, Value);
		Iterator = Iterator->Type->next(Iterator);
		if (Iterator->Type == MLErrorT) return Iterator;
	}
	return List;
}

static ml_value_t *map_fn(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(1);
	ml_value_t *Iterator = Args[0]->Type->iterate(Args[0]);
	if (Iterator->Type == MLErrorT) return Iterator;
	ml_value_t *Map = ml_map();
	while (Iterator != MLNil) {
		ml_value_t *Key = Iterator->Type->key(Iterator);
		Key = Key->Type->deref(Key);
		if (Key->Type == MLErrorT) return Key;
		ml_value_t *Value = Iterator->Type->current(Iterator);
		Value = Value->Type->deref(Value);
		if (Value->Type == MLErrorT) return Value;
		ml_map_insert(Map, Key, Value);
		Iterator = Iterator->Type->next(Iterator);
		if (Iterator->Type == MLErrorT) return Iterator;
	}
	return Map;
}

static ml_value_t *uniq_fn(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(1);
	ml_value_t *Iterator = Args[0]->Type->iterate(Args[0]);
	if (Iterator->Type == MLErrorT) return Iterator;
	ml_value_t *Map = ml_map();
	while (Iterator != MLNil) {
		ml_value_t *Value = Iterator->Type->current(Iterator);
		Value = Value->Type->deref(Value);
		if (Value->Type == MLErrorT) return Value;
		ml_map_insert(Map, Value, MLSome);
		Iterator = Iterator->Type->next(Iterator);
		if (Iterator->Type == MLErrorT) return Iterator;
	}
	return Map;
}

static ml_value_t *count_fn(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(1);
	ml_value_t *Iterator = Args[0]->Type->iterate(Args[0]);
	if (Iterator->Type == MLErrorT) return Iterator;
	int Total = 0;
	while (Iterator != MLNil) {
		++Total;
		Iterator = Iterator->Type->next(Iterator);
		if (Iterator->Type == MLErrorT) return Iterator;
	}
	return ml_integer(Total);
}

static ml_value_t *LessMethod, *GreaterMethod, *AddMethod, *MulMethod;

static ml_value_t *min_fn(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(1);
	ml_value_t *Iterator = Args[0]->Type->iterate(Args[0]);
	if (Iterator->Type == MLErrorT) return Iterator;
	if (Iterator == MLNil) return MLNil;
	ml_value_t *FoldArgs[2] = {Iterator->Type->current(Iterator), 0};
	FoldArgs[0] = FoldArgs[0]->Type->deref(FoldArgs[0]);
	if (FoldArgs[0]->Type == MLErrorT) return FoldArgs[0];
	Iterator = Iterator->Type->next(Iterator);
	if (Iterator->Type == MLErrorT) return Iterator;
	while (Iterator != MLNil) {
		FoldArgs[1] = Iterator->Type->current(Iterator);
		FoldArgs[1] = FoldArgs[1]->Type->deref(FoldArgs[1]);
		if (FoldArgs[1]->Type == MLErrorT) return FoldArgs[1];
		ml_value_t *Compare = ml_call(GreaterMethod, 2, FoldArgs);
		if (Compare->Type == MLErrorT) return Compare;
		if (Compare != MLNil) FoldArgs[0] = FoldArgs[1];
		Iterator = Iterator->Type->next(Iterator);
		if (Iterator->Type == MLErrorT) return Iterator;
	}
	return FoldArgs[0];
}

static ml_value_t *max_fn(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(1);
	ml_value_t *Iterator = Args[0]->Type->iterate(Args[0]);
	if (Iterator->Type == MLErrorT) return Iterator;
	if (Iterator == MLNil) return MLNil;
	ml_value_t *FoldArgs[2] = {Iterator->Type->current(Iterator), 0};
	FoldArgs[0] = FoldArgs[0]->Type->deref(FoldArgs[0]);
	if (FoldArgs[0]->Type == MLErrorT) return FoldArgs[0];
	Iterator = Iterator->Type->next(Iterator);
	if (Iterator->Type == MLErrorT) return Iterator;
	while (Iterator != MLNil) {
		FoldArgs[1] = Iterator->Type->current(Iterator);
		FoldArgs[1] = FoldArgs[1]->Type->deref(FoldArgs[1]);
		if (FoldArgs[1]->Type == MLErrorT) return FoldArgs[1];
		ml_value_t *Compare = ml_call(LessMethod, 2, FoldArgs);
		if (Compare->Type == MLErrorT) return Compare;
		if (Compare != MLNil) FoldArgs[0] = FoldArgs[1];
		Iterator = Iterator->Type->next(Iterator);
		if (Iterator->Type == MLErrorT) return Iterator;
	}
	return FoldArgs[0];
}

static ml_value_t *sum_fn(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(1);
	ml_value_t *Iterator = Args[0]->Type->iterate(Args[0]);
	if (Iterator->Type == MLErrorT) return Iterator;
	if (Iterator == MLNil) return MLNil;
	ml_value_t *FoldArgs[2] = {Iterator->Type->current(Iterator), 0};
	FoldArgs[0] = FoldArgs[0]->Type->deref(FoldArgs[0]);
	if (FoldArgs[0]->Type == MLErrorT) return FoldArgs[0];
	Iterator = Iterator->Type->next(Iterator);
	if (Iterator->Type == MLErrorT) return Iterator;
	while (Iterator != MLNil) {
		FoldArgs[1] = Iterator->Type->current(Iterator);
		FoldArgs[1] = FoldArgs[1]->Type->deref(FoldArgs[1]);
		if (FoldArgs[1]->Type == MLErrorT) return FoldArgs[1];
		FoldArgs[0] = ml_call(AddMethod, 2, FoldArgs);
		if (FoldArgs[0]->Type == MLErrorT) return FoldArgs[0];
		Iterator = Iterator->Type->next(Iterator);
		if (Iterator->Type == MLErrorT) return Iterator;
	}
	return FoldArgs[0];
}

static ml_value_t *prod_fn(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(1);
	ml_value_t *Iterator = Args[0]->Type->iterate(Args[0]);
	if (Iterator->Type == MLErrorT) return Iterator;
	if (Iterator == MLNil) return MLNil;
	ml_value_t *FoldArgs[2] = {Iterator->Type->current(Iterator), 0};
	FoldArgs[0] = FoldArgs[0]->Type->deref(FoldArgs[0]);
	if (FoldArgs[0]->Type == MLErrorT) return FoldArgs[0];
	Iterator = Iterator->Type->next(Iterator);
	if (Iterator->Type == MLErrorT) return Iterator;
	while (Iterator != MLNil) {
		FoldArgs[1] = Iterator->Type->current(Iterator);
		FoldArgs[1] = FoldArgs[1]->Type->deref(FoldArgs[1]);
		if (FoldArgs[1]->Type == MLErrorT) return FoldArgs[1];
		FoldArgs[0] = ml_call(MulMethod, 2, FoldArgs);
		if (FoldArgs[0]->Type == MLErrorT) return FoldArgs[0];
		Iterator = Iterator->Type->next(Iterator);
		if (Iterator->Type == MLErrorT) return Iterator;
	}
	return FoldArgs[0];
}

void ml_iterfns_init(stringmap_t *Globals) {
	LessMethod = ml_method("<");
	GreaterMethod = ml_method(">");
	AddMethod = ml_method("+");
	MulMethod = ml_method("*");
	stringmap_insert(Globals, "all", ml_function(0, all_fn));
	stringmap_insert(Globals, "map", ml_function(0, map_fn));
	stringmap_insert(Globals, "uniq", ml_function(0, uniq_fn));
	stringmap_insert(Globals, "count", ml_function(0, count_fn));
	stringmap_insert(Globals, "min", ml_function(0, min_fn));
	stringmap_insert(Globals, "max", ml_function(0, max_fn));
	stringmap_insert(Globals, "sum", ml_function(0, sum_fn));
	stringmap_insert(Globals, "prod", ml_function(0, prod_fn));
}
