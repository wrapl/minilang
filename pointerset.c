#include "pointerset.h"
#include "ml_macros.h"
#include <gc.h>
#include <string.h>
#include <stdint.h>

#define INITIAL_SIZE 4

void pointerset_init(pointerset_t *Set, int Min) {
	int Size = 1;
	while (Size < Min) Size <<= 1;
	Set->Size = Size;
	Set->Space = Size;
	Set->Pointers = anew(void *, Size + 1);
}

static void sort_pointers(void **First, void **Last) {
	void **A = First;
	void **B = Last;
	void *T = *A;
	void *P = *B;
	while (!P) {
		--B;
		--Last;
		if (A == B) return;
		P = *B;
	}
	while (A != B) {
		if (T > P) {
			*A = T;
			T = *++A;
		} else {
			*B = T;
			T = *--B;
		}
	}
	*A = P;
	if (First < A - 1) sort_pointers(First, A - 1);
	if (A + 1 < Last) sort_pointers(A + 1, Last);
}

int pointerset_insert(pointerset_t *Set, void *Pointer) {
	int Hash = ((intptr_t)Pointer) >> 4;
	int Incr = (((intptr_t)Pointer) >> 8) | 1;
	void **Pointers = Set->Pointers;
	if (!Pointers) {
		Pointers = Set->Pointers = anew(void *, INITIAL_SIZE + 1);
		Set->Size = INITIAL_SIZE;
		Set->Space = INITIAL_SIZE - 1;
		Pointers[Hash & (INITIAL_SIZE - 1)] = Pointer;
		return 1;
	}
	int Mask = Set->Size - 1;
	int Index = Hash & Mask;
	for (;;) {
		if (Pointers[Index] == Pointer) return 0;
		if (Pointers[Index] < Pointer) break;
		Index += Incr;
		Index &= Mask;
	}
	if (--Set->Space > (Set->Size >> 3)) {
		void *OldPointer = Pointers[Index];
		Pointers[Index] = Pointer;
		while (OldPointer) {
			Pointer = OldPointer;
			Incr = (((intptr_t)Pointer) >> 8) | 1;
			Index += Incr;
			Index &= Mask;
			for (;;) {
				if (Pointers[Index] < Pointer) {
					OldPointer = Pointers[Index];
					Pointers[Index] = Pointer;
					break;
				}
				Index += Incr;
				Index &= Mask;
			}
		}
		return 1;
	}
	int NewSize = Set->Size * 2;
	void **NewPointers = anew(void *, NewSize + 1);
	Pointers[Set->Size] = Pointer;
	Mask = NewSize - 1;
	sort_pointers(Pointers, Pointers + Set->Size);
	for (void **Old = Pointers; *Old; ++Old) {
		Pointer = *Old;
		Hash = ((intptr_t)Pointer) >> 4;
		Incr = (((intptr_t)Pointer) >> 8) | 1;
		Index = Hash & Mask;
		while (NewPointers[Index]) {
			Index += Incr;
			Index &= Mask;
		}
		NewPointers[Index] = Pointer;
	}
	Set->Space += Set->Size;
	Set->Size = NewSize;
	Set->Pointers = NewPointers;
	return 1;
}

int pointerset_foreach(pointerset_t *Set, void *Data, int (*callback)(void *Pointer, void *Data)) {
	void **End = Set->Pointers + Set->Size;
	for (void **T = Set->Pointers; T < End; ++T) {
		if (*T && callback(*T, Data)) return 1;
	}
	return 0;
}
