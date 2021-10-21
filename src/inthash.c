#include "inthash.h"
#include "ml_macros.h"
#include <gc/gc.h>

inthash_t *inthash_new() {
	return new(inthash_t);
}

#ifndef ASM_INTHASH_SEARCH
void *inthash_search(const inthash_t *Map, uintptr_t Key) {
	if (!Map->Size) return NULL;
	uintptr_t *Keys = Map->Keys;
	size_t Mask = Map->Size - 1;
	size_t Index = (Key >> INTHASH_INDEX_SHIFT) & Mask;
	if (Keys[Index] == Key) return Map->Values[Index];
	if (Keys[Index] < Key) return NULL;
	size_t Incr = (Key >> INTHASH_INCR_SHIFT) | 1;
	do {
		Index = (Index + Incr) & Mask;
		if (Keys[Index] == Key) return Map->Values[Index];
	} while (Keys[Index] > Key);
	return NULL;
}

inthash_result_t inthash_search2(const inthash_t *Map, uintptr_t Key) {
	if (!Map->Size) return (inthash_result_t){NULL, 0};
	uintptr_t *Keys = Map->Keys;
	size_t Mask = Map->Size - 1;
	size_t Index = (Key >> INTHASH_INDEX_SHIFT) & Mask;
	if (Keys[Index] == Key) return (inthash_result_t){Map->Values[Index], 1};
	if (Keys[Index] < Key) return (inthash_result_t){NULL, 0};
	size_t Incr = (Key >> INTHASH_INCR_SHIFT) | 1;
	do {
		Index = (Index + Incr) & Mask;
		if (Keys[Index] == Key) return (inthash_result_t){Map->Values[Index], 1};
	} while (Keys[Index] > Key);
	return (inthash_result_t){NULL, 0};
}
#endif

static void inthash_nodes_sort(uintptr_t *KeyA, uintptr_t *KeyB, void **ValueA, void **ValueB) {
	uintptr_t *KeyA1 = KeyA, *KeyB1 = KeyB;
	uintptr_t KeyTemp = *KeyA;
	uintptr_t KeyPivot = *KeyB;
	void **ValueA1 = ValueA, **ValueB1 = ValueB;
	void *ValueTemp = *ValueA;
	void *ValuePivot = *ValueB;
	while (KeyA1 < KeyB1) {
		if (KeyTemp > KeyPivot) {
			*KeyA1 = KeyTemp;
			++KeyA1;
			KeyTemp = *KeyA1;
			*ValueA1 = ValueTemp;
			++ValueA1;
			ValueTemp = *ValueA1;
		} else {
			*KeyB1 = KeyTemp;
			--KeyB1;
			KeyTemp = *KeyB1;
			*ValueB1 = ValueTemp;
			--ValueB1;
			ValueTemp = *ValueB1;
		}
	}
	*KeyA1 = KeyPivot;
	*ValueA1 = ValuePivot;
	if (KeyA1 - KeyA > 1) inthash_nodes_sort(KeyA, KeyA1 - 1, ValueA, ValueA1 - 1);
	if (KeyB - KeyB1 > 1) inthash_nodes_sort(KeyB1 + 1, KeyB, ValueB1 + 1, ValueB);
}

#define INITIAL_SIZE 8

void *inthash_insert(inthash_t *Map, uintptr_t Key, void *Value) {
	uintptr_t *Keys = Map->Keys;
	void **Values = Map->Values;
	if (!Keys) {
		Keys = Map->Keys = anew(uintptr_t, INITIAL_SIZE);
		Values = Map->Values = anew(void *, INITIAL_SIZE);
		Map->Size = INITIAL_SIZE;
		Map->Space = INITIAL_SIZE - 1;
		size_t Index =  (Key >> INTHASH_INDEX_SHIFT) & (INITIAL_SIZE - 1);
		Keys[Index] = Key;
		Values[Index] = Value;
		return NULL;
	}
	size_t Mask = Map->Size - 1;
	size_t Index = (Key >> INTHASH_INDEX_SHIFT) & Mask;
	size_t Incr = (Key >> INTHASH_INCR_SHIFT) | 1;
	for (;;) {
		if (Keys[Index] == Key) {
			void *Old = Values[Index];
			Values[Index] = Value;
			return Old;
		}
		if (Keys[Index] < Key) break;
		Index = (Index + Incr) & Mask;
	}
	if (--Map->Space > (Map->Size >> 2)) {
		uintptr_t Key1 = Keys[Index];
		void *Value1 = Values[Index];
		Keys[Index] = Key;
		Values[Index] = Value;
		while (Key1) {
			Incr = (Key1 >> INTHASH_INCR_SHIFT) | 1;
			while (Keys[Index] > Key1) Index = (Index + Incr) & Mask;
			uintptr_t Key2 = Keys[Index];
			void *Value2 = Values[Index];
			Keys[Index] = Key1;
			Values[Index] = Value1;
			Key1 = Key2;
			Value1 = Value2;
		}
	} else {
		while (Keys[Index]) Index = (Index + 1) & Mask;
		Keys[Index] = Key;
		Values[Index] = Value;
		inthash_nodes_sort(Keys, Keys + Mask, Values, Values + Mask);
		size_t Size2 = 2 * Map->Size;
		Mask = Size2 - 1;
		uintptr_t *Keys2 = anew(uintptr_t, Size2);
		void **Values2 = anew(void *, Size2);
		for (int I = 0; Keys[I]; ++I) {
			uintptr_t Key2 = Keys[I];
			void *Value2 = Values[I];
			size_t Index2 = (Key2 >> INTHASH_INDEX_SHIFT) & Mask;
			size_t Incr2 = (Key2 >> INTHASH_INCR_SHIFT) | 1;
			while (Keys2[Index2]) Index2 = (Index2 + Incr2) & Mask;
			Keys2[Index2] = Key2;
			Values2[Index2] = Value2;
		}
		Map->Keys = Keys2;
		Map->Values = Values2;
		Map->Space += Map->Size;
		Map->Size = Size2;
	}
	return NULL;
}
